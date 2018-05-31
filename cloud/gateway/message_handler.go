package main

import (
	"bytes"
	"encoding/binary"
	"math"

	"anki/ipc"
	"anki/log"
	"clad/cloud"
	extint "proto/external_interface"

	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func WriteToEngine(conn ipc.Conn, msg *cloud.MessageExternalToRobot) (int, error) {
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

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoDriveWheelsToClad(msg *extint.DriveWheelsRequest) *cloud.MessageExternalToRobot {
	return cloud.NewMessageExternalToRobotWithDriveWheels(&cloud.DriveWheels{
		Lwheel_speed_mmps:  msg.LeftWheelMmps,
		Rwheel_speed_mmps:  msg.RightWheelMmps,
		Lwheel_accel_mmps2: msg.LeftWheelMmps2,
		Rwheel_accel_mmps2: msg.RightWheelMmps2,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoPlayAnimationToClad(msg *extint.PlayAnimationRequest) *cloud.MessageExternalToRobot {
	if msg.Animation == nil {
		return nil
	}
	log.Println("Animation name:", msg.Animation.Name)
	return cloud.NewMessageExternalToRobotWithPlayAnimation(&cloud.PlayAnimation{
		NumLoops:        msg.Loops,
		AnimationName:   msg.Animation.Name,
		IgnoreBodyTrack: false,
		IgnoreHeadTrack: false,
		IgnoreLiftTrack: false,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoMoveHeadToClad(msg *extint.MoveHeadRequest) *cloud.MessageExternalToRobot {
	return cloud.NewMessageExternalToRobotWithMoveHead(&cloud.MoveHead{
		Speed_rad_per_sec:  msg.SpeedRadPerSec,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoMoveLiftToClad(msg *extint.MoveLiftRequest) *cloud.MessageExternalToRobot {
	return cloud.NewMessageExternalToRobotWithMoveLift(&cloud.MoveLift{
		Speed_rad_per_sec:  msg.SpeedRadPerSec,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoDriveArcToClad(msg *extint.DriveArcRequest) *cloud.MessageExternalToRobot {
	// Bind msg.CurvatureRadiusMm which is an int32 to an int16 boundary to prevent overflow
	if (msg.CurvatureRadiusMm < math.MinInt16) {
		msg.CurvatureRadiusMm = math.MinInt16
	} else if (msg.CurvatureRadiusMm > math.MaxInt16) {
		msg.CurvatureRadiusMm = math.MaxInt16
	}
	return cloud.NewMessageExternalToRobotWithDriveArc(&cloud.DriveArc{
		Speed:  msg.Speed,
		Accel: msg.Accel,
		CurvatureRadius_mm: int16(msg.CurvatureRadiusMm),
		// protobuf does not have a 16-bit integer representation, this conversion is used to satisfy the specs specified in the clad
	})
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
	engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = animation_result
	_, err := WriteToEngine(engineSock, ProtoPlayAnimationToClad(in))
	if err != nil {
		engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = nil
		return nil, err
	}
	<-animation_result
	engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = nil
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

func (m *rpcService) Test(ctx context.Context, in *extint.TestRequest) (*extint.TestResult, error) {
	log.Println("Received rpc request Test(", in, ")")
	return nil, status.Errorf(codes.Unimplemented, "Test not yet implemented")
}

func newServer() *rpcService {
	return new(rpcService)
}
