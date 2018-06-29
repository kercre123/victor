package main

import (
	"bytes"
	"encoding/binary"
	"math"
	"reflect"
	"time"

	"anki/ipc"
	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const faceImagePixelsPerChunk = 600

func WriteToEngine(conn ipc.Conn, msg *gw_clad.MessageExternalToRobot) (int, error) {
	var err error
	var buf bytes.Buffer
	if msg == nil {
		return -1, status.Errorf(codes.InvalidArgument, "Unable to parse request")
	}
	if err = binary.Write(&buf, binary.LittleEndian, uint16(msg.Size())); err != nil {
		return -1, status.Errorf(codes.Internal, err.Error())
	}
	if err = msg.Pack(&buf); err != nil {
		return -1, status.Errorf(codes.Internal, err.Error())
	}
	return engineSock.Write(buf.Bytes())
}

func WriteProtoToEngine(conn ipc.Conn, msg proto.Message) (int, error) {
	var err error
	var buf bytes.Buffer
	if msg == nil {
		return -1, status.Errorf(codes.InvalidArgument, "Unable to parse request")
	}
	if err = binary.Write(&buf, binary.LittleEndian, uint16(proto.Size(msg))); err != nil {
		return -1, status.Errorf(codes.Internal, err.Error())
	}
	data, err := proto.Marshal(msg)
	if err != nil {
		return -1, status.Errorf(codes.Internal, err.Error())
	}
	if err = binary.Write(&buf, binary.LittleEndian, data); err != nil {
		return -1, status.Errorf(codes.Internal, err.Error())
	}
	return protoEngineSock.Write(buf.Bytes())
}

func createChannel(tag interface{}, numChannels int) (func(), chan extint.GatewayWrapper) {
	result := make(chan extint.GatewayWrapper, numChannels)
	reflectedType := reflect.TypeOf(tag).String()
	log.Println("Listening for", reflectedType)
	engineProtoChanMap[reflectedType] = result
	return func() {
		delete(engineProtoChanMap, reflectedType)
	}, result
}

func ClearMapSetting(tag gw_clad.MessageRobotToExternalTag) {
	delete(engineChanMap, tag)
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoDriveWheelsToClad(msg *extint.DriveWheelsRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDriveWheels(&gw_clad.DriveWheels{
		LeftWheelMmps:   msg.LeftWheelMmps,
		RightWheelMmps:  msg.RightWheelMmps,
		LeftWheelMmps2:  msg.LeftWheelMmps2,
		RightWheelMmps2: msg.RightWheelMmps2,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoPlayAnimationToClad(msg *extint.PlayAnimationRequest) *gw_clad.MessageExternalToRobot {
	if msg.Animation == nil {
		return nil
	}
	log.Println("Animation name:", msg.Animation.Name)
	return gw_clad.NewMessageExternalToRobotWithPlayAnimation(&gw_clad.PlayAnimation{
		NumLoops:        msg.Loops,
		AnimationName:   msg.Animation.Name,
		IgnoreBodyTrack: false,
		IgnoreHeadTrack: false,
		IgnoreLiftTrack: false,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoMoveHeadToClad(msg *extint.MoveHeadRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithMoveHead(&gw_clad.MoveHead{
		SpeedRadPerSec: msg.SpeedRadPerSec,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoMoveLiftToClad(msg *extint.MoveLiftRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithMoveLift(&gw_clad.MoveLift{
		SpeedRadPerSec: msg.SpeedRadPerSec,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoDriveArcToClad(msg *extint.DriveArcRequest) *gw_clad.MessageExternalToRobot {
	// Bind msg.CurvatureRadiusMm which is an int32 to an int16 boundary to prevent overflow
	if msg.CurvatureRadiusMm < math.MinInt16 {
		msg.CurvatureRadiusMm = math.MinInt16
	} else if msg.CurvatureRadiusMm > math.MaxInt16 {
		msg.CurvatureRadiusMm = math.MaxInt16
	}
	return gw_clad.NewMessageExternalToRobotWithDriveArc(&gw_clad.DriveArc{
		Speed:             msg.Speed,
		Accel:             msg.Accel,
		CurvatureRadiusMm: int16(msg.CurvatureRadiusMm),
		// protobuf does not have a 16-bit integer representation, this conversion is used to satisfy the specs specified in the clad
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoSDKActivationToClad(msg *extint.SDKActivationRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithSDKActivationRequest(&gw_clad.SDKActivationRequest{
		Slot:   msg.Slot,
		Enable: msg.Enable,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func FaceImageChunkToClad(faceData [faceImagePixelsPerChunk]uint16, pixelCount uint16, chunkIndex uint8, chunkCount uint8, durationMs uint32, interruptRunning bool) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDisplayFaceImageRGBChunk(&gw_clad.DisplayFaceImageRGBChunk{
		FaceData:         faceData,
		NumPixels:        pixelCount,
		ChunkIndex:       chunkIndex,
		NumChunks:        chunkCount,
		DurationMs:       durationMs,
		InterruptRunning: interruptRunning,
	})
}

func ProtoAppIntentToClad(msg *extint.AppIntentRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithAppIntent(&gw_clad.AppIntent{
		Param:  msg.Param,
		Intent: msg.Intent,
	})
}

func ProtoRequestEnrolledNamesToClad(msg *extint.RequestEnrolledNamesRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithRequestEnrolledNames(&gw_clad.RequestEnrolledNames{})
}

func ProtoCancelFaceEnrollmentToClad(msg *extint.CancelFaceEnrollmentRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithCancelFaceEnrollment(&gw_clad.CancelFaceEnrollment{})
}

func ProtoUpdateEnrolledFaceByIDToClad(msg *extint.UpdateEnrolledFaceByIDRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithUpdateEnrolledFaceByID(&gw_clad.UpdateEnrolledFaceByID{
		FaceID:  msg.FaceId,
		OldName: msg.OldName,
		NewName: msg.NewName,
	})
}

func ProtoEraseEnrolledFaceByIDToClad(msg *extint.EraseEnrolledFaceByIDRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithEraseEnrolledFaceByID(&gw_clad.EraseEnrolledFaceByID{
		FaceID: msg.FaceId,
	})
}

func ProtoEraseAllEnrolledFacesToClad(msg *extint.EraseAllEnrolledFacesRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithEraseAllEnrolledFaces(&gw_clad.EraseAllEnrolledFaces{})
}

func ProtoSetFaceToEnrollToClad(msg *extint.SetFaceToEnrollRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithSetFaceToEnroll(&gw_clad.SetFaceToEnroll{
		Name:        msg.Name,
		ObservedID:  msg.ObservedId,
		SaveID:      msg.SaveId,
		SaveToRobot: msg.SaveToRobot,
		SayName:     msg.SayName,
		UseMusic:    msg.UseMusic,
	})
}

func ProtoListAnimationsToClad(msg *extint.ListAnimationsRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithRequestAvailableAnimations(&gw_clad.RequestAvailableAnimations{})
}

func ProtoEnableVisionModeToClad(msg *extint.EnableVisionModeRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithEnableVisionMode(&gw_clad.EnableVisionMode{
		Mode:   gw_clad.VisionMode(msg.Mode),
		Enable: msg.Enable,
	})
}

func CladFeatureStatusToProto(msg *gw_clad.FeatureStatus) *extint.FeatureStatus {
	proto_msg := reflect.ValueOf(*msg).Interface().(extint.FeatureStatus)
	return &proto_msg
}

func CladMeetVictorFaceScanStartedToProto(msg *gw_clad.MeetVictorFaceScanStarted) *extint.MeetVictorFaceScanStarted {
	proto_msg := reflect.ValueOf(*msg).Interface().(extint.MeetVictorFaceScanStarted)
	return &proto_msg
}

func CladMeetVictorFaceScanCompleteToProto(msg *gw_clad.MeetVictorFaceScanComplete) *extint.MeetVictorFaceScanComplete {
	proto_msg := reflect.ValueOf(*msg).Interface().(extint.MeetVictorFaceScanComplete)
	return &proto_msg
}

func CladStatusToProto(msg *gw_clad.Status) *extint.Status {
	var status *extint.Status
	switch tag := msg.Tag(); tag {
	case gw_clad.StatusTag_FeatureStatus:
		status = &extint.Status{
			StatusType: &extint.Status_FeatureStatus{
				CladFeatureStatusToProto(msg.GetFeatureStatus()),
			},
		}
	case gw_clad.StatusTag_MeetVictorFaceScanStarted:
		status = &extint.Status{
			StatusType: &extint.Status_MeetVictorFaceScanStarted{
				CladMeetVictorFaceScanStartedToProto(msg.GetMeetVictorFaceScanStarted()),
			},
		}
	case gw_clad.StatusTag_FaceEnrollmentCompleted:
		status = &extint.Status{
			StatusType: &extint.Status_MeetVictorFaceScanComplete{
				CladMeetVictorFaceScanCompleteToProto(msg.GetMeetVictorFaceScanComplete()),
			},
		}
	case gw_clad.StatusTag_MeetVictorFaceScanComplete:
		status = &extint.Status{
			StatusType: &extint.Status_MeetVictorFaceScanComplete{
				CladMeetVictorFaceScanCompleteToProto(msg.GetMeetVictorFaceScanComplete()),
			},
		}
	case gw_clad.StatusTag_INVALID:
		log.Println(tag, "tag for status is invalid")
		return nil
	default:
		log.Println(tag, "tag for status is not yet implemented")
		return nil
	}
	return status
}

func CladCladRectToProto(msg *gw_clad.CladRect) *extint.CladRect {
	return &extint.CladRect{
		XTopLeft: msg.XTopLeft,
		YTopLeft: msg.YTopLeft,
		Width:    msg.Width,
		Height:   msg.Height,
	}
}

func CladCladPointsToProto(msg []gw_clad.CladPoint2d) []*extint.CladPoint {
	var points []*extint.CladPoint
	for _, point := range msg {
		points = append(points, &extint.CladPoint{X: point.X, Y: point.Y})
	}
	return points
}

func CladExpressionValuesToProto(msg []uint8) []uint32 {
	var expression_values []uint32
	for _, val := range msg {
		expression_values = append(expression_values, uint32(val))
	}
	return expression_values
}

func CladRobotObservedFaceToProto(msg *gw_clad.RobotObservedFace) *extint.RobotObservedFace {
	// BlinkAmount, Gaze and SmileAmount are not exposed to the SDK
	return &extint.RobotObservedFace{
		FaceId:    msg.FaceID,
		Timestamp: msg.Timestamp,
		Pose:      CladPoseToProto(&msg.Pose),
		ImgRect:   CladCladRectToProto(&msg.ImgRect),
		Name:      msg.Name,

		Expression: extint.FacialExpression(msg.Expression + 1), // protobuf enums have a 0 start value

		// Individual expression values histogram, sums to 100 (Exception: all zero if expressio: msg.
		ExpressionValues: CladExpressionValuesToProto(msg.ExpressionValues[:]),

		// Face landmarks
		LeftEye:  CladCladPointsToProto(msg.LeftEye),
		RightEye: CladCladPointsToProto(msg.RightEye),
		Nose:     CladCladPointsToProto(msg.Nose),
		Mouth:    CladCladPointsToProto(msg.Mouth),
	}
}

func CladRobotChangedObservedFaceIDToProto(msg *gw_clad.RobotChangedObservedFaceID) *extint.RobotChangedObservedFaceID {
	return &extint.RobotChangedObservedFaceID{
		OldId: msg.OldID,
		NewId: msg.NewID,
	}
}

func CladPoseToProto(msg *gw_clad.PoseStruct3d) *extint.PoseStruct {
	return &extint.PoseStruct{
		X:        msg.X,
		Y:        msg.Y,
		Z:        msg.Z,
		Q0:       msg.Q0,
		Q1:       msg.Q1,
		Q2:       msg.Q2,
		Q3:       msg.Q3,
		OriginId: msg.OriginID,
	}
}

func CladAccelDataToProto(msg *gw_clad.AccelData) *extint.AccelData {
	return &extint.AccelData{
		X: msg.X,
		Y: msg.Y,
		Z: msg.Z,
	}
}

func CladGyroDataToProto(msg *gw_clad.GyroData) *extint.GyroData {
	return &extint.GyroData{
		X: msg.X,
		Y: msg.Y,
		Z: msg.Z,
	}
}

func CladRobotStateToProto(msg *gw_clad.RobotState) *extint.RobotStateResult {
	var robot_state *extint.RobotState
	robot_state = &extint.RobotState{
		Pose:                  CladPoseToProto(&msg.Pose),
		PoseAngleRad:          msg.PoseAngleRad,
		PosePitchRad:          msg.PosePitchRad,
		LeftWheelSpeedMmps:    msg.LeftWheelSpeedMmps,
		RightWheelSpeedMmps:   msg.RightWheelSpeedMmps,
		HeadAngleRad:          msg.HeadAngleRad,
		LiftHeightMm:          msg.LiftHeightMm,
		BatteryVoltage:        msg.BatteryVoltage,
		Accel:                 CladAccelDataToProto(&msg.Accel),
		Gyro:                  CladGyroDataToProto(&msg.Gyro),
		CarryingObjectId:      msg.CarryingObjectID,
		CarryingObjectOnTopId: msg.CarryingObjectOnTopID,
		HeadTrackingObjectId:  msg.HeadTrackingObjectID,
		LocalizedToObjectId:   msg.LocalizedToObjectID,
		LastImageTimeStamp:    msg.LastImageTimeStamp,
		Status:                msg.Status,
		GameStatus:            uint32(msg.GameStatus), // protobuf does not have a uint8 representation, so cast to a uint32
	}

	return &extint.RobotStateResult{
		RobotState: robot_state,
	}
}

func CladEventToProto(msg *gw_clad.Event) *extint.Event {
	switch tag := msg.Tag(); tag {
	case gw_clad.EventTag_Status:
		return &extint.Event{
			EventType: &extint.Event_Status{
				CladStatusToProto(msg.GetStatus()),
			},
		}
	case gw_clad.EventTag_INVALID:
		log.Println(tag, "tag is invalid")
		return nil
	default:
		log.Println(tag, "tag is not yet implemented")
		return nil
	}
}

func SendOnboardingContinue(in *extint.GatewayWrapper_OnboardingContinue) (*extint.OnboardingInputResponse, error) {
	f, result := createChannel(&extint.GatewayWrapper_OnboardingContinueResponse{}, 1)
	defer f()
	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	continue_result := <-result
	return &extint.OnboardingInputResponse{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingContinueResponse{
			continue_result.GetOnboardingContinueResponse(),
		},
	}, nil
}

func SendOnboardingSkip(in *extint.GatewayWrapper_OnboardingSkip) (*extint.OnboardingInputResponse, error) {
	log.Println("Received rpc request OnboardingSkip(", in, ")")
	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func SendOnboardingConnectionComplete(in *extint.GatewayWrapper_OnboardingConnectionComplete) (*extint.OnboardingInputResponse, error) {
	log.Println("Received rpc request OnbOnboardingConnectionCompleteoardingSkip(", in, ")")
	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

// The service definition.
// This must implement all the rpc functions defined in the external_interface proto file.
type rpcService struct{}

func (m *rpcService) DriveWheels(ctx context.Context, in *extint.DriveWheelsRequest) (*extint.DriveWheelsResult, error) {
	log.Println("Received rpc request DriveWheels(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoDriveWheelsToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.DriveWheelsResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) PlayAnimation(ctx context.Context, in *extint.PlayAnimationRequest) (*extint.PlayAnimationResult, error) {
	log.Println("Received rpc request PlayAnimation(", in, ")")
	animation_result := make(chan RobotToExternalCladResult)
	engineChanMap[gw_clad.MessageRobotToExternalTag_RobotCompletedAction] = animation_result
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_RobotCompletedAction)
	_, err := WriteToEngine(engineSock, ProtoPlayAnimationToClad(in))
	if err != nil {
		return nil, err
	}
	<-animation_result // TODO: Properly handle the result
	return &extint.PlayAnimationResult{
		Status: &extint.ResultStatus{
			Description: "Animation completed",
		},
	}, nil
}

func (m *rpcService) ListAnimations(ctx context.Context, in *extint.ListAnimationsRequest) (*extint.ListAnimationsResult, error) {
	log.Println("Received rpc request ListAnimations(", in, ")")

	// NOTE: The channels have been having issues getting clogged
	//  currently allocating a large number of channels to minimize failures
	animationAvailableResponse := make(chan RobotToExternalCladResult, 1000)
	engineChanMap[gw_clad.MessageRobotToExternalTag_AnimationAvailable] = animationAvailableResponse
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_AnimationAvailable)

	// NOTE: The end of message has 5 channels because there was an issue
	//  where with one (or 2) channels the endOfMessage was occasionally in a similar
	//  way to the clogging on animationAvailableResponse.  This occurred despite
	//  seeing the intent to broadcast this once from the engine side.
	endOfMessageResponse := make(chan RobotToExternalCladResult, 5)
	engineChanMap[gw_clad.MessageRobotToExternalTag_EndOfMessage] = endOfMessageResponse
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_EndOfMessage)

	_, err := WriteToEngine(engineSock, ProtoListAnimationsToClad(in))
	if err != nil {
		return nil, err
	}

	var anims []*extint.Animation

	done := false
	for done == false {
		select {
		case chanResponse := <-animationAvailableResponse:
			var newAnim = extint.Animation{
				Name: chanResponse.Message.GetAnimationAvailable().AnimName,
			}
			anims = append(anims, &newAnim)
		case chanResponse := <-endOfMessageResponse:
			if chanResponse.Message.GetEndOfMessage().MessageType == gw_clad.MessageType_AnimationAvailable {
				log.Println("Final animation list message recieved - count: ", len(anims))
				done = true
			}
		case <-time.After(5 * time.Second):
			return nil, status.Errorf(codes.DeadlineExceeded, "ListAnimations request timed out")
		}
	}

	return &extint.ListAnimationsResult{
		Status: &extint.ResultStatus{
			Description: "Available animations returned",
		},
		AnimationNames: anims,
	}, nil
}

func (m *rpcService) MoveHead(ctx context.Context, in *extint.MoveHeadRequest) (*extint.MoveHeadResult, error) {
	log.Println("Received rpc request MoveHead(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoMoveHeadToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.MoveHeadResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) MoveLift(ctx context.Context, in *extint.MoveLiftRequest) (*extint.MoveLiftResult, error) {
	log.Println("Received rpc request MoveLift(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoMoveLiftToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.MoveLiftResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) DriveArc(ctx context.Context, in *extint.DriveArcRequest) (*extint.DriveArcResult, error) {
	log.Println("Received rpc request DriveArc(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoDriveArcToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.DriveArcResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func SendFaceDataAsChunks(in *extint.DisplayFaceImageRGBRequest, chunkCount int, pixelsPerChunk int, totalPixels int) error {

	var convertedUint16Data [faceImagePixelsPerChunk]uint16

	// cycle until we run out of bytes to transfer
	for i := 0; i < chunkCount; i++ {

		pixelCount := faceImagePixelsPerChunk
		if i == chunkCount-1 {
			pixelCount = totalPixels - faceImagePixelsPerChunk*i
		}

		firstByte := (pixelsPerChunk * 2) * i
		finalByte := firstByte + (pixelCount * 2)
		slicedBinaryData := in.FaceData[firstByte:finalByte]

		for j := 0; j < pixelCount; j++ {
			uintAsBytes := slicedBinaryData[j*2 : j*2+2]
			convertedUint16Data[j] = binary.BigEndian.Uint16(uintAsBytes)
		}

		// Copy a subset of the pixels to the bytes?
		message := FaceImageChunkToClad(convertedUint16Data, uint16(pixelCount), uint8(i), uint8(chunkCount), in.DurationMs, in.InterruptRunning)

		_, err := WriteToEngine(engineSock, message)
		if err != nil {
			return err
		}
	}

	return nil
}

func (m *rpcService) DisplayFaceImageRGB(ctx context.Context, in *extint.DisplayFaceImageRGBRequest) (*extint.DisplayFaceImageRGBResult, error) {
	log.Println("Received rpc request SetOLEDToSolidColor(", in, ")")

	const totalPixels = 17664
	chunkCount := (totalPixels + faceImagePixelsPerChunk + 1) / faceImagePixelsPerChunk

	SendFaceDataAsChunks(in, chunkCount, faceImagePixelsPerChunk, totalPixels)

	return &extint.DisplayFaceImageRGBResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

// TODO: This should be handled as an event stream once RobotState is made an event
// Long running message for sending events to listening sdk users
func (c *rpcService) RobotStateStream(in *extint.RobotStateRequest, stream extint.ExternalInterface_RobotStateStreamServer) error {
	log.Println("Received rpc request RobotStateStream(", in, ")")

	stream_channel := make(chan RobotToExternalCladResult, 16)
	engineChanMap[gw_clad.MessageRobotToExternalTag_RobotState] = stream_channel
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_RobotState)
	log.Println(engineChanMap[gw_clad.MessageRobotToExternalTag_RobotState])

	for result := range stream_channel {
		log.Println("Got result:", result)
		robot_state := CladRobotStateToProto(result.Message.GetRobotState())
		log.Println("Made RobotState:", robot_state)
		if err := stream.Send(robot_state); err != nil {
			return err
		} else if err = stream.Context().Err(); err != nil {
			// This is the case where the user disconnects the stream
			// We should still return the err in case the user doesn't think they disconnected
			return err
		}
	}
	return nil
}

func (m *rpcService) AppIntent(ctx context.Context, in *extint.AppIntentRequest) (*extint.AppIntentResult, error) {
	log.Println("Received rpc request AppIntent(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoAppIntentToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.AppIntentResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) CancelFaceEnrollment(ctx context.Context, in *extint.CancelFaceEnrollmentRequest) (*extint.CancelFaceEnrollmentResult, error) {
	log.Println("Received rpc request CancelFaceEnrollment(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoCancelFaceEnrollmentToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.CancelFaceEnrollmentResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) RequestEnrolledNames(ctx context.Context, in *extint.RequestEnrolledNamesRequest) (*extint.RequestEnrolledNamesResult, error) {
	log.Println("Received rpc request RequestEnrolledNames(", in, ")")
	enrolledNamesResponse := make(chan RobotToExternalCladResult)
	engineChanMap[gw_clad.MessageRobotToExternalTag_EnrolledNamesResponse] = enrolledNamesResponse
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_EnrolledNamesResponse)
	_, err := WriteToEngine(engineSock, ProtoRequestEnrolledNamesToClad(in))
	if err != nil {
		return nil, err
	}
	names := <-enrolledNamesResponse
	var faces []*extint.LoadedKnownFace
	for _, element := range names.Message.GetEnrolledNamesResponse().Faces {
		var newFace = extint.LoadedKnownFace{
			SecondsSinceFirstEnrolled: element.SecondsSinceFirstEnrolled,
			SecondsSinceLastUpdated:   element.SecondsSinceLastUpdated,
			SecondsSinceLastSeen:      element.SecondsSinceLastSeen,
			LastSeenSecondsSinceEpoch: element.LastSeenSecondsSinceEpoch,
			FaceId: element.FaceID,
			Name:   element.Name,
		}
		faces = append(faces, &newFace)
	}
	return &extint.RequestEnrolledNamesResult{
		Status: &extint.ResultStatus{
			Description: "Enrolled names returned",
		},
		Faces: faces,
	}, nil
}

// TODO Wait for response RobotRenamedEnrolledFace
func (m *rpcService) UpdateEnrolledFaceByID(ctx context.Context, in *extint.UpdateEnrolledFaceByIDRequest) (*extint.UpdateEnrolledFaceByIDResult, error) {
	log.Println("Received rpc request UpdateEnrolledFaceByID(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoUpdateEnrolledFaceByIDToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.UpdateEnrolledFaceByIDResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

// TODO Wait for response RobotRenamedEnrolledFace
func (m *rpcService) EraseEnrolledFaceByID(ctx context.Context, in *extint.EraseEnrolledFaceByIDRequest) (*extint.EraseEnrolledFaceByIDResult, error) {
	log.Println("Received rpc request EraseEnrolledFaceByID(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoEraseEnrolledFaceByIDToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EraseEnrolledFaceByIDResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

// TODO Wait for response RobotErasedAllEnrolledFaces
func (m *rpcService) EraseAllEnrolledFaces(ctx context.Context, in *extint.EraseAllEnrolledFacesRequest) (*extint.EraseAllEnrolledFacesResult, error) {
	log.Println("Received rpc request EraseAllEnrolledFaces(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoEraseAllEnrolledFacesToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EraseAllEnrolledFacesResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) SetFaceToEnroll(ctx context.Context, in *extint.SetFaceToEnrollRequest) (*extint.SetFaceToEnrollResult, error) {
	log.Println("Received rpc request SetFaceToEnroll(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoSetFaceToEnrollToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.SetFaceToEnrollResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

// Long running message for sending events to listening sdk users
func (c *rpcService) EventStream(in *extint.EventRequest, stream extint.ExternalInterface_EventStreamServer) error {
	log.Println("Received rpc request EventStream(", in, ")")

	f, eventsChannel := createChannel(&extint.GatewayWrapper_Event{}, 16)
	defer f()

	for result := range eventsChannel {
		log.Println("Got result:", result)
		eventResult := &extint.EventResult{
			Event: result.GetEvent(),
		}
		log.Println("Made event:", eventResult)
		if err := stream.Send(eventResult); err != nil {
			return err
		} else if err = stream.Context().Err(); err != nil {
			// This is the case where the user disconnects the stream
			// We should still return the err in case the user doesn't think they disconnected
			return err
		}
	}
	return nil
}

func (m *rpcService) SDKBehaviorActivation(ctx context.Context, in *extint.SDKActivationRequest) (*extint.SDKActivationResult, error) {
	log.Println("Received rpc request SDKBehaviorActivation(", in, ")")
	if in.Enable {
		// Request control from behavior system
		return SDKBehaviorRequestActivation(in)
	}

	return SDKBehaviorRequestDeactivation(in)
}

// Request control from behavior system.
func SDKBehaviorRequestActivation(in *extint.SDKActivationRequest) (*extint.SDKActivationResult, error) {
	// We are enabling the SDK behavior. Wait for the engine-to-game reply
	// that the SDK behavior was activated.
	sdk_activation_result := make(chan RobotToExternalCladResult)
	engineChanMap[gw_clad.MessageRobotToExternalTag_SDKActivationResult] = sdk_activation_result
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_SDKActivationResult)

	_, err := WriteToEngine(engineSock, ProtoSDKActivationToClad(in))
	if err != nil {
		return nil, err
	}
	result := <-sdk_activation_result
	return &extint.SDKActivationResult{
		Status: &extint.ResultStatus{
			Description: "SDKActivationResult returned",
		},
		Slot:    result.Message.GetSDKActivationResult().Slot,
		Enabled: result.Message.GetSDKActivationResult().Enabled,
	}, nil
}

func SDKBehaviorRequestDeactivation(in *extint.SDKActivationRequest) (*extint.SDKActivationResult, error) {
	// Deactivate the SDK behavior. Note that we don't wait for a reply
	// so that our SDK program can exit immediately.
	_, err := WriteToEngine(engineSock, ProtoSDKActivationToClad(in))
	if err != nil {
		return nil, err
	}

	// Note: SDKActivationResult contains Slot and Enabled, which we are currently
	// ignoring when we deactivate the SDK behavior since we are not waiting for
	// the SDKActivationResult message to return.
	return &extint.SDKActivationResult{
		Status: &extint.ResultStatus{
			Description: "SDKActivationResult returned",
		},
	}, nil
}

// Example sending an int to and receiving an int from the engine
// TODO: Remove this example code once more code is converted to protobuf
func (m *rpcService) Pang(ctx context.Context, in *extint.Ping) (*extint.Pong, error) {
	log.Println("Received rpc request Ping(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_Pong{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_Ping{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	pong := <-result
	return pong.GetPong(), nil
}

// Example sending a string to and receiving a string from the engine
// TODO: Remove this example code once more code is converted to protobuf
func (m *rpcService) Bang(ctx context.Context, in *extint.Bing) (*extint.Bong, error) {
	log.Println("Received rpc request Bing(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_Bong{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_Bing{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	bong := <-result
	return bong.GetBong(), nil
}

// Request the current robot onboarding status
func (m *rpcService) GetOnboardingState(ctx context.Context, in *extint.OnboardingStateRequest) (*extint.OnboardingStateResponse, error) {
	log.Println("Received rpc request GetOnboardingState(", in, ")")
	f, result := createChannel(&extint.GatewayWrapper_OnboardingState{}, 1)
	defer f()
	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_OnboardingStateRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	onboarding_state := <-result
	return &extint.OnboardingStateResponse{
		OnboardingState: onboarding_state.GetOnboardingState(),
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) SendOnboardingInput(ctx context.Context, in *extint.OnboardingInputRequest) (*extint.OnboardingInputResponse, error) {
	log.Println("Received rpc request OnboardingInputRequest(", in, ")")

	// oneof_message_type
	switch x := in.OneofMessageType.(type) {
	case *extint.OnboardingInputRequest_OnboardingContinue:
		return SendOnboardingContinue(&extint.GatewayWrapper_OnboardingContinue{
			in.GetOnboardingContinue(),
		})
	case *extint.OnboardingInputRequest_OnboardingSkip:
		return SendOnboardingSkip(&extint.GatewayWrapper_OnboardingSkip{
			in.GetOnboardingSkip(),
		})
	case *extint.OnboardingInputRequest_OnboardingConnectionComplete:
		return SendOnboardingConnectionComplete(&extint.GatewayWrapper_OnboardingConnectionComplete{
			in.GetOnboardingConnectionComplete(),
		})
	default:
		return nil, status.Errorf(0, "OnboardingInputRequest.OneofMessageType has unexpected type %T", x)
	}
}

func (m *rpcService) PhotosInfo(ctx context.Context, in *extint.PhotosInfoRequest) (*extint.PhotosInfoResponse, error) {
	log.Println("Received rpc request PhotosInfo(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_PhotosInfoResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PhotosInfoRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload := <-result
	return payload.GetPhotosInfoResponse(), nil
}

func (m *rpcService) Photo(ctx context.Context, in *extint.PhotoRequest) (*extint.PhotoResponse, error) {
	log.Println("Received rpc request Photo(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_PhotoResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PhotoRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload := <-result
	return payload.GetPhotoResponse(), nil
}

func (m *rpcService) Thumbnail(ctx context.Context, in *extint.ThumbnailRequest) (*extint.ThumbnailResponse, error) {
	log.Println("Received rpc request Thumbnail(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_ThumbnailResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ThumbnailRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload := <-result
	return payload.GetThumbnailResponse(), nil
}

func (m *rpcService) DeletePhoto(ctx context.Context, in *extint.DeletePhotoRequest) (*extint.DeletePhotoResponse, error) {
	log.Println("Received rpc request DeletePhoto(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_DeletePhotoResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DeletePhotoRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload := <-result
	return payload.GetDeletePhotoResponse(), nil
}

func (m *rpcService) GetLatestAttentionTransfer(ctx context.Context, in *extint.LatestAttentionTransferRequest) (*extint.LatestAttentionTransferResponse, error) {
	log.Println("Received rpc request GetLatestAttentionTransfer(", in, ")")
	f, result := createChannel(&extint.GatewayWrapper_LatestAttentionTransfer{}, 1)
	defer f()
	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_LatestAttentionTransferRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	attention_transfer := <-result
	return &extint.LatestAttentionTransferResponse{
		LatestAttentionTransfer: attention_transfer.GetLatestAttentionTransfer(),
		Status: &extint.ResultStatus{
			Description: "Response from engine",
		},
	}, nil
}

func (m *rpcService) EnableVisionMode(ctx context.Context, in *extint.EnableVisionModeRequest) (*extint.EnableVisionModeResult, error) {
	log.Println("Received rpc request EnableVisionMode(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoEnableVisionModeToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EnableVisionModeResult{
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) PushSettings(ctx context.Context, in *extint.PushSettingsRequest) (*extint.PushSettingsResponse, error) {
	log.Println("Received rpc request PushSettings(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_PushSettingsResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PushSettingsRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	response := <-result
	return response.GetPushSettingsResponse(), nil
}

func (m *rpcService) PullSettings(ctx context.Context, in *extint.PullSettingsRequest) (*extint.PullSettingsResponse, error) {
	log.Println("Received rpc request PullSettings(", in, ")")

	f, result := createChannel(&extint.GatewayWrapper_PullSettingsResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PullSettingsRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	response := <-result
	return response.GetPullSettingsResponse(), nil
}

func (m *rpcService) UpdateSettings(ctx context.Context, in *extint.UpdateSettingsRequest) (*extint.UpdateSettingsResponse, error) {
	log.Println("Received rpc request UpdateSettings(", in, ")")
	tag := reflect.TypeOf(&extint.GatewayWrapper_UpdateSettingsResponse{}).String()
	if _, ok := engineProtoChanMap[tag]; ok {
		return &extint.UpdateSettingsResponse{
			Code: extint.ResultCode_ERROR_UPDATE_IN_PROGRESS,
		}, nil
	}

	f, result := createChannel(&extint.GatewayWrapper_UpdateSettingsResponse{}, 1)
	defer f()

	_, err := WriteProtoToEngine(protoEngineSock, &extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_UpdateSettingsRequest{
			in,
		},
	})
	if err != nil {
		return nil, err
	}
	response := <-result
	return response.GetUpdateSettingsResponse(), nil
}

func (m *rpcService) WifiScan(ctx context.Context, in *extint.WifiScanRequest) (*extint.WifiScanResponse, error) {
	log.Println("Received rpc request WifiScan(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "WifiScan not yet implemented")
}

func (m *rpcService) WifiConnect(ctx context.Context, in *extint.WifiConnectRequest) (*extint.WifiConnectResponse, error) {
	log.Println("Received rpc request WifiConnect(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "WifiConnect not yet implemented")
}

func (m *rpcService) WifiRemove(ctx context.Context, in *extint.WifiRemoveRequest) (*extint.WifiRemoveResponse, error) {
	log.Println("Received rpc request WifiRemove(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "WifiRemove not yet implemented")
}

func (m *rpcService) WifiRemoveAll(ctx context.Context, in *extint.WifiRemoveAllRequest) (*extint.WifiRemoveAllResponse, error) {
	log.Println("Received rpc request WifiRemoveAll(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "WifiRemoveAll not yet implemented")
}

// NOTE: this is the only function that won't need to check the client_token_guid header
func (m *rpcService) UserAuthentication(ctx context.Context, in *extint.UserAuthenticationRequest) (*extint.UserAuthenticationResponse, error) {
	log.Println("Received rpc request UserAuthentication(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "UserAuthentication not yet implemented")
}

func newServer() *rpcService {
	return new(rpcService)
}
