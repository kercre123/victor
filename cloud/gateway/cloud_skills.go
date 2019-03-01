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
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

const maxRetries = 10

var skillExecuting = false
	
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

func launchSkill(AwsApiGwyBaseUrl string, AwsApiKey string) {
	url := AwsApiGwyBaseUrl + "/CloudSkills-Trigger"
	req, err := http.NewRequest("GET", url, nil)
	req.Header.Add("x-api-key", AwsApiKey)
	query := req.URL.Query()
	query.Add("skill_key", "skills/skill_2.py")
	req.URL.RawQuery = query.Encode()
	client := &http.Client{}
    response, err := client.Do(req)
    if err != nil {
        panic(err)
    }
    defer response.Body.Close()
}

func startCloudSkill(c extint.ExternalInterfaceClient, ctx context.Context, cloudSkillConstants Storage) {
	behaviorControlClient, err := c.BehaviorControl(ctx)
	if err != nil {
        panic(err)
    }

    // Gain SDK behavior control
	behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRequest{ControlRequest: &extint.ControlRequest{Priority: 20}}})

	startTime := time.Now()
	// Trigger the skill
	go launchSkill(cloudSkillConstants.AwsApiGwyBaseUrl, cloudSkillConstants.AwsApiKey)
	elapsedTime := time.Since(startTime)
	log.Println("Elapsed time [trigger] = ", elapsedTime)
	
	client := &http.Client{}

	commandId := 0
	numRetries := 0
	url := cloudSkillConstants.AwsApiGwyBaseUrl + "/CloudSkills-GetCommand"

	for {
		startTime = time.Now()

		// All commands run, release SDK behavior control
		if numRetries >= maxRetries {
			behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRelease{ControlRelease: &extint.ControlRelease{}}})
			skillExecuting = false
			return
		}

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

	    // Check if the fetch failed or if the command has not been queued in yet
	    if len(body) == 0 {
	    	numRetries += 1
	    	time.Sleep(1 * time.Second)
	    	continue
	    }

	    // Reset retry count
	    numRetries = 0

	 	var jsonCommand Command
	    if err := json.Unmarshal(body, &jsonCommand); err != nil {
	        panic(err)
	    }

	    // Decode the message received
	    encodedMessage := jsonCommand.Item.Message.S
	 	decodedMessage, err := base64.StdEncoding.DecodeString(encodedMessage)

	 	// TODO: Find a better way to handle incoming serialized proto messages
	 	switch messageType := jsonCommand.Item.Type.S; messageType {
	 	case "say_text": {
	 		var sayTextReq extint.SayTextRequest
	    	sayTextReq.XXX_Unmarshal(decodedMessage)
	    	c.SayText(ctx, &sayTextReq)
	 	}
	 	case "set_lift": {
	 		var moveLiftReq extint.MoveLiftRequest
	    	moveLiftReq.XXX_Unmarshal(decodedMessage)
	    	c.MoveLift(ctx, &moveLiftReq)
	 	}
	 	case "set_head": {
	 		var moveHeadReq extint.MoveHeadRequest
	    	moveHeadReq.XXX_Unmarshal(decodedMessage)
	    	c.MoveHead(ctx, &moveHeadReq)
	 	}
	 	case "set_wheel_motors": {
	 		var driveWheelsReq extint.DriveWheelsRequest
	    	driveWheelsReq.XXX_Unmarshal(decodedMessage)
	    	c.DriveWheels(ctx, &driveWheelsReq)
	 	}
	 	case "play_animation": {
	 		var playAnimationReq extint.PlayAnimationRequest
	    	playAnimationReq.XXX_Unmarshal(decodedMessage)
	    	c.PlayAnimation(ctx, &playAnimationReq)
	 	}

	 	}

	 	elapsedTime = time.Since(startTime)
	 	log.Println("Elapsed time [fetch command] = ", elapsedTime)

	    commandId += 1

	}
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

	f, eventsChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_Event{}, 10)
	defer f()

	// Listen for petting events and trigger skill if robot it pet
	for {
		response, ok := <-eventsChannel
		if !ok {
			log.Println("EventStream: event channel closed")
		}
		event := response.GetEvent()
		robotState := event.GetRobotState()
    	if robotState != nil {
    		touchData := robotState.GetTouchData()
    		if touchData != nil && touchData.GetIsBeingTouched() && !skillExecuting {
    			skillExecuting = true
    			go startCloudSkill(c, ctx, cloudSkillConstants)
    		}
    	}
	}
}