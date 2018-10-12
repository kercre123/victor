package main

import (
	"encoding/binary"
	"io/ioutil"
	"os"
	"os/exec"
	"reflect"
	"sync"
	"time"

	"anki/log"
	"anki/robot/loguploader"
	cloud_clad "clad/cloud"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

const faceImagePixelsPerChunk = 600

var (
	connectionIdLock sync.Mutex
	connectionId     string
)

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
	return gw_clad.NewMessageExternalToRobotWithPlayAnimation(&gw_clad.PlayAnimation{
		NumLoops:        msg.Loops,
		AnimationName:   msg.Animation.Name,
		IgnoreBodyTrack: msg.IgnoreBodyTrack,
		IgnoreHeadTrack: msg.IgnoreHeadTrack,
		IgnoreLiftTrack: msg.IgnoreLiftTrack,
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

func ProtoPoseToClad(msg *extint.PoseStruct) *gw_clad.PoseStruct3d {
	return &gw_clad.PoseStruct3d{
		X:        msg.X,
		Y:        msg.Y,
		Z:        msg.Z,
		Q0:       msg.Q0,
		Q1:       msg.Q1,
		Q2:       msg.Q2,
		Q3:       msg.Q3,
		OriginID: msg.OriginId,
	}
}

func ProtoCreateFixedCustomObjectToClad(msg *extint.CreateFixedCustomObjectRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithCreateFixedCustomObject(&gw_clad.CreateFixedCustomObject{
		Pose:    *ProtoPoseToClad(msg.Pose),
		XSizeMm: msg.XSizeMm,
		YSizeMm: msg.YSizeMm,
		ZSizeMm: msg.ZSizeMm,
	})
}

func ProtoDefineCustomBoxToClad(msg *extint.DefineCustomBoxRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDefineCustomBox(&gw_clad.DefineCustomBox{
		CustomType:     gw_clad.ObjectType(msg.CustomType - 1),
		MarkerFront:    gw_clad.CustomObjectMarker(msg.MarkerFront - 1),
		MarkerBack:     gw_clad.CustomObjectMarker(msg.MarkerBack - 1),
		MarkerTop:      gw_clad.CustomObjectMarker(msg.MarkerTop - 1),
		MarkerBottom:   gw_clad.CustomObjectMarker(msg.MarkerBottom - 1),
		MarkerLeft:     gw_clad.CustomObjectMarker(msg.MarkerLeft - 1),
		MarkerRight:    gw_clad.CustomObjectMarker(msg.MarkerRight - 1),
		XSizeMm:        msg.XSizeMm,
		YSizeMm:        msg.YSizeMm,
		ZSizeMm:        msg.ZSizeMm,
		MarkerWidthMm:  msg.MarkerWidthMm,
		MarkerHeightMm: msg.MarkerHeightMm,
		IsUnique:       msg.IsUnique,
	})
}

func ProtoDefineCustomCubeToClad(msg *extint.DefineCustomCubeRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDefineCustomCube(&gw_clad.DefineCustomCube{
		CustomType:     gw_clad.ObjectType(msg.CustomType - 1),
		Marker:         gw_clad.CustomObjectMarker(msg.Marker - 1),
		SizeMm:         msg.SizeMm,
		MarkerWidthMm:  msg.MarkerWidthMm,
		MarkerHeightMm: msg.MarkerHeightMm,
		IsUnique:       msg.IsUnique,
	})
}

func ProtoDefineCustomWallToClad(msg *extint.DefineCustomWallRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDefineCustomWall(&gw_clad.DefineCustomWall{
		CustomType:     gw_clad.ObjectType(msg.CustomType - 1),
		Marker:         gw_clad.CustomObjectMarker(msg.Marker - 1),
		WidthMm:        msg.WidthMm,
		HeightMm:       msg.HeightMm,
		MarkerWidthMm:  msg.MarkerWidthMm,
		MarkerHeightMm: msg.MarkerHeightMm,
		IsUnique:       msg.IsUnique,
	})
}

func SliceToArray(msg []uint32) [3]uint32 {
	var arr [3]uint32
	copy(arr[:], msg)
	return arr
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

func CladEventToProto(msg *gw_clad.Event) *extint.Event {
	switch tag := msg.Tag(); tag {
	// Event is currently unused in CLAD, but if you start
	// using it again, replace [MessageName] with your msg name
	// case gw_clad.EventTag_[MessageName]:
	// 	return &extint.Event{
	// 		EventType: &extint.Event_[MessageName]{
	// 			Clad[MessageName]ToProto(msg.Get[MessageName]()),
	// 		},
	// 	}
	case gw_clad.EventTag_INVALID:
		log.Println(tag, "tag is invalid")
		return nil
	default:
		log.Println(tag, "tag is not yet implemented")
		return nil
	}
}

func CladObjectConnectionStateToProto(msg *gw_clad.ObjectConnectionState) *extint.ObjectConnectionState {
	return &extint.ObjectConnectionState{
		ObjectId:   msg.ObjectID,
		FactoryId:  msg.FactoryID,
		ObjectType: extint.ObjectType(msg.ObjectType + 1),
		Connected:  msg.Connected,
	}
}
func CladObjectAvailableToProto(msg *gw_clad.ObjectAvailable) *extint.ObjectAvailable {
	return &extint.ObjectAvailable{
		FactoryId: msg.FactoryId,
	}
}
func CladObjectMovedToProto(msg *gw_clad.ObjectMoved) *extint.ObjectMoved {
	return &extint.ObjectMoved{
		Timestamp: msg.Timestamp,
		ObjectId:  msg.ObjectID,
	}
}
func CladObjectStoppedMovingToProto(msg *gw_clad.ObjectStoppedMoving) *extint.ObjectStoppedMoving {
	return &extint.ObjectStoppedMoving{
		Timestamp: msg.Timestamp,
		ObjectId:  msg.ObjectID,
	}
}
func CladObjectUpAxisChangedToProto(msg *gw_clad.ObjectUpAxisChanged) *extint.ObjectUpAxisChanged {
	// In clad, unknown is the final value
	// In proto, the convention is that 0 is unknown
	upAxis := extint.UpAxis_INVALID_AXIS
	if msg.UpAxis != gw_clad.UpAxis_UnknownAxis {
		upAxis = extint.UpAxis(msg.UpAxis + 1)
	}

	return &extint.ObjectUpAxisChanged{
		Timestamp: msg.Timestamp,
		ObjectId:  msg.ObjectID,
		UpAxis:    upAxis,
	}
}
func CladObjectTappedToProto(msg *gw_clad.ObjectTapped) *extint.ObjectTapped {
	return &extint.ObjectTapped{
		Timestamp: msg.Timestamp,
		ObjectId:  msg.ObjectID,
	}
}

func CladRobotObservedObjectToProto(msg *gw_clad.RobotObservedObject) *extint.RobotObservedObject {
	return &extint.RobotObservedObject{
		Timestamp:             msg.Timestamp,
		ObjectFamily:          extint.ObjectFamily(msg.ObjectFamily + 1),
		ObjectType:            extint.ObjectType(msg.ObjectType + 1),
		ObjectId:              msg.ObjectID,
		ImgRect:               CladCladRectToProto(&msg.ImgRect),
		Pose:                  CladPoseToProto(&msg.Pose),
		IsActive:              uint32(msg.IsActive),
		TopFaceOrientationRad: msg.TopFaceOrientationRad,
	}
}

func SendOnboardingComplete(in *extint.GatewayWrapper_OnboardingCompleteRequest) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingCompleteResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	completeResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingCompleteResponse{
			OnboardingCompleteResponse: completeResponse.GetOnboardingCompleteResponse(),
		},
	}, nil
}

func SendOnboardingWakeUpStarted(in *extint.GatewayWrapper_OnboardingWakeUpStartedRequest) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingWakeUpStartedResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	completeResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingWakeUpStartedResponse{
			OnboardingWakeUpStartedResponse: completeResponse.GetOnboardingWakeUpStartedResponse(),
		},
	}, nil
}

func SendOnboardingWakeUp(in *extint.GatewayWrapper_OnboardingWakeUpRequest) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingWakeUpResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	wakeUpResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingWakeUpResponse{
			OnboardingWakeUpResponse: wakeUpResponse.GetOnboardingWakeUpResponse(),
		},
	}, nil
}

func SendOnboardingContinue(in *extint.GatewayWrapper_OnboardingContinue) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingContinueResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	continueResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingContinueResponse{
			OnboardingContinueResponse: continueResponse.GetOnboardingContinueResponse(),
		},
	}, nil
}

func SendOnboardingGetStep(in *extint.GatewayWrapper_OnboardingGetStep) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingStepResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	select {
	case getStepResponse, ok := <-responseChan:
		if !ok {
			return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
		}
		return &extint.OnboardingInputResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_REQUEST_PROCESSING,
			},
			OneofMessageType: &extint.OnboardingInputResponse_OnboardingStepResponse{
				OnboardingStepResponse: getStepResponse.GetOnboardingStepResponse(),
			},
		}, nil
	case <-time.After(10 * time.Second):
		return &extint.OnboardingInputResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_NOT_FOUND,
			},
			OneofMessageType: &extint.OnboardingInputResponse_OnboardingStepResponse{
				OnboardingStepResponse: &extint.OnboardingStepResponse{
					StepNumber: extint.OnboardingSteps_STEP_INVALID,
				},
			},
		}, nil
	}
}

func SendOnboardingSkip(in *extint.GatewayWrapper_OnboardingSkip) (*extint.OnboardingInputResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func SendOnboardingSkipOnboarding(in *extint.GatewayWrapper_OnboardingSkipOnboarding) (*extint.OnboardingInputResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func SendOnboardingRestart(in *extint.GatewayWrapper_OnboardingRestart) (*extint.OnboardingInputResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

// The service definition.
// This must implement all the rpc functions defined in the external_interface proto file.
type rpcService struct{}

func (service *rpcService) ProtocolVersion(ctx context.Context, in *extint.ProtocolVersionRequest) (*extint.ProtocolVersionResponse, error) {
	return &extint.ProtocolVersionResponse{
		Result: extint.ProtocolVersionResponse_SUCCESS,
	}, nil
}

func (service *rpcService) DriveWheels(ctx context.Context, in *extint.DriveWheelsRequest) (*extint.DriveWheelsResponse, error) {
	_, err := engineCladManager.Write(ProtoDriveWheelsToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.DriveWheelsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) PlayAnimation(ctx context.Context, in *extint.PlayAnimationRequest) (*extint.PlayAnimationResponse, error) {
	f, animResponseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PlayAnimationResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoPlayAnimationToClad(in))
	if err != nil {
		return nil, err
	}

	setPlayAnimationResponse, ok := <-animResponseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := setPlayAnimationResponse.GetPlayAnimationResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) ListAnimations(ctx context.Context, in *extint.ListAnimationsRequest) (*extint.ListAnimationsResponse, error) {
	// 50 messages are sent per engine tick, so this channel is set to read 50 at a time
	f1, animationAvailableResponse := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_AnimationAvailable, 50)
	defer f1()

	f2, endOfMessageResponse := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_EndOfMessage, 1)
	defer f2()

	_, err := engineCladManager.Write(ProtoListAnimationsToClad(in))
	if err != nil {
		return nil, err
	}

	var anims []*extint.Animation

	done := false
	remaining := -1
	for done == false || remaining != 0 {
		select {
		case chanResponse, ok := <-animationAvailableResponse:
			if !ok {
				return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
			}
			var newAnim = extint.Animation{
				Name: chanResponse.GetAnimationAvailable().AnimName,
			}
			anims = append(anims, &newAnim)
			remaining = remaining - 1
		case chanResponse, ok := <-endOfMessageResponse:
			if !ok {
				return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
			}
			if chanResponse.GetEndOfMessage().MessageType == gw_clad.MessageType_AnimationAvailable {
				remaining = len(animationAvailableResponse)
				done = true
			}
		case <-time.After(5 * time.Second):
			return nil, grpc.Errorf(codes.DeadlineExceeded, "ListAnimations request timed out")
		}
	}

	return &extint.ListAnimationsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		AnimationNames: anims,
	}, nil
}

func (service *rpcService) MoveHead(ctx context.Context, in *extint.MoveHeadRequest) (*extint.MoveHeadResponse, error) {
	_, err := engineCladManager.Write(ProtoMoveHeadToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.MoveHeadResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) MoveLift(ctx context.Context, in *extint.MoveLiftRequest) (*extint.MoveLiftResponse, error) {
	_, err := engineCladManager.Write(ProtoMoveLiftToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.MoveLiftResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
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
		slicedBinaryData := in.FaceData[firstByte:finalByte] // TODO: Make this not implode on empty

		for j := 0; j < pixelCount; j++ {
			uintAsBytes := slicedBinaryData[j*2 : j*2+2]
			convertedUint16Data[j] = binary.BigEndian.Uint16(uintAsBytes)
		}

		// Copy a subset of the pixels to the bytes?
		message := FaceImageChunkToClad(convertedUint16Data, uint16(pixelCount), uint8(i), uint8(chunkCount), in.DurationMs, in.InterruptRunning)

		_, err := engineCladManager.Write(message)
		if err != nil {
			return err
		}
	}

	return nil
}

func (service *rpcService) DisplayFaceImageRGB(ctx context.Context, in *extint.DisplayFaceImageRGBRequest) (*extint.DisplayFaceImageRGBResponse, error) {
	const totalPixels = 17664
	chunkCount := (totalPixels + faceImagePixelsPerChunk + 1) / faceImagePixelsPerChunk

	SendFaceDataAsChunks(in, chunkCount, faceImagePixelsPerChunk, totalPixels)

	return &extint.DisplayFaceImageRGBResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) AppIntent(ctx context.Context, in *extint.AppIntentRequest) (*extint.AppIntentResponse, error) {
	_, err := engineCladManager.Write(ProtoAppIntentToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.AppIntentResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) CancelFaceEnrollment(ctx context.Context, in *extint.CancelFaceEnrollmentRequest) (*extint.CancelFaceEnrollmentResponse, error) {
	_, err := engineCladManager.Write(ProtoCancelFaceEnrollmentToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.CancelFaceEnrollmentResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) RequestEnrolledNames(ctx context.Context, in *extint.RequestEnrolledNamesRequest) (*extint.RequestEnrolledNamesResponse, error) {
	f, enrolledNamesResponse := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_EnrolledNamesResponse, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoRequestEnrolledNamesToClad(in))
	if err != nil {
		return nil, err
	}
	names, ok := <-enrolledNamesResponse
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	var faces []*extint.LoadedKnownFace
	for _, element := range names.GetEnrolledNamesResponse().Faces {
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
	return &extint.RequestEnrolledNamesResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Faces: faces,
	}, nil
}

// TODO Wait for response RobotRenamedEnrolledFace
func (service *rpcService) UpdateEnrolledFaceByID(ctx context.Context, in *extint.UpdateEnrolledFaceByIDRequest) (*extint.UpdateEnrolledFaceByIDResponse, error) {
	_, err := engineCladManager.Write(ProtoUpdateEnrolledFaceByIDToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.UpdateEnrolledFaceByIDResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

// TODO Wait for response RobotRenamedEnrolledFace
func (service *rpcService) EraseEnrolledFaceByID(ctx context.Context, in *extint.EraseEnrolledFaceByIDRequest) (*extint.EraseEnrolledFaceByIDResponse, error) {
	_, err := engineCladManager.Write(ProtoEraseEnrolledFaceByIDToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EraseEnrolledFaceByIDResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

// TODO Wait for response RobotErasedAllEnrolledFaces
func (service *rpcService) EraseAllEnrolledFaces(ctx context.Context, in *extint.EraseAllEnrolledFacesRequest) (*extint.EraseAllEnrolledFacesResponse, error) {
	_, err := engineCladManager.Write(ProtoEraseAllEnrolledFacesToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EraseAllEnrolledFacesResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) SetFaceToEnroll(ctx context.Context, in *extint.SetFaceToEnrollRequest) (*extint.SetFaceToEnrollResponse, error) {
	_, err := engineCladManager.Write(ProtoSetFaceToEnrollToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.SetFaceToEnrollResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func isMember(needle string, haystack []string) bool {
	for _, word := range haystack {
		if word == needle {
			return true
		}
	}
	return false
}

func checkFilters(event *extint.Event, whiteList, blackList *extint.FilterList) bool {
	if whiteList == nil && blackList == nil {
		return true
	}
	props := &proto.Properties{}
	val := reflect.ValueOf(event.GetEventType())
	if val.Kind() == reflect.Ptr {
		val = val.Elem()
	}
	props.Parse(val.Type().Field(0).Tag.Get("protobuf"))
	responseType := props.OrigName

	if whiteList != nil && isMember(responseType, whiteList.List) {
		return true
	}
	if blackList != nil && !isMember(responseType, blackList.List) {
		return true
	}
	return false
}

// Should be called on WiFi connect.
func (service *rpcService) onConnect(id string) {
	// Call DAS WiFi connection event to indicate start of a WiFi connection.
	// Log the connection id for the primary connection, which is the first person to connect.
	log.Das("wifi_conn_id.start", (&log.DasFields{}).SetStrings(id))
}

// Should be called on WiFi disconnect.
func (service *rpcService) onDisconnect() {
	// Call DAS WiFi connection event to indicate stop of a WiFi connection
	log.Das("wifi_conn_id.stop", (&log.DasFields{}).SetStrings(""))
	connectionId = ""
}

func (service *rpcService) checkConnectionID(id string) bool {
	connectionIdLock.Lock()
	defer connectionIdLock.Unlock()
	if len(connectionId) != 0 && id != connectionId {
		log.Println("Connection id already set: current='%s', incoming='%s'", connectionId, id)
		return false
	}
	// Check whether we are in Webots.
	if IsOnRobot {
		f, responseChan := switchboardManager.CreateChannel(gw_clad.SwitchboardResponseTag_ExternalConnectionResponse, 1)
		defer f()
		switchboardManager.Write(gw_clad.NewSwitchboardRequestWithExternalConnectionRequest(&gw_clad.ExternalConnectionRequest{}))

		response, ok := <-responseChan
		if !ok {
			log.Println("Failed to receive ConnectionID response from vic-switchboard")
			return false
		}
		connectionResponse := response.GetExternalConnectionResponse()

		// IsConnected shows whether switchboard is connected over ble.
		// Detect if someone else is connected.
		if connectionResponse.IsConnected && connectionResponse.ConnectionId != id {
			// Someone is connected over BLE and they are not the primary connection.
			// We return false so the app can tell you not to connect.
			log.Printf("Detected mismatched BLE connection id: BLE='%s', incoming='%s'\n", connectionResponse.ConnectionId, id)
			return false
		}
	}
	connectionId = id
	return true
}

// Long running message for sending events to listening sdk users
func (service *rpcService) EventStream(in *extint.EventRequest, stream extint.ExternalInterface_EventStreamServer) error {
	// TODO: v Remove the tempEventStream handling below when the app connection properly closes v
	tempEventStreamMutex1.Lock()
	if tempEventStreamDone != nil {
		close(tempEventStreamDone)
	}
	tempEventStreamMutex2.Lock()
	defer tempEventStreamMutex2.Unlock()
	tempEventStreamDone = make(chan struct{})
	tempEventStreamMutex1.Unlock()
	// TODO: ^ Remove the tempEventStream handling above when the app connection properly closes ^

	isPrimary := service.checkConnectionID(in.ConnectionId)
	if isPrimary {
		service.onConnect(connectionId)
		defer service.onDisconnect()
	}
	resp := &extint.EventResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Event: &extint.Event{
			EventType: &extint.Event_ConnectionResponse{
				ConnectionResponse: &extint.ConnectionResponse{
					IsPrimary: isPrimary,
				},
			},
		},
	}
	err := stream.Send(resp)
	if err != nil {
		log.Println("Closing Event stream (on send):", err)
		return err
	} else if err = stream.Context().Err(); err != nil {
		log.Println("Closing Event stream:", err)
		// This is the case where the user disconnects the stream
		// We should still return the err in case the user doesn't think they disconnected
		return err
	}

	if isPrimary {
		log.Printf("EventStream: Sent primary connection response '%s'\n", connectionId)
	} else {
		log.Printf("EventStream: Sent secondary connection response given='%s', current='%s'\n", in.ConnectionId, connectionId)
	}

	f, eventsChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_Event{}, 512)
	defer f()

	ping := extint.EventResponse{
		Event: &extint.Event{
			EventType: &extint.Event_KeepAlive{
				KeepAlive: &extint.KeepAlivePing{},
			},
		},
	}

	whiteList := in.GetWhiteList()
	blackList := in.GetBlackList()

	pingTicker := time.Tick(time.Second)

	for {
		select {
		// TODO: remove entire tempEventStreamDone case when the app connection properly closes
		case <-tempEventStreamDone:
			log.Println("EventStream closing because another stream has opened")
			return grpc.Errorf(codes.Unavailable, "Connection closed because another event stream has opened")
		case response, ok := <-eventsChannel:
			if !ok {
				return grpc.Errorf(codes.Internal, "EventStream: event channel closed")
			}
			event := response.GetEvent()
			if checkFilters(event, whiteList, blackList) {
				if logVerbose {
					log.Printf("EventStream: Sending event to client: %#v\n", *event)
				}
				eventResponse := &extint.EventResponse{
					Event: event,
				}
				if err := stream.Send(eventResponse); err != nil {
					log.Println("Closing Event stream (on send):", err)
					return err
				} else if err = stream.Context().Err(); err != nil {
					log.Println("Closing Event stream:", err)
					// This is the case where the user disconnects the stream
					// We should still return the err in case the user doesn't think they disconnected
					return err
				}
			}
		case <-pingTicker: // ping to check connection liveness after one second.
			if err := stream.Send(&ping); err != nil {
				log.Println("Closing Event stream (on send):", err)
				return err
			} else if err = stream.Context().Err(); err != nil {
				log.Println("Closing Event stream:", err)
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				return err
			}
		}
	}
	return nil
}

func (service *rpcService) BehaviorRequestToGatewayWrapper(request *extint.BehaviorControlRequest) (*extint.GatewayWrapper, error) {
	msg := &extint.GatewayWrapper{}
	switch x := request.RequestType.(type) {
	case *extint.BehaviorControlRequest_ControlRelease:
		msg.OneofMessageType = &extint.GatewayWrapper_ControlRelease{
			ControlRelease: request.GetControlRelease(),
		}
	case *extint.BehaviorControlRequest_ControlRequest:
		msg.OneofMessageType = &extint.GatewayWrapper_ControlRequest{
			ControlRequest: request.GetControlRequest(),
		}
	default:
		return nil, grpc.Errorf(codes.InvalidArgument, "BehaviorControlRequest.ControlRequest has unexpected type %T", x)
	}
	return msg, nil
}

func (service *rpcService) BehaviorControlRequestHandler(in extint.ExternalInterface_BehaviorControlServer, done chan struct{}) {
	defer close(done)
	defer engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ControlRelease{
			ControlRelease: &extint.ControlRelease{},
		},
	})
	for {
		request, err := in.Recv()
		if err != nil {
			log.Printf("BehaviorControlRequestHandler.close: %s\n", err.Error())
			return
		}
		log.Println("BehaviorControl Incoming Request:", request)

		msg, err := service.BehaviorRequestToGatewayWrapper(request)
		if err != nil {
			log.Println(err)
			return
		}
		_, err = engineProtoManager.Write(msg)
		if err != nil {
			return
		}
	}
}

func (service *rpcService) BehaviorControlResponseHandler(out extint.ExternalInterface_AssumeBehaviorControlServer, responses chan extint.GatewayWrapper, done chan struct{}) error {
	ping := extint.BehaviorControlResponse{
		ResponseType: &extint.BehaviorControlResponse_KeepAlive{
			KeepAlive: &extint.KeepAlivePing{},
		},
	}

	pingTicker := time.Tick(time.Second)

	for {
		select {
		case <-done:
			return nil
		case response, ok := <-responses:
			if !ok {
				return grpc.Errorf(codes.Internal, "Failed to retrieve message")
			}
			msg := response.GetBehaviorControlResponse()
			if err := out.Send(msg); err != nil {
				log.Println("Closing BehaviorControl stream (on send):", err)
				return err
			} else if err = out.Context().Err(); err != nil {
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				log.Println("Closing BehaviorControl stream:", err)
				return err
			}
		case <-pingTicker: // ping to check connection liveness after one second.
			if err := out.Send(&ping); err != nil {
				log.Println("Closing BehaviorControl stream (on send):", err)
				return err
			} else if err = out.Context().Err(); err != nil {
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				log.Println("Closing BehaviorControl stream:", err)
				return err
			}
		}
	}
	return nil
}

func (service *rpcService) BehaviorControl(bidirectionalStream extint.ExternalInterface_BehaviorControlServer) error {
	if disableStreams {
		// Disabled for Vector 1.0 release
		return grpc.Errorf(codes.Unimplemented, "BehaviorControl disabled in message_handler.go")
	}

	done := make(chan struct{})

	f, behaviorStatus := engineProtoManager.CreateChannel(&extint.GatewayWrapper_BehaviorControlResponse{}, 1)
	defer f()

	go service.BehaviorControlRequestHandler(bidirectionalStream, done)
	return service.BehaviorControlResponseHandler(bidirectionalStream, behaviorStatus, done)
}

func (service *rpcService) AssumeBehaviorControl(in *extint.BehaviorControlRequest, out extint.ExternalInterface_AssumeBehaviorControlServer) error {
	if disableStreams {
		// Disabled for Vector 1.0 release
		return grpc.Errorf(codes.Unimplemented, "AssumeBehaviorControl disabled in message_handler.go")
	}

	done := make(chan struct{})

	f, behaviorStatus := engineProtoManager.CreateChannel(&extint.GatewayWrapper_BehaviorControlResponse{}, 1)
	defer f()

	msg, err := service.BehaviorRequestToGatewayWrapper(in)
	if err != nil {
		return err
	}

	_, err = engineProtoManager.Write(msg)
	if err != nil {
		return err
	}

	return service.BehaviorControlResponseHandler(out, behaviorStatus, done)
}

func (service *rpcService) DriveOffCharger(ctx context.Context, in *extint.DriveOffChargerRequest) (*extint.DriveOffChargerResponse, error) {
	if disableStreams {
		// Disable for Vector 1.0 release
		return nil, grpc.Errorf(codes.Unimplemented, "DriveOffCharger disabled in message_handler.go")
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DriveOffChargerResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DriveOffChargerRequest{
			DriveOffChargerRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	driveOffChargerResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := driveOffChargerResponse.GetDriveOffChargerResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) DriveOnCharger(ctx context.Context, in *extint.DriveOnChargerRequest) (*extint.DriveOnChargerResponse, error) {
	if disableStreams {
		// Disabled for Vector 1.0 release
		return nil, grpc.Errorf(codes.Unimplemented, "DriveOnCharger disabled in message_handler.go")
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DriveOnChargerResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DriveOnChargerRequest{
			DriveOnChargerRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	driveOnChargerResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := driveOnChargerResponse.GetDriveOnChargerResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

// Request the current robot onboarding status
func (service *rpcService) GetOnboardingState(ctx context.Context, in *extint.OnboardingStateRequest) (*extint.OnboardingStateResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingState{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_OnboardingStateRequest{
			OnboardingStateRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	onboardingState, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.OnboardingStateResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		OnboardingState: onboardingState.GetOnboardingState(),
	}, nil
}

func (service *rpcService) SendOnboardingInput(ctx context.Context, in *extint.OnboardingInputRequest) (*extint.OnboardingInputResponse, error) {
	// oneof_message_type
	switch x := in.OneofMessageType.(type) {
	case *extint.OnboardingInputRequest_OnboardingCompleteRequest:
		return SendOnboardingComplete(&extint.GatewayWrapper_OnboardingCompleteRequest{
			OnboardingCompleteRequest: in.GetOnboardingCompleteRequest(),
		})
	case *extint.OnboardingInputRequest_OnboardingWakeUpStartedRequest:
		return SendOnboardingWakeUpStarted(&extint.GatewayWrapper_OnboardingWakeUpStartedRequest{
			OnboardingWakeUpStartedRequest: in.GetOnboardingWakeUpStartedRequest(),
		})
	case *extint.OnboardingInputRequest_OnboardingWakeUpRequest:
		return SendOnboardingWakeUp(&extint.GatewayWrapper_OnboardingWakeUpRequest{
			OnboardingWakeUpRequest: in.GetOnboardingWakeUpRequest(),
		})
	case *extint.OnboardingInputRequest_OnboardingContinue:
		return SendOnboardingContinue(&extint.GatewayWrapper_OnboardingContinue{
			OnboardingContinue: in.GetOnboardingContinue(),
		})
	case *extint.OnboardingInputRequest_OnboardingSkip:
		return SendOnboardingSkip(&extint.GatewayWrapper_OnboardingSkip{
			OnboardingSkip: in.GetOnboardingSkip(),
		})
	case *extint.OnboardingInputRequest_OnboardingSkipOnboarding:
		return SendOnboardingSkipOnboarding(&extint.GatewayWrapper_OnboardingSkipOnboarding{
			OnboardingSkipOnboarding: in.GetOnboardingSkipOnboarding(),
		})
	case *extint.OnboardingInputRequest_OnboardingRestart:
		return SendOnboardingRestart(&extint.GatewayWrapper_OnboardingRestart{
			OnboardingRestart: in.GetOnboardingRestart(),
		})
	case *extint.OnboardingInputRequest_OnboardingGetStep:
		return SendOnboardingGetStep(&extint.GatewayWrapper_OnboardingGetStep{
			OnboardingGetStep: in.GetOnboardingGetStep(),
		})
	default:
		return nil, grpc.Errorf(codes.InvalidArgument, "OnboardingInputRequest.OneofMessageType has unexpected type %T", x)
	}
}

func (service *rpcService) PhotosInfo(ctx context.Context, in *extint.PhotosInfoRequest) (*extint.PhotosInfoResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PhotosInfoResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PhotosInfoRequest{
			PhotosInfoRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	infoResponse := payload.GetPhotosInfoResponse()
	infoResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return infoResponse, nil
}

func SendImageHelper(fullpath string) ([]byte, error) {
	log.Println("Reading file at", fullpath)
	dat, err := ioutil.ReadFile(fullpath)
	if err != nil {
		log.Println("Error reading file ", fullpath)
		return nil, err
	}
	return dat, nil
}

func (service *rpcService) Photo(ctx context.Context, in *extint.PhotoRequest) (*extint.PhotoResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PhotoPathMessage{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PhotoRequest{
			PhotoRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	if !payload.GetPhotoPathMessage().GetSuccess() {
		return &extint.PhotoResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_NOT_FOUND,
			},
			Success: false,
		}, err
	}
	imageData, err := SendImageHelper(payload.GetPhotoPathMessage().GetFullPath())
	if err != nil {
		return &extint.PhotoResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_NOT_FOUND,
			},
			Success: false,
		}, err
	}
	return &extint.PhotoResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Success: true,
		Image:   imageData,
	}, err
}

func (service *rpcService) Thumbnail(ctx context.Context, in *extint.ThumbnailRequest) (*extint.ThumbnailResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_ThumbnailPathMessage{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ThumbnailRequest{
			ThumbnailRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	if !payload.GetThumbnailPathMessage().GetSuccess() {
		return &extint.ThumbnailResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_NOT_FOUND,
			},
			Success: false,
		}, err
	}
	imageData, err := SendImageHelper(payload.GetThumbnailPathMessage().GetFullPath())
	if err != nil {
		return &extint.ThumbnailResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_NOT_FOUND,
			},
			Success: false,
		}, err
	}
	return &extint.ThumbnailResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Success: true,
		Image:   imageData,
	}, err
}

func (service *rpcService) DeletePhoto(ctx context.Context, in *extint.DeletePhotoRequest) (*extint.DeletePhotoResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DeletePhotoResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DeletePhotoRequest{
			DeletePhotoRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	photoResponse := payload.GetDeletePhotoResponse()
	photoResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return photoResponse, nil
}

func (service *rpcService) GetLatestAttentionTransfer(ctx context.Context, in *extint.LatestAttentionTransferRequest) (*extint.LatestAttentionTransferResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_LatestAttentionTransfer{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_LatestAttentionTransferRequest{
			LatestAttentionTransferRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	attentionTransfer, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return &extint.LatestAttentionTransferResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		LatestAttentionTransfer: attentionTransfer.GetLatestAttentionTransfer(),
	}, nil
}

func (service *rpcService) EnableVisionMode(ctx context.Context, in *extint.EnableVisionModeRequest) (*extint.EnableVisionModeResponse, error) {
	_, err := engineCladManager.Write(ProtoEnableVisionModeToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.EnableVisionModeResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) ConnectCube(ctx context.Context, in *extint.ConnectCubeRequest) (*extint.ConnectCubeResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_ConnectCubeResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ConnectCubeRequest{
			ConnectCubeRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	gatewayWrapper, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := gatewayWrapper.GetConnectCubeResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) DisconnectCube(ctx context.Context, in *extint.DisconnectCubeRequest) (*extint.DisconnectCubeResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DisconnectCubeRequest{
			DisconnectCubeRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	return &extint.DisconnectCubeResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) CubesAvailable(ctx context.Context, in *extint.CubesAvailableRequest) (*extint.CubesAvailableResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_CubesAvailableResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_CubesAvailableRequest{
			CubesAvailableRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	cubesAvailable, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := cubesAvailable.GetCubesAvailableResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) FlashCubeLights(ctx context.Context, in *extint.FlashCubeLightsRequest) (*extint.FlashCubeLightsResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_FlashCubeLightsRequest{
			FlashCubeLightsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	return &extint.FlashCubeLightsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) ForgetPreferredCube(ctx context.Context, in *extint.ForgetPreferredCubeRequest) (*extint.ForgetPreferredCubeResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ForgetPreferredCubeRequest{
			ForgetPreferredCubeRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	return &extint.ForgetPreferredCubeResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) SetPreferredCube(ctx context.Context, in *extint.SetPreferredCubeRequest) (*extint.SetPreferredCubeResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_SetPreferredCubeRequest{
			SetPreferredCubeRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	return &extint.SetPreferredCubeResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (service *rpcService) SetCubeLights(ctx context.Context, in *extint.SetCubeLightsRequest) (*extint.SetCubeLightsResponse, error) {
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_SetCubeLightsRequest{
			SetCubeLightsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	return &extint.SetCubeLightsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

// NOTE: this is removed from external_interface because the result from the robot (size 100) is too large for the domain socket between gateway and engine
func (service *rpcService) RobotStatusHistory(ctx context.Context, in *extint.RobotHistoryRequest) (*extint.RobotHistoryResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_RobotHistoryResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_RobotHistoryRequest{
			RobotHistoryRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return response.GetRobotHistoryResponse(), nil
}

func (service *rpcService) PullJdocs(ctx context.Context, in *extint.PullJdocsRequest) (*extint.PullJdocsResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PullJdocsResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PullJdocsRequest{
			PullJdocsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return response.GetPullJdocsResponse(), nil
}

func (service *rpcService) UpdateSettings(ctx context.Context, in *extint.UpdateSettingsRequest) (*extint.UpdateSettingsResponse, error) {
	f, responseChan, ok := engineProtoManager.CreateUniqueChannel(&extint.GatewayWrapper_UpdateSettingsResponse{}, 1)
	if !ok {
		return &extint.UpdateSettingsResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_ERROR_UPDATE_IN_PROGRESS,
			},
		}, nil
	}
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_UpdateSettingsRequest{
			UpdateSettingsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return response.GetUpdateSettingsResponse(), nil
}

func (service *rpcService) UpdateAccountSettings(ctx context.Context, in *extint.UpdateAccountSettingsRequest) (*extint.UpdateAccountSettingsResponse, error) {
	f, responseChan, ok := engineProtoManager.CreateUniqueChannel(&extint.GatewayWrapper_UpdateAccountSettingsResponse{}, 1)
	if !ok {
		return &extint.UpdateAccountSettingsResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_ERROR_UPDATE_IN_PROGRESS,
			},
		}, nil
	}
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_UpdateAccountSettingsRequest{
			UpdateAccountSettingsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return response.GetUpdateAccountSettingsResponse(), nil
}

func (service *rpcService) UpdateUserEntitlements(ctx context.Context, in *extint.UpdateUserEntitlementsRequest) (*extint.UpdateUserEntitlementsResponse, error) {
	f, responseChan, ok := engineProtoManager.CreateUniqueChannel(&extint.GatewayWrapper_UpdateUserEntitlementsResponse{}, 1)
	if !ok {
		return &extint.UpdateUserEntitlementsResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_ERROR_UPDATE_IN_PROGRESS,
			},
		}, nil
	}
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_UpdateUserEntitlementsRequest{
			UpdateUserEntitlementsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	return response.GetUpdateUserEntitlementsResponse(), nil
}

// NOTE: this is the only function that won't need to check the client_token_guid header
func (service *rpcService) UserAuthentication(ctx context.Context, in *extint.UserAuthenticationRequest) (*extint.UserAuthenticationResponse, error) {
	if !IsOnRobot {
		return nil, grpc.Errorf(codes.Internal, "User authentication is only available on the robot")
	}

	f, authChan := switchboardManager.CreateChannel(gw_clad.SwitchboardResponseTag_AuthResponse, 1)
	defer f()

	switchboardManager.Write(gw_clad.NewSwitchboardRequestWithAuthRequest(&cloud_clad.AuthRequest{
		SessionToken: string(in.UserSessionId),
	}))
	response, ok := <-authChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	auth := response.GetAuthResponse()
	code := extint.UserAuthenticationResponse_UNAUTHORIZED
	token := auth.AppToken
	if auth.Error == cloud_clad.TokenError_NoError {
		code = extint.UserAuthenticationResponse_AUTHORIZED

		// Force an update of the tokens
		response := make(chan struct{})
		tokenManager.ForceUpdate(response)
		<-response
		log.Das("sdk.activate", &log.DasFields{})
	} else {
		token = ""
	}
	return &extint.UserAuthenticationResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Code:            code,
		ClientTokenGuid: []byte(token),
	}, nil
}

func ValidateActionTag(idTag int32) error {
	firstTag := int32(extint.ActionTagConstants_FIRST_SDK_TAG)
	lastTag := int32(extint.ActionTagConstants_LAST_SDK_TAG)
	if idTag < firstTag || idTag > lastTag {
		return grpc.Errorf(codes.InvalidArgument, "Invalid Action tag_id")
	}

	return nil
}

func (service *rpcService) GoToPose(ctx context.Context, in *extint.GoToPoseRequest) (*extint.GoToPoseResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_GoToPoseResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_GoToPoseRequest{
			GoToPoseRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	goToPoseResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := goToPoseResponse.GetGoToPoseResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	log.Printf("Received rpc response GoToPose(%#v)\n", in)
	return response, nil
}

func (service *rpcService) DockWithCube(ctx context.Context, in *extint.DockWithCubeRequest) (*extint.DockWithCubeResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DockWithCubeResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DockWithCubeRequest{
			DockWithCubeRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	dockWithCubeResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := dockWithCubeResponse.GetDockWithCubeResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) DriveStraight(ctx context.Context, in *extint.DriveStraightRequest) (*extint.DriveStraightResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DriveStraightResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_DriveStraightRequest{
			DriveStraightRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	driveStraightResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := driveStraightResponse.GetDriveStraightResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) TurnInPlace(ctx context.Context, in *extint.TurnInPlaceRequest) (*extint.TurnInPlaceResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_TurnInPlaceResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_TurnInPlaceRequest{
			TurnInPlaceRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	turnInPlaceResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := turnInPlaceResponse.GetTurnInPlaceResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) SetHeadAngle(ctx context.Context, in *extint.SetHeadAngleRequest) (*extint.SetHeadAngleResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_SetHeadAngleResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_SetHeadAngleRequest{
			SetHeadAngleRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	setHeadAngleResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := setHeadAngleResponse.GetSetHeadAngleResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) SetLiftHeight(ctx context.Context, in *extint.SetLiftHeightRequest) (*extint.SetLiftHeightResponse, error) {

	if err := ValidateActionTag(in.IdTag); err != nil {
		return nil, err
	}

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_SetLiftHeightResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_SetLiftHeightRequest{
			SetLiftHeightRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}

	setLiftHeightResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := setLiftHeightResponse.GetSetLiftHeightResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (service *rpcService) BatteryState(ctx context.Context, in *extint.BatteryStateRequest) (*extint.BatteryStateResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_BatteryStateResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_BatteryStateRequest{
			BatteryStateRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	payload.GetBatteryStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetBatteryStateResponse(), nil
}

func (service *rpcService) VersionState(ctx context.Context, in *extint.VersionStateRequest) (*extint.VersionStateResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_VersionStateResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_VersionStateRequest{
			VersionStateRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	payload.GetVersionStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetVersionStateResponse(), nil
}

func (service *rpcService) NetworkState(ctx context.Context, in *extint.NetworkStateRequest) (*extint.NetworkStateResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_NetworkStateResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_NetworkStateRequest{
			NetworkStateRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	payload.GetNetworkStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetNetworkStateResponse(), nil
}

func (service *rpcService) SayText(ctx context.Context, in *extint.SayTextRequest) (*extint.SayTextResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_SayTextResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_SayTextRequest{
			SayTextRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	sayTextResponse := payload.GetSayTextResponse()
	sayTextResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return sayTextResponse, nil
}

func AudioSendModeRequest(mode extint.AudioProcessingMode) error {
	log.Println("SDK Requesting Audio with mode(", mode, ")")

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_AudioSendModeRequest{
			AudioSendModeRequest: &extint.AudioSendModeRequest{
				Mode: mode,
			},
		},
	})

	return err
}

type AudioFeedCache struct {
	Data        []byte
	GroupId     int32
	Invalid     bool
	Size        int32
	LastChunkId int32
}

func ResetAudioCache(cache *AudioFeedCache) error {
	cache.Data = nil
	cache.GroupId = -1
	cache.Invalid = false
	cache.Size = 0
	cache.LastChunkId = -1
	return nil
}

func UnpackAudioChunk(audioChunk *extint.AudioChunk, cache *AudioFeedCache) bool {
	groupId := int32(audioChunk.GetGroupId())
	chunkId := int32(audioChunk.GetChunkId())

	if cache.GroupId != -1 && chunkId == 0 {
		if !cache.Invalid {
			log.Errorln("Lost final chunk of audio group; discarding")
		}
		cache.GroupId = -1
	}

	if cache.GroupId == -1 {
		if chunkId != 0 {
			if !cache.Invalid {
				log.Errorln("Received chunk of broken audio stream")
			}
			cache.Invalid = true
			return false
		}
		// discard any previous in-progress image
		ResetAudioCache(cache)

		cache.Data = make([]byte, extint.AudioConstants_SAMPLE_COUNTS_PER_SDK_MESSAGE*2)
		cache.GroupId = int32(groupId)
	}

	if chunkId != cache.LastChunkId+1 || groupId != cache.GroupId {
		log.Errorf("Audio missing chunks; discarding (last_chunk_id=%i partial_audio_group_id=%i)\n", cache.LastChunkId, cache.GroupId)
		ResetAudioCache(cache)
		cache.Invalid = true
		return false
	}

	dataSize := int32(len(audioChunk.GetSignalPower()))
	copy(cache.Data[cache.Size:cache.Size+dataSize], audioChunk.GetSignalPower()[:])
	cache.Size += dataSize
	cache.LastChunkId = chunkId

	return chunkId == int32(audioChunk.GetAudioChunkCount()-1)
}

// Long running message for sending audio feed to listening sdk users
func (service *rpcService) AudioFeed(in *extint.AudioFeedRequest, stream extint.ExternalInterface_AudioFeedServer) error {
	if disableStreams {
		// Disabled for Vector 1.0 release
		return grpc.Errorf(codes.Unimplemented, "AudioFeed disabled in message_handler.go")
	}

	// @TODO: Expose other audio processing modes
	//
	// The composite multi-microphone non-beamforming (AUDIO_VOICE_DETECT_MODE) mode has been identified as the best for voice detection,
	// as well as incidentally calculating directional and noise_floor data.  As such it's most reasonable as the SDK's default mode.
	//
	// While this mode will send directional source data, it is different from DIRECTIONAL_MODE in that it does not isolate and clean
	// up the sound stream with respect to the loudest direction (which makes the result more human-ear pleasing but more ml difficult).
	//
	// It should however be noted that the robot will automatically shift into FAST_MODE (cleaned up single microphone) when
	// entering low power mode, so its important to leave this exposed.
	//

	// Enable audio stream
	err := AudioSendModeRequest(extint.AudioProcessingMode_AUDIO_VOICE_DETECT_MODE)
	if err != nil {
		return err
	}

	// Disable audio stream
	defer AudioSendModeRequest(extint.AudioProcessingMode_AUDIO_OFF)

	// Forward audio data from engine
	f, audioFeedChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_AudioChunk{}, 1024)
	defer f()

	cache := AudioFeedCache{
		Data:    nil,
		GroupId: -1,
		Invalid: false,
		Size:    0,
	}

	for result := range audioFeedChannel {

		audioChunk := result.GetAudioChunk()

		readyToSend := UnpackAudioChunk(audioChunk, &cache)
		if readyToSend {
			audioFeedResponse := &extint.AudioFeedResponse{
				RobotTimeStamp:     audioChunk.GetRobotTimeStamp(),
				GroupId:            uint32(audioChunk.GetGroupId()),
				SignalPower:        cache.Data[0:cache.Size],
				DirectionStrengths: audioChunk.GetDirectionStrengths(),
				SourceDirection:    audioChunk.GetSourceDirection(),
				SourceConfidence:   audioChunk.GetSourceConfidence(),
				NoiseFloorPower:    audioChunk.GetNoiseFloorPower(),
			}
			ResetAudioCache(&cache)

			if err := stream.Send(audioFeedResponse); err != nil {
				return err
			} else if err = stream.Context().Err(); err != nil {
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				return err
			}
		}
	}

	errMsg := "AudioChunk engine stream died unexpectedly"
	log.Errorln(errMsg)
	return grpc.Errorf(codes.Internal, errMsg)
}

// TODO: Add option to request a single image from the robot(SingleShot) once VIC-5159 is resolved.
func ImageSendModeRequest(mode extint.ImageRequest_ImageSendMode) error {
	log.Println("Requesting ImageRequest with mode(", mode, ")")

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ImageRequest{
			ImageRequest: &extint.ImageRequest{
				Mode: mode,
			},
		},
	})

	return err
}

type CameraFeedCache struct {
	Data        []byte
	ImageId     int32
	Invalid     bool
	Size        int32
	LastChunkId int32
}

func ResetCameraCache(cache *CameraFeedCache) error {
	cache.Data = nil
	cache.ImageId = -1
	cache.Invalid = false
	cache.Size = 0
	cache.LastChunkId = -1
	return nil
}

func UnpackCameraImageChunk(imageChunk *extint.ImageChunk, cache *CameraFeedCache) bool {
	imageId := int32(imageChunk.GetImageId())
	chunkId := int32(imageChunk.GetChunkId())

	if cache.ImageId != -1 && chunkId == 0 {
		if !cache.Invalid {
			log.Errorln("Lost final chunk of image; discarding")
		}
		cache.ImageId = -1
	}

	if cache.ImageId == -1 {
		if chunkId != 0 {
			if !cache.Invalid {
				log.Errorln("Received chunk of broken image")
			}
			cache.Invalid = true
			return false
		}
		// discard any previous in-progress image
		ResetCameraCache(cache)

		cache.Data = make([]byte, imageChunk.GetWidth()*imageChunk.GetHeight()*3)
		cache.ImageId = int32(imageId)
	}

	if chunkId != cache.LastChunkId+1 || imageId != cache.ImageId {
		log.Errorf("Image missing chunks; discarding (last_chunk_id=%i partial_image_id=%i)\n", cache.LastChunkId, cache.ImageId)
		ResetCameraCache(cache)
		cache.Invalid = true
		return false
	}

	dataSize := int32(len(imageChunk.GetData()))
	copy(cache.Data[cache.Size:cache.Size+dataSize], imageChunk.GetData()[:])
	cache.Size += dataSize
	cache.LastChunkId = chunkId

	return chunkId == int32(imageChunk.GetImageChunkCount()-1)
}

// Long running message for sending camera feed to listening sdk users
func (service *rpcService) CameraFeed(in *extint.CameraFeedRequest, stream extint.ExternalInterface_CameraFeedServer) error {
	if disableStreams {
		// Disabled for Vector 1.0 release
		return grpc.Errorf(codes.Unimplemented, "CameraFeed disabled in message_handler.go")
	}

	// Enable video stream
	err := ImageSendModeRequest(extint.ImageRequest_STREAM)
	if err != nil {
		return err
	}

	// Disable video stream
	defer ImageSendModeRequest(extint.ImageRequest_OFF)

	f, cameraFeedChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_ImageChunk{}, 1024)
	defer f()

	cache := CameraFeedCache{
		Data:    nil,
		ImageId: -1,
		Invalid: false,
		Size:    0,
	}

	for result := range cameraFeedChannel {

		imageChunk := result.GetImageChunk()
		readyToSend := UnpackCameraImageChunk(imageChunk, &cache)
		if readyToSend {
			cameraFeedResponse := &extint.CameraFeedResponse{
				FrameTimeStamp: imageChunk.GetFrameTimeStamp(),
				ImageId:        uint32(cache.ImageId),
				ImageEncoding:  imageChunk.GetImageEncoding(),
				Data:           cache.Data[0:cache.Size],
			}
			ResetCameraCache(&cache)

			if err := stream.Send(cameraFeedResponse); err != nil {
				return err
			} else if err = stream.Context().Err(); err != nil {
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				return err
			}
		}
	}

	errMsg := "ImageChunk engine stream died unexpectedly"
	log.Errorln(errMsg)
	return grpc.Errorf(codes.Internal, errMsg)
}

// CheckUpdateStatus tells if the robot is ready to reboot and update.
func (service *rpcService) CheckUpdateStatus(ctx context.Context, in *extint.CheckUpdateStatusRequest) (*extint.CheckUpdateStatusResponse, error) {
	if _, err := os.Stat("/run/update-engine/done"); err == nil {
		return &extint.CheckUpdateStatusResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_OK,
			},
			UpdateStatus: extint.CheckUpdateStatusResponse_READY_TO_INSTALL,
		}, nil
	}
	return &extint.CheckUpdateStatusResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_OK,
		},
		UpdateStatus: extint.CheckUpdateStatusResponse_NO_UPDATE,
	}, nil
}

// UpdateAndRestart reboots the robot when an update is available.
// This will apply the update when the robot starts up.
func (service *rpcService) UpdateAndRestart(ctx context.Context, in *extint.UpdateAndRestartRequest) (*extint.UpdateAndRestartResponse, error) {
	if _, err := os.Stat("/run/update-engine/done"); err == nil {
		go func() {
			<-time.After(5 * time.Second)
			exec.Command("/sbin/reboot").Run()
		}()
		return &extint.UpdateAndRestartResponse{
			Status: &extint.ResponseStatus{
				Code: extint.ResponseStatus_OK,
			},
		}, nil
	}
	return &extint.UpdateAndRestartResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_NOT_FOUND,
		},
	}, nil
}

// UploadDebugLogs will upload debug logs to S3, and return a url to the caller.
// TODO This is exposed as an external API. Prevent users from spamming this by internally rate-limiting or something?
func (service *rpcService) UploadDebugLogs(ctx context.Context, in *extint.UploadDebugLogsRequest) (*extint.UploadDebugLogsResponse, error) {
	url, err := loguploader.UploadDebugLogs()
	if err != nil {
		log.Println("MessageHandler.UploadDebugLogs.Error: " + err.Error())
		return nil, grpc.Errorf(codes.Internal, err.Error())
	}
	response := &extint.UploadDebugLogsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_OK,
		},
		Url: url,
	}
	return response, nil
}

// CheckCloudConnection is used to verify Vector's connection to the Anki Cloud
// Its main use is to be called by the app during setup, but is fine for use by the outside world.
func (service *rpcService) CheckCloudConnection(ctx context.Context, in *extint.CheckCloudRequest) (*extint.CheckCloudResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_CheckCloudResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_CheckCloudRequest{
			CheckCloudRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	payload, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	cloudResponse := payload.GetCheckCloudResponse()
	cloudResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return cloudResponse, nil
}

func (service *rpcService) DeleteCustomObjects(ctx context.Context, in *extint.DeleteCustomObjectsRequest) (*extint.DeleteCustomObjectsResponse, error) {
	if (in.Mask & uint32(extint.CustomObjectDeletionMask_DELETION_MASK_ARCHETYPES)) != 0 {
		f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_RobotDeletedCustomMarkerObjects, 1)
		defer f()

		_, err := engineCladManager.Write(gw_clad.NewMessageExternalToRobotWithUndefineAllCustomMarkerObjects(
			&gw_clad.UndefineAllCustomMarkerObjects{}))

		if err != nil {
			return nil, err
		}

		_, ok := <-responseChan
		if !ok {
			return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
		}
	}
	if (in.Mask & uint32(extint.CustomObjectDeletionMask_DELETION_MASK_FIXED_CUSTOM_OBJECTS)) != 0 {
		f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_RobotDeletedFixedCustomObjects, 1)
		defer f()

		_, err := engineCladManager.Write(gw_clad.NewMessageExternalToRobotWithDeleteFixedCustomObjects(
			&gw_clad.DeleteFixedCustomObjects{}))

		if err != nil {
			return nil, err
		}

		_, ok := <-responseChan
		if !ok {
			return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
		}
	}
	if (in.Mask & uint32(extint.CustomObjectDeletionMask_DELETION_MASK_CUSTOM_MARKER_OBJECTS)) != 0 {
		f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_RobotDeletedCustomMarkerObjects, 1)
		defer f()

		_, err := engineCladManager.Write(gw_clad.NewMessageExternalToRobotWithDeleteCustomMarkerObjects(
			&gw_clad.DeleteCustomMarkerObjects{}))

		if err != nil {
			return nil, err
		}

		_, ok := <-responseChan
		if !ok {
			return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
		}
	}

	return &extint.DeleteCustomObjectsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
	}, nil
}

func (service *rpcService) CreateFixedCustomObject(ctx context.Context, in *extint.CreateFixedCustomObjectRequest) (*extint.CreateFixedCustomObjectResponse, error) {
	f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_CreatedFixedCustomObject, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoCreateFixedCustomObjectToClad(in))
	if err != nil {
		return nil, err
	}

	chanResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := chanResponse.GetCreatedFixedCustomObject()

	return &extint.CreateFixedCustomObjectResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		ObjectId: response.ObjectID,
	}, nil
}

func (service *rpcService) DefineCustomBox(ctx context.Context, in *extint.DefineCustomBoxRequest) (*extint.DefineCustomBoxResponse, error) {
	f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_DefinedCustomObject, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoDefineCustomBoxToClad(in))
	if err != nil {
		return nil, err
	}

	chanResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := chanResponse.GetDefinedCustomObject()

	return &extint.DefineCustomBoxResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Success: response.Success,
	}, nil
}

func (service *rpcService) DefineCustomCube(ctx context.Context, in *extint.DefineCustomCubeRequest) (*extint.DefineCustomCubeResponse, error) {
	f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_DefinedCustomObject, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoDefineCustomCubeToClad(in))
	if err != nil {
		return nil, err
	}

	chanResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := chanResponse.GetDefinedCustomObject()

	return &extint.DefineCustomCubeResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Success: response.Success,
	}, nil
}

func (service *rpcService) DefineCustomWall(ctx context.Context, in *extint.DefineCustomWallRequest) (*extint.DefineCustomWallResponse, error) {
	f, responseChan := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_DefinedCustomObject, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoDefineCustomWallToClad(in))
	if err != nil {
		return nil, err
	}

	chanResponse, ok := <-responseChan
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to retrieve message")
	}
	response := chanResponse.GetDefinedCustomObject()

	return &extint.DefineCustomWallResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		Success: response.Success,
	}, nil
}

func newServer() *rpcService {
	return new(rpcService)
}
