package main

import (
	"encoding/base64"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strconv"
	"time"
	"anki/log"

	extint "proto/external_interface"
	grpcRuntime "github.com/grpc-ecosystem/grpc-gateway/runtime"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)
	
type Command struct {
	Item struct {
		Message struct {
			S string `json:"S"`
		} `json:"message"`
		Type struct {
			S string `json:"S"`
		} `json:"type"`
		Order struct {
			N string `json:"N"`
		} `json:"order"`
	} `json:"Item"`
	ResponseMetadata struct {
		RequestID      string `json:"RequestId"`
		HTTPStatusCode int    `json:"HTTPStatusCode"`
		HTTPHeaders    struct {
			Server         string `json:"server"`
			Date           string `json:"date"`
			ContentType    string `json:"content-type"`
			ContentLength  string `json:"content-length"`
			Connection     string `json:"connection"`
			XAmznRequestid string `json:"x-amzn-requestid"`
			XAmzCrc32      string `json:"x-amz-crc32"`
		} `json:"HTTPHeaders"`
		RetryAttempts int `json:"RetryAttempts"`
	} `json:"ResponseMetadata"`
}

type TriggerLambdaMessageBody struct {
	TotalMessageCount int
}

type Storage struct {
	AwsApiKey string
	RpcCallToken string
	AwsApiGwyBaseUrl string
}

type Token struct {
	AuthorizationToken string
}

func (tok *Token) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return map[string]string{
		"Authorization": tok.AuthorizationToken,
	}, nil
}

func (tok *Token) RequireTransportSecurity() bool {
	return true
}

func launchSkill(AwsApiGwyBaseUrl string, AwsApiKey string) int {
	url := AwsApiGwyBaseUrl + "/CloudSkills-Trigger"
	req, err := http.NewRequest("GET", url, nil)
	req.Header.Add("x-api-key", AwsApiKey)
	query := req.URL.Query()
	query.Add("skill_key", "skills/skill_1.py")
	req.URL.RawQuery = query.Encode()
	client := &http.Client{}
    response, err := client.Do(req)
    if err != nil {
        panic(err)
    }
    defer response.Body.Close()

    body, err := ioutil.ReadAll(response.Body)

	var jsonMessageBody TriggerLambdaMessageBody
    if err := json.Unmarshal(body, &jsonMessageBody); err != nil {
        panic(err)
    }

    return jsonMessageBody.TotalMessageCount
}

func fetchCloudSkill(creds credentials.TransportCredentials) {
	// TODO: Identify an ideal secure storage mechanism on the robot to store these keys
	data, err := ioutil.ReadFile("/data/vic-gateway/cloud-skills-data.json")
	if err != nil {
        panic(err)
    }
	var cloudSkillConstants Storage
	if err := json.Unmarshal(data, &cloudSkillConstants); err != nil {
        panic(err)
    }

	perRPCCreds := Token{
		AuthorizationToken: cloudSkillConstants.RpcCallToken,
	}
	conn, err := grpc.Dial("localhost:443", 
							grpc.WithTransportCredentials(creds),
							grpc.WithPerRPCCredentials(&perRPCCreds))
	if err != nil {
		log.Println("CloudSkill: did not connect: %v", err)
	}
	defer conn.Close()
	
	ctx := context.Background()
	c := extint.NewExternalInterfaceClient(conn)
	gwmux := grpcRuntime.NewServeMux(grpcRuntime.WithMarshalerOption(grpcRuntime.MIMEWildcard, &grpcRuntime.JSONPb{EmitDefaults: true, OrigName: true, EnumsAsInts: true}))
	extint.RegisterExternalInterfaceHandlerClient(ctx, gwmux, c)
	time.Sleep(5 * time.Second)

	behaviorControlClient, err := c.BehaviorControl(ctx)
	if err != nil {
        panic(err)
    }

    // Gain SDK behavior control
	behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRequest{ControlRequest: &extint.ControlRequest{Priority: 20}}})

	// Trigger skill
	numberOfCommands := launchSkill(cloudSkillConstants.AwsApiGwyBaseUrl, cloudSkillConstants.AwsApiKey)
	client := &http.Client{}

	commandId := 1
 
	for {

		// All commands run, release SDK behavior control
		if commandId > numberOfCommands {
			time.Sleep(5 * time.Second)
			behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRelease{ControlRelease: &extint.ControlRelease{}}})
			return
		}

		url := cloudSkillConstants.AwsApiGwyBaseUrl + "/CloudSkills-GetCommand"
		req, err := http.NewRequest("GET", url, nil)
		
		// Add API key to header
		req.Header.Add("x-api-key", cloudSkillConstants.AwsApiKey)
		
		// Request command by id
		query := req.URL.Query()
		query.Add("command_id", strconv.Itoa(commandId))
		req.URL.RawQuery = query.Encode()
	    response, err := client.Do(req)
	    if err != nil {
	        panic(err)
	    }
	    defer response.Body.Close()

	    body, err := ioutil.ReadAll(response.Body)

	 	var jsonCommand Command
	    if err := json.Unmarshal(body, &jsonCommand); err != nil {
	        panic(err)
	    }

	    encodedMessage := jsonCommand.Item.Message.S
	 	decodedMessage, err := base64.StdEncoding.DecodeString(encodedMessage)

	 	// TODO: Find a better way to handle incoming serialized proto messages
	 	switch messageType := jsonCommand.Item.Type.S; messageType {
	 	case "/Anki.Vector.external_interface.ExternalInterface/SayText": {
	 		var sayTextReq extint.SayTextRequest
	    	sayTextReq.XXX_Unmarshal(decodedMessage)
	    	c.SayText(ctx, &sayTextReq)
	 	}
	 	case "/Anki.Vector.external_interface.ExternalInterface/MoveLift": {
	 		var moveLiftReq extint.MoveLiftRequest
	    	moveLiftReq.XXX_Unmarshal(decodedMessage)
	    	c.MoveLift(ctx, &moveLiftReq)
	 	}

	 	}

	    commandId += 1

	}
}