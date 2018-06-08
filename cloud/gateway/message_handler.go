package main

import (
	"bytes"
	"encoding/binary"
	"math"

	"anki/ipc"
	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

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

func ClearMapSetting(tag gw_clad.MessageRobotToExternalTag) {
	delete(engineChanMap, tag)
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoDriveWheelsToClad(msg *extint.DriveWheelsRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDriveWheels(&gw_clad.DriveWheels{
		Lwheel_speed_mmps:  msg.LeftWheelMmps,
		Rwheel_speed_mmps:  msg.RightWheelMmps,
		Lwheel_accel_mmps2: msg.LeftWheelMmps2,
		Rwheel_accel_mmps2: msg.RightWheelMmps2,
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
		Speed_rad_per_sec: msg.SpeedRadPerSec,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoMoveLiftToClad(msg *extint.MoveLiftRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithMoveLift(&gw_clad.MoveLift{
		Speed_rad_per_sec: msg.SpeedRadPerSec,
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
		Speed:              msg.Speed,
		Accel:              msg.Accel,
		CurvatureRadius_mm: int16(msg.CurvatureRadiusMm),
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
func FaceImageChunkToClad(faceData [faceImagePixelsPerChunk]uint16, pixelCount uint16, chunkIndex uint8, chunkCount uint8, duration_ms uint32, interruptRunning bool) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDisplayFaceImageRGBChunk(&gw_clad.DisplayFaceImageRGBChunk{
		FaceData:         faceData,
		NumPixels:        pixelCount,
		ChunkIndex:       chunkIndex,
		NumChunks:        chunkCount,
		Duration_ms:      duration_ms,
		InterruptRunning: interruptRunning,
	})
}

func CladFeatureStatusToProto(msg *gw_clad.FeatureStatus) *extint.FeatureStatus {
	return &extint.FeatureStatus{
		FeatureName: msg.FeatureName,
		Source:      msg.Source,
	}
}

func CladMeetVictorFaceScanStartedToProto(msg *gw_clad.MeetVictorFaceScanStarted) *extint.MeetVictorFaceScanStarted {
	return &extint.MeetVictorFaceScanStarted{}
}

func CladMeetVictorFaceScanCompleteToProto(msg *gw_clad.MeetVictorFaceScanComplete) *extint.MeetVictorFaceScanComplete {
	return &extint.MeetVictorFaceScanComplete{}
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

func CladEventToProto(msg *gw_clad.Event) *extint.EventResult {
	var event *extint.Event
	switch tag := msg.Tag(); tag {
	case gw_clad.EventTag_Status:
		event = &extint.Event{
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
	return &extint.EventResult{
		Event: event,
	}
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
	animation_result := make(chan RobotToExternalResult)
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
	return nil, status.Errorf(codes.Unimplemented, "ListAnimations not yet implemented")
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

// Long running message for sending events to listening sdk users
func (c *rpcService) EventStream(in *extint.EventRequest, stream extint.ExternalInterface_EventStreamServer) error {
	log.Println("Received rpc request EventStream(", in, ")")

	events_channel := make(chan RobotToExternalResult, 16)
	engineChanMap[gw_clad.MessageRobotToExternalTag_Event] = events_channel
	defer ClearMapSetting(gw_clad.MessageRobotToExternalTag_Event)
	log.Println(engineChanMap[gw_clad.MessageRobotToExternalTag_Event])

	for result := range events_channel {
		log.Println("Got result:", result)
		event := CladEventToProto(result.Message.GetEvent())
		log.Println("Made event:", event)
		if err := stream.Send(event); err != nil {
			return err
		} else if err = stream.Context().Err(); err != nil {
			// This is the case where the user disconnects the stream
			// We should still return the err in case the user doesn't think they disconnected
			return err
		}
	}
	return nil
}

// Request control from behavior system
func (m *rpcService) SDKBehaviorActivation(ctx context.Context, in *extint.SDKActivationRequest) (*extint.SDKActivationResult, error) {
	log.Println("Received rpc request SDKBehaviorActivation(", in, ")")
	_, err := WriteToEngine(engineSock, ProtoSDKActivationToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.SDKActivationResult{
		// TODO Set data for Slot and ResultStatus
		//Slot: &extint.Slot{
		//},
		Status: &extint.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func newServer() *rpcService {
	return new(rpcService)
}
