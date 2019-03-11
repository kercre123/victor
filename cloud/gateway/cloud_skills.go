package main

import (
	"encoding/base64"
	"encoding/json"
	"io/ioutil"
	"strconv"
	"anki/log"

	extint "proto/external_interface"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"github.com/gorilla/websocket"
)

const maxRetries = 10

var skillExecuting = false
	
type Command struct {
	Message_type string
	Message string
}

type Storage struct {
	AwsApiKey string
	RpcCallToken string
	AwsApiGwyBaseUrl string
}

type Token struct {
	AuthorizationToken string
}

type AwsRequest struct {
	Action string
	SkillKey string
}

type AwsRobotResponseRequest struct {
	Action string
	MessageDeDuplicationId string
	RobotResponse interface{}
}

func (tok *Token) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return map[string]string{
		"Authorization": tok.AuthorizationToken,
	}, nil
}

func (tok *Token) RequireTransportSecurity() bool {
	return true
}

func launchSkill(websocketClient *websocket.Conn) {
	req := AwsRequest{
		Action: "trigger",
		SkillKey: "skills/skill_2.py",
	}
	reqBytes, err := json.Marshal(req)
	if err != nil {
		panic(err)
	}
	err = websocketClient.WriteMessage(websocket.TextMessage, reqBytes)
	if err != nil {
		panic(err)
	}
}

func sendResponseToAws(websocketClient *websocket.Conn, id int, res interface{}) {
	req := AwsRobotResponseRequest{
		Action: "response",
		MessageDeDuplicationId: strconv.Itoa(id),
		RobotResponse: res,
	}
	log.Println(req)
	resBytes, err := json.Marshal(req)
 	if err != nil {
 		panic(err)
 	}
 	err = websocketClient.WriteMessage(websocket.TextMessage, resBytes)
}

func startCloudSkill(c extint.ExternalInterfaceClient, ctx context.Context, cloudSkillConstants Storage) {
	behaviorControlClient, err := c.BehaviorControl(ctx)
	if err != nil {
        panic(err)
    }

    // Gain SDK behavior control
	behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRequest{ControlRequest: &extint.ControlRequest{Priority: 20}}})


	// Create a websocket client
	websocketClient, _, err := websocket.DefaultDialer.Dial("wss://5tgocjykol.execute-api.us-west-1.amazonaws.com/test/", nil)
	if err != nil {
		panic(err)
	}
	defer websocketClient.Close()

	launchSkill(websocketClient)

	messageId := 0

	for {
		log.Println("Start read")
		_, message, err := websocketClient.ReadMessage()
		if err != nil {
			panic(err)
		}
		log.Println("End read")

		messageId += 1
		
		var jsonCommand Command
	    if err := json.Unmarshal(message, &jsonCommand); err != nil {
	    	log.Println(err)
	        panic(err)
	    }

	    if jsonCommand.Message_type == "end_skill" {
	    	behaviorControlClient.Send(&extint.BehaviorControlRequest{RequestType: &extint.BehaviorControlRequest_ControlRelease{ControlRelease: &extint.ControlRelease{}}})
	    	skillExecuting = false
	    	return
	    }

	    log.Println(jsonCommand)

		// Decode the message received
	    encodedMessage := jsonCommand.Message
	 	decodedMessage, err := base64.StdEncoding.DecodeString(encodedMessage)

	 	// TODO: Find a better way to handle incoming serialized proto messages
	 	switch messageType := jsonCommand.Message_type; messageType {
	 	case "say_text": {
	 		var sayTextReq extint.SayTextRequest
	    	sayTextReq.XXX_Unmarshal(decodedMessage)
	    	res, _ := c.SayText(ctx, &sayTextReq)
	    	sendResponseToAws(websocketClient, messageId, res)
	 	}
	 	case "set_lift": {
	 		var moveLiftReq extint.MoveLiftRequest
	    	moveLiftReq.XXX_Unmarshal(decodedMessage)
	    	res, _ := c.MoveLift(ctx, &moveLiftReq)
	    	sendResponseToAws(websocketClient, messageId, res)
	 	}
	 	case "set_head": {
	 		var moveHeadReq extint.MoveHeadRequest
	    	moveHeadReq.XXX_Unmarshal(decodedMessage)
	    	res, _ := c.MoveHead(ctx, &moveHeadReq)
	    	sendResponseToAws(websocketClient, messageId, res)
	 	}
	 	case "set_wheel": {
	 		var driveWheelsReq extint.DriveWheelsRequest
	    	driveWheelsReq.XXX_Unmarshal(decodedMessage)
	    	res, _ := c.DriveWheels(ctx, &driveWheelsReq)
	    	sendResponseToAws(websocketClient, messageId, res)
	 	}
	 	case "play_animation": {
	 		var playAnimationReq extint.PlayAnimationRequest
	    	playAnimationReq.XXX_Unmarshal(decodedMessage)
	    	res, _ := c.PlayAnimation(ctx, &playAnimationReq)
	    	sendResponseToAws(websocketClient, messageId, res)
	 	}
	 	}
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