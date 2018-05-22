package main

import (
	"bytes"
	"encoding/binary"
	"fmt"

	"anki/ipc"
	"clad/cloud"
	"proto/external_interface"

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
func ProtoDriveWheelsToClad(msg *external_interface.DriveWheelsRequest) *cloud.MessageExternalToRobot {
	return cloud.NewMessageExternalToRobotWithDriveWheels(&cloud.DriveWheels{
		Lwheel_speed_mmps:  msg.LeftWheelMmps,
		Rwheel_speed_mmps:  msg.RightWheelMmps,
		Lwheel_accel_mmps2: msg.LeftWheelMmps2,
		Rwheel_accel_mmps2: msg.RightWheelMmps2,
	})
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoPlayAnimationToClad(msg *external_interface.PlayAnimationRequest) *cloud.MessageExternalToRobot {
	if msg.Animation == nil {
		return nil
	}
	fmt.Println("Animation name:", msg.Animation.Name)
	return cloud.NewMessageExternalToRobotWithPlayAnimation(&cloud.PlayAnimation{
		NumLoops:        msg.Loops,
		AnimationName:   msg.Animation.Name,
		IgnoreBodyTrack: false,
		IgnoreHeadTrack: false,
		IgnoreLiftTrack: false,
	})
}

// The service definition.
// This must implement all the rpc functions defined in the external_interface proto file.
type rpcService struct{}

func (m *rpcService) DriveWheels(ctx context.Context, in *external_interface.DriveWheelsRequest) (*external_interface.DriveWheelsResult, error) {
	fmt.Printf("Received rpc request DriveWheels(%s)\n", in)
	_, err := WriteToEngine(engineSock, ProtoDriveWheelsToClad(in))
	if err != nil {
		return nil, err
	}
	return &external_interface.DriveWheelsResult{
		Status: &external_interface.ResultStatus{
			Description: "Message sent to engine",
		},
	}, nil
}

func (m *rpcService) PlayAnimation(ctx context.Context, in *external_interface.PlayAnimationRequest) (*external_interface.PlayAnimationResult, error) {
	fmt.Printf("Received rpc request PlayAnimation(%s)\n", in)
	animation_result := make(chan RobotToExternalResult)
	engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = animation_result
	_, err := WriteToEngine(engineSock, ProtoPlayAnimationToClad(in))
	if err != nil {
		engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = nil
		return nil, err
	}
	<-animation_result
	engineChanMap[cloud.MessageRobotToExternalTag_RobotCompletedAction] = nil
	return &external_interface.PlayAnimationResult{
		Status: &external_interface.ResultStatus{
			Description: "Animation completed",
		},
	}, nil
}

func (m *rpcService) ListAnimations(ctx context.Context, in *external_interface.ListAnimationsRequest) (*external_interface.ListAnimationsResult, error) {
	fmt.Printf("Received rpc request ListAnimations(%s)\n", in)
	return nil, status.Errorf(codes.Unimplemented, "ListAnimations not yet implemented")
}

func (m *rpcService) Test(ctx context.Context, in *external_interface.TestRequest) (*external_interface.TestResult, error) {
	fmt.Printf("Received rpc request Test(%s)\n", in)
	return nil, status.Errorf(codes.Unimplemented, "Test not yet implemented")
}

func newServer() *rpcService {
	return new(rpcService)
}
