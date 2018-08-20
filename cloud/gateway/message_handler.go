package main

import (
	"encoding/base64"
	"encoding/binary"
	"io/ioutil"
	"math"
	"reflect"
	"strings"
	"time"

	"anki/log"
	cloud_clad "clad/cloud"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
)

const faceImagePixelsPerChunk = 600

func readAuthToken(ctx context.Context) (interface{}, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return nil, grpc.Errorf(codes.Internal, "Failed to extract context metadata")
	}
	log.Printf("Received metadata: %+v", md)

	if len(md["Authorization"]) == 0 {
		return nil, grpc.Errorf(codes.Unauthenticated, "No auth token")
	}
	authHeader := md["Authorization"][0]
	if !ok {
		return nil, grpc.Errorf(codes.Unauthenticated, "No auth token")
	}
	if strings.HasPrefix(authHeader, "Basic ") {
		_, err := base64.StdEncoding.DecodeString(authHeader[6:])
		if err != nil {
			return nil, grpc.Errorf(codes.Unauthenticated, "Failed to decode auth token (Base64)")
		}
		// todo
	} else if strings.HasPrefix(authHeader, "Bearer ") {
		return authHeader[7:], nil
	}
	return nil, grpc.Errorf(codes.Unauthenticated, "Invalid auth type")
}

func AuthInterceptor(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	_, err := readAuthToken(ctx)
	if err != nil {
		// return nil, err
	}

	// TODO: verify token

	return handler(ctx, req)
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

func ProtoPathMotionProfileToClad(msg *extint.PathMotionProfile) gw_clad.PathMotionProfile {
	return gw_clad.PathMotionProfile{
		SpeedMmps:                msg.SpeedMmps,
		AccelMmps2:               msg.AccelMmps2,
		DecelMmps2:               msg.DecelMmps2,
		PointTurnSpeedRadPerSec:  msg.PointTurnSpeedRadPerSec,
		PointTurnAccelRadPerSec2: msg.PointTurnAccelRadPerSec2,
		PointTurnDecelRadPerSec2: msg.PointTurnDecelRadPerSec2,
		DockSpeedMmps:            msg.DockSpeedMmps,
		DockAccelMmps2:           msg.DockAccelMmps2,
		DockDecelMmps2:           msg.DockDecelMmps2,
		ReverseSpeedMmps:         msg.ReverseSpeedMmps,
		IsCustom:                 msg.IsCustom,
	}
}

// TODO: we should find a way to auto-generate the equivalent of this function as part of clad or protoc
func ProtoGoToPoseToClad(msg *extint.GoToPoseRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithGotoPose(&gw_clad.GotoPose{
		XMm:        msg.XMm,
		YMm:        msg.YMm,
		Rad:        msg.Rad,
		MotionProf: ProtoPathMotionProfileToClad(msg.MotionProf),
		Level:      0,
	})
}

func ProtoDriveStraightToClad(msg *extint.DriveStraightRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithDriveStraight(&gw_clad.DriveStraight{
		SpeedMmps:           msg.SpeedMmps,
		DistMm:              msg.DistMm,
		ShouldPlayAnimation: msg.ShouldPlayAnimation,
	})
}

func ProtoTurnInPlaceToClad(msg *extint.TurnInPlaceRequest) *gw_clad.MessageExternalToRobot {
	// Bind IsAbsolute to a uint8 boundary, the clad side uses a uint8, proto side uses uint32
	if msg.IsAbsolute > math.MaxUint8 {
		msg.IsAbsolute = math.MaxUint8
	}
	return gw_clad.NewMessageExternalToRobotWithTurnInPlace(&gw_clad.TurnInPlace{
		AngleRad:        msg.AngleRad,
		SpeedRadPerSec:  msg.SpeedRadPerSec,
		AccelRadPerSec2: msg.AccelRadPerSec2,
		TolRad:          msg.TolRad,
		IsAbsolute:      uint8(msg.IsAbsolute),
	})
}

func ProtoSetHeadAngleToClad(msg *extint.SetHeadAngleRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithSetHeadAngle(&gw_clad.SetHeadAngle{
		AngleRad:          msg.AngleRad,
		MaxSpeedRadPerSec: msg.MaxSpeedRadPerSec,
		AccelRadPerSec2:   msg.AccelRadPerSec2,
		DurationSec:       msg.DurationSec,
	})
}

func ProtoSetLiftHeightToClad(msg *extint.SetLiftHeightRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithSetLiftHeight(&gw_clad.SetLiftHeight{
		HeightMm:          msg.HeightMm,
		MaxSpeedRadPerSec: msg.MaxSpeedRadPerSec,
		AccelRadPerSec2:   msg.AccelRadPerSec2,
		DurationSec:       msg.DurationSec,
	})
}

func SliceToArray(msg []uint32) [3]uint32 {
	var arr [3]uint32
	copy(arr[:], msg)
	return arr
}

func ProtoSetBackpackLightsToClad(msg *extint.SetBackpackLightsRequest) *gw_clad.MessageExternalToRobot {
	return gw_clad.NewMessageExternalToRobotWithSetBackpackLEDs(&gw_clad.SetBackpackLEDs{
		OnColor:               SliceToArray(msg.OnColor),
		OffColor:              SliceToArray(msg.OffColor),
		OnPeriodMs:            SliceToArray(msg.OnPeriodMs),
		OffPeriodMs:           SliceToArray(msg.OffPeriodMs),
		TransitionOnPeriodMs:  SliceToArray(msg.TransitionOnPeriodMs),
		TransitionOffPeriodMs: SliceToArray(msg.TransitionOffPeriodMs),
		Offset:                [3]int32{0, 0, 0},
	})
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

func SendOnboardingContinue(in *extint.GatewayWrapper_OnboardingContinue) (*extint.OnboardingInputResponse, error) {
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_OnboardingContinueResponse{}, 1)
	defer f()
	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: in,
	})
	if err != nil {
		return nil, err
	}
	continueResponse := <-responseChan
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
	getStepResponse := <-responseChan
	return &extint.OnboardingInputResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
		OneofMessageType: &extint.OnboardingInputResponse_OnboardingStepResponse{
			OnboardingStepResponse: getStepResponse.GetOnboardingStepResponse(),
		},
	}, nil
}

func SendOnboardingSkip(in *extint.GatewayWrapper_OnboardingSkip) (*extint.OnboardingInputResponse, error) {
	log.Println("Received rpc request OnboardingSkip(", in, ")")
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
	log.Println("Received rpc request OnboardingSkipOnboarding(", in, ")")
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
	log.Println("Received rpc request OnboardingRestart(", in, ")")
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

func (m *rpcService) ProtocolVersion(ctx context.Context, in *extint.ProtocolVersionRequest) (*extint.ProtocolVersionResponse, error) {
	return &extint.ProtocolVersionResponse{
		Result: extint.ProtocolVersionResponse_SUCCESS,
	}, nil
}

func (m *rpcService) DriveWheels(ctx context.Context, in *extint.DriveWheelsRequest) (*extint.DriveWheelsResponse, error) {
	log.Println("Received rpc request DriveWheels(", in, ")")
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

func (m *rpcService) PlayAnimation(ctx context.Context, in *extint.PlayAnimationRequest) (*extint.PlayAnimationResponse, error) {
	log.Println("Received rpc request PlayAnimation(", in, ")")
	f, animResponseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PlayAnimationResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoPlayAnimationToClad(in))
	if err != nil {
		return nil, err
	}

	setPlayAnimationResponse := <-animResponseChan
	response := setPlayAnimationResponse.GetPlayAnimationResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) ListAnimations(ctx context.Context, in *extint.ListAnimationsRequest) (*extint.ListAnimationsResponse, error) {
	log.Println("Received rpc request ListAnimations(", in, ")")

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
		case chanResponse := <-animationAvailableResponse:
			var newAnim = extint.Animation{
				Name: chanResponse.GetAnimationAvailable().AnimName,
			}
			anims = append(anims, &newAnim)
			remaining = remaining - 1
		case chanResponse := <-endOfMessageResponse:
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

func (m *rpcService) MoveHead(ctx context.Context, in *extint.MoveHeadRequest) (*extint.MoveHeadResponse, error) {
	log.Println("Received rpc request MoveHead(", in, ")")
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

func (m *rpcService) MoveLift(ctx context.Context, in *extint.MoveLiftRequest) (*extint.MoveLiftResponse, error) {
	log.Println("Received rpc request MoveLift(", in, ")")
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

func (m *rpcService) DriveArc(ctx context.Context, in *extint.DriveArcRequest) (*extint.DriveArcResponse, error) {
	log.Println("Received rpc request DriveArc(", in, ")")
	_, err := engineCladManager.Write(ProtoDriveArcToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.DriveArcResponse{
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
		slicedBinaryData := in.FaceData[firstByte:finalByte]

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

func (m *rpcService) DisplayFaceImageRGB(ctx context.Context, in *extint.DisplayFaceImageRGBRequest) (*extint.DisplayFaceImageRGBResponse, error) {
	log.Println("Received rpc request SetOLEDToSolidColor(", in, ")")

	const totalPixels = 17664
	chunkCount := (totalPixels + faceImagePixelsPerChunk + 1) / faceImagePixelsPerChunk

	SendFaceDataAsChunks(in, chunkCount, faceImagePixelsPerChunk, totalPixels)

	return &extint.DisplayFaceImageRGBResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (m *rpcService) AppIntent(ctx context.Context, in *extint.AppIntentRequest) (*extint.AppIntentResponse, error) {
	log.Println("Received rpc request AppIntent(", in, ")")
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

func (m *rpcService) CancelFaceEnrollment(ctx context.Context, in *extint.CancelFaceEnrollmentRequest) (*extint.CancelFaceEnrollmentResponse, error) {
	log.Println("Received rpc request CancelFaceEnrollment(", in, ")")
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

func (m *rpcService) RequestEnrolledNames(ctx context.Context, in *extint.RequestEnrolledNamesRequest) (*extint.RequestEnrolledNamesResponse, error) {
	log.Println("Received rpc request RequestEnrolledNames(", in, ")")
	f, enrolledNamesResponse := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_EnrolledNamesResponse, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoRequestEnrolledNamesToClad(in))
	if err != nil {
		return nil, err
	}
	names := <-enrolledNamesResponse
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
func (m *rpcService) UpdateEnrolledFaceByID(ctx context.Context, in *extint.UpdateEnrolledFaceByIDRequest) (*extint.UpdateEnrolledFaceByIDResponse, error) {
	log.Println("Received rpc request UpdateEnrolledFaceByID(", in, ")")
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
func (m *rpcService) EraseEnrolledFaceByID(ctx context.Context, in *extint.EraseEnrolledFaceByIDRequest) (*extint.EraseEnrolledFaceByIDResponse, error) {
	log.Println("Received rpc request EraseEnrolledFaceByID(", in, ")")
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
func (m *rpcService) EraseAllEnrolledFaces(ctx context.Context, in *extint.EraseAllEnrolledFacesRequest) (*extint.EraseAllEnrolledFacesResponse, error) {
	log.Println("Received rpc request EraseAllEnrolledFaces(", in, ")")
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

func (m *rpcService) SetFaceToEnroll(ctx context.Context, in *extint.SetFaceToEnrollRequest) (*extint.SetFaceToEnrollResponse, error) {
	log.Println("Received rpc request SetFaceToEnroll(", in, ")")
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

func (c *rpcService) EventKeepAlive(responses chan<- extint.GatewayWrapper, done chan struct{}) {
	ping := extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_Event{
			Event: &extint.Event{
				EventType: &extint.Event_KeepAlive{
					KeepAlive: &extint.KeepAlivePing{},
				},
			},
		},
	}
	for true {
		select {
		case <-done:
			return
		case <-time.After(time.Second):
			responses <- ping
		}
	}
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

	if responseType == "keep_alive" {
		return true
	}
	if whiteList != nil && isMember(responseType, whiteList.List) {
		return true
	}
	if blackList != nil && !isMember(responseType, blackList.List) {
		return true
	}
	return false
}

// Long running message for sending events to listening sdk users
func (c *rpcService) EventStream(in *extint.EventRequest, stream extint.ExternalInterface_EventStreamServer) error {
	log.Println("Received rpc request EventStream(", in, ")")

	f, eventsChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_Event{}, 16)
	defer f()

	done := make(chan struct{})
	go c.EventKeepAlive(eventsChannel, done)
	defer close(done)

	whiteList := in.GetWhiteList()
	blackList := in.GetBlackList()

	for response := range eventsChannel {
		event := response.GetEvent()
		if checkFilters(event, whiteList, blackList) {
			if logVerbose {
				log.Printf("Sending event: %+v\n", event)
			}
			eventResponse := &extint.EventResponse{
				Event: event,
			}
			if err := stream.Send(eventResponse); err != nil {
				log.Println("Closing stream:", err)
				return err
			} else if err = stream.Context().Err(); err != nil {
				log.Println("Closing stream:", err)
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				return err
			}
		}
	}
	return nil
}

func (c *rpcService) BehaviorRequestToGatewayWrapper(request *extint.BehaviorControlRequest) (*extint.GatewayWrapper, error) {
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

func (c *rpcService) BehaviorControlRequestHandler(in extint.ExternalInterface_BehaviorControlServer, responses chan extint.BehaviorControlResponse, done chan struct{}) {
	defer engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_ControlRelease{
			ControlRelease: &extint.ControlRelease{},
		},
	})
	for true {
		request, err := in.Recv()
		if err != nil {
			log.Println(err)
			close(done)
			return
		}
		log.Println("BehaviorControl Incoming Request:", request)

		msg, err := c.BehaviorRequestToGatewayWrapper(request)
		if err != nil {
			log.Println(err)
			close(done)
			return
		}
		_, err = engineProtoManager.Write(msg)
		if err != nil {
			close(done)
			return
		}
	}
}

func (c *rpcService) BehaviorControlResponseForwarding(wrapper chan extint.GatewayWrapper, responses chan extint.BehaviorControlResponse, done chan struct{}) {
	for true {
		select {
		case <-done:
			return
		case msg := <-wrapper:
			responses <- *msg.GetBehaviorControlResponse()
		}
	}
}

func (c *rpcService) BehaviorControlKeepAlive(responses chan extint.BehaviorControlResponse, done chan struct{}) {
	ping := extint.BehaviorControlResponse{
		ResponseType: &extint.BehaviorControlResponse_KeepAlive{
			KeepAlive: &extint.KeepAlivePing{},
		},
	}
	for true {
		select {
		case <-done:
			return
		case <-time.After(time.Second):
			responses <- ping
		}
	}
}

// TODO: name this better
func (c *rpcService) BehaviorControlResponseHandler(out extint.ExternalInterface_AssumeBehaviorControlServer, responses chan extint.BehaviorControlResponse, done chan struct{}) error {
	for true {
		select {
		case <-done:
			return nil
		case response := <-responses:
			log.Printf("BehaviorControl update: %+v", response)
			if err := out.Send(&response); err != nil {
				return err
			} else if err = out.Context().Err(); err != nil {
				// This is the case where the user disconnects the stream
				// We should still return the err in case the user doesn't think they disconnected
				return err
			}
		}
	}
	return nil
}

func (c *rpcService) BehaviorControl(bidirectionalStream extint.ExternalInterface_BehaviorControlServer) error {
	log.Println("Received rpc request BehaviorControl")

	done := make(chan struct{})
	responses := make(chan extint.BehaviorControlResponse)

	f, behaviorStatus := engineProtoManager.CreateChannel(&extint.GatewayWrapper_BehaviorControlResponse{}, 1)
	defer f()

	go c.BehaviorControlRequestHandler(bidirectionalStream, responses, done)
	go c.BehaviorControlKeepAlive(responses, done)
	go c.BehaviorControlResponseForwarding(behaviorStatus, responses, done)

	return c.BehaviorControlResponseHandler(bidirectionalStream, responses, done)
}

func (c *rpcService) AssumeBehaviorControl(in *extint.BehaviorControlRequest, out extint.ExternalInterface_AssumeBehaviorControlServer) error {
	log.Println("Received rpc request AssumeBehaviorControl(", in, ")")

	done := make(chan struct{})
	responses := make(chan extint.BehaviorControlResponse)

	f, behaviorStatus := engineProtoManager.CreateChannel(&extint.GatewayWrapper_BehaviorControlResponse{}, 1)
	defer f()

	msg, err := c.BehaviorRequestToGatewayWrapper(in)
	if err != nil {
		return err
	}

	_, err = engineProtoManager.Write(msg)
	if err != nil {
		return err
	}

	go c.BehaviorControlKeepAlive(responses, done)
	go c.BehaviorControlResponseForwarding(behaviorStatus, responses, done)

	return c.BehaviorControlResponseHandler(out, responses, done)
}

func (m *rpcService) DriveOffCharger(ctx context.Context, in *extint.DriveOffChargerRequest) (*extint.DriveOffChargerResponse, error) {
	log.Println("Received rpc request DriveOffChargerRequest(", in, ")")

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
	driveOffChargerResponse := <-responseChan
	response := driveOffChargerResponse.GetDriveOffChargerResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) DriveOnCharger(ctx context.Context, in *extint.DriveOnChargerRequest) (*extint.DriveOnChargerResponse, error) {
	log.Println("Received rpc request DriveOnChargerRequest(", in, ")")

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
	driveOnChargerResponse := <-responseChan
	response := driveOnChargerResponse.GetDriveOnChargerResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

// Example sending an int to and receiving an int from the engine
// TODO: Remove this example code once more code is converted to protobuf
func (m *rpcService) Pang(ctx context.Context, in *extint.Ping) (*extint.Pong, error) {
	if logVerbose {
		log.Println("Received rpc request Ping(", in, ")")
	}
	return &extint.Pong{
		Pong: in.Ping + 1,
	}, nil
}

// Request the current robot onboarding status
func (m *rpcService) GetOnboardingState(ctx context.Context, in *extint.OnboardingStateRequest) (*extint.OnboardingStateResponse, error) {
	log.Println("Received rpc request GetOnboardingState(", in, ")")
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
	onboardingState := <-responseChan
	return &extint.OnboardingStateResponse{
		OnboardingState: onboardingState.GetOnboardingState(),
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
	}, nil
}

func (m *rpcService) SendOnboardingInput(ctx context.Context, in *extint.OnboardingInputRequest) (*extint.OnboardingInputResponse, error) {
	log.Println("Received rpc request OnboardingInputRequest(", in, ")")

	// oneof_message_type
	switch x := in.OneofMessageType.(type) {
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

func (m *rpcService) PhotosInfo(ctx context.Context, in *extint.PhotosInfoRequest) (*extint.PhotosInfoResponse, error) {
	log.Println("Received rpc request PhotosInfo(", in, ")")

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
	payload := <-responseChan
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

func (m *rpcService) Photo(ctx context.Context, in *extint.PhotoRequest) (*extint.PhotoResponse, error) {
	log.Println("Received rpc request Photo(", in, ")")

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
	payload := <-responseChan
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

func (m *rpcService) Thumbnail(ctx context.Context, in *extint.ThumbnailRequest) (*extint.ThumbnailResponse, error) {
	log.Println("Received rpc request Thumbnail(", in, ")")

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
	payload := <-responseChan
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

func (m *rpcService) DeletePhoto(ctx context.Context, in *extint.DeletePhotoRequest) (*extint.DeletePhotoResponse, error) {
	log.Println("Received rpc request DeletePhoto(", in, ")")

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
	payload := <-responseChan
	photoResponse := payload.GetDeletePhotoResponse()
	photoResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return photoResponse, nil
}

func (m *rpcService) GetLatestAttentionTransfer(ctx context.Context, in *extint.LatestAttentionTransferRequest) (*extint.LatestAttentionTransferResponse, error) {
	log.Println("Received rpc request GetLatestAttentionTransfer(", in, ")")
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
	attentionTransfer := <-responseChan
	return &extint.LatestAttentionTransferResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		LatestAttentionTransfer: attentionTransfer.GetLatestAttentionTransfer(),
	}, nil
}

func (m *rpcService) EnableVisionMode(ctx context.Context, in *extint.EnableVisionModeRequest) (*extint.EnableVisionModeResponse, error) {
	log.Println("Received rpc request EnableVisionMode(", in, ")")
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

func (m *rpcService) ConnectCube(ctx context.Context, in *extint.ConnectCubeRequest) (*extint.ConnectCubeResponse, error) {
	log.Println("Received rpc request ConnectCube(", in, ")")

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
	gatewayWrapper := <-responseChan
	response := gatewayWrapper.GetConnectCubeResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) DisconnectCube(ctx context.Context, in *extint.DisconnectCubeRequest) (*extint.DisconnectCubeResponse, error) {
	log.Println("Received rpc request DisconnectCube(", in, ")")

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

func (m *rpcService) CubesAvailable(ctx context.Context, in *extint.CubesAvailableRequest) (*extint.CubesAvailableResponse, error) {
	log.Println("Received rpc request CubesAvailable(", in, ")")
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
	cubesAvailable := <-responseChan
	response := cubesAvailable.GetCubesAvailableResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) FlashCubeLights(ctx context.Context, in *extint.FlashCubeLightsRequest) (*extint.FlashCubeLightsResponse, error) {
	log.Println("Received rpc request FlashCubeLights(", in, ")")
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

func (m *rpcService) ForgetPreferredCube(ctx context.Context, in *extint.ForgetPreferredCubeRequest) (*extint.ForgetPreferredCubeResponse, error) {
	log.Println("Received rpc request ForgetPreferredCube(", in, ")")
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

func (m *rpcService) SetPreferredCube(ctx context.Context, in *extint.SetPreferredCubeRequest) (*extint.SetPreferredCubeResponse, error) {
	log.Println("Received rpc request SetPreferredCube(", in, ")")
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

func (m *rpcService) SetCubeLights(ctx context.Context, in *extint.SetCubeLightsRequest) (*extint.SetCubeLightsResponse, error) {
	log.Println("Received rpc request SetCubeLights(", in, ")")
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

func (m *rpcService) PushJdocs(ctx context.Context, in *extint.PushJdocsRequest) (*extint.PushJdocsResponse, error) {
	log.Println("Received rpc request PushJdocs(", in, ")")

	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_PushJdocsResponse{}, 1)
	defer f()

	_, err := engineProtoManager.Write(&extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_PushJdocsRequest{
			PushJdocsRequest: in,
		},
	})
	if err != nil {
		return nil, err
	}
	response := <-responseChan
	return response.GetPushJdocsResponse(), nil
}

func (m *rpcService) RobotStatusHistory(ctx context.Context, in *extint.RobotHistoryRequest) (*extint.RobotHistoryResponse, error) {
	log.Println("Received rpc request RobotStatusHistory(", in, ")")

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
	response := <-responseChan
	return response.GetRobotHistoryResponse(), nil
}

func (m *rpcService) PullJdocs(ctx context.Context, in *extint.PullJdocsRequest) (*extint.PullJdocsResponse, error) {
	log.Println("Received rpc request PullJdocs(", in, ")")

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
	response := <-responseChan
	return response.GetPullJdocsResponse(), nil
}

func (m *rpcService) UpdateSettings(ctx context.Context, in *extint.UpdateSettingsRequest) (*extint.UpdateSettingsResponse, error) {
	log.Println("Received rpc request UpdateSettings(", in, ")")

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
	response := <-responseChan
	return response.GetUpdateSettingsResponse(), nil
}

// NOTE: this is the only function that won't need to check the client_token_guid header
func (m *rpcService) UserAuthentication(ctx context.Context, in *extint.UserAuthenticationRequest) (*extint.UserAuthenticationResponse, error) {
	log.Println("Received rpc request UserAuthentication(", in, ")")

	f, authChan := switchboardManager.CreateChannel(gw_clad.SwitchboardResponseTag_AuthResponse, 1)
	defer f()

	switchboardManager.Write(gw_clad.NewSwitchboardRequestWithAuthRequest(&cloud_clad.AuthRequest{
		SessionToken: string(in.UserSessionId),
	}))
	response := <-authChan
	auth := response.GetAuthResponse()
	code := extint.UserAuthenticationResponse_UNAUTHORIZED
	token := auth.AppToken
	if auth.Error == cloud_clad.TokenError_NoError {
		code = extint.UserAuthenticationResponse_AUTHORIZED
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

func (m *rpcService) GoToPose(ctx context.Context, in *extint.GoToPoseRequest) (*extint.GoToPoseResponse, error) {
	log.Println("Received rpc request GoToPose(", in, ")")

	f, goToPoseResponse := engineCladManager.CreateChannel(gw_clad.MessageRobotToExternalTag_RobotCompletedAction, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoGoToPoseToClad(in))
	if err != nil {
		return nil, err
	}

	response := <-goToPoseResponse
	actionResult := response.GetRobotCompletedAction().Result
	return &extint.GoToPoseResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_RESPONSE_RECEIVED,
		},
		ActionResult: extint.ActionResult(actionResult),
	}, nil
}

func (m *rpcService) DriveStraight(ctx context.Context, in *extint.DriveStraightRequest) (*extint.DriveStraightResponse, error) {
	log.Println("Received rpc request DriveStraight(", in, ")")
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_DriveStraightResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoDriveStraightToClad(in))
	if err != nil {
		return nil, err
	}

	driveStraightResponse := <-responseChan
	response := driveStraightResponse.GetDriveStraightResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) TurnInPlace(ctx context.Context, in *extint.TurnInPlaceRequest) (*extint.TurnInPlaceResponse, error) {
	log.Println("Received rpc request TurnInPlace(", in, ")")
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_TurnInPlaceResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoTurnInPlaceToClad(in))
	if err != nil {
		return nil, err
	}

	turnInPlaceResponse := <-responseChan
	response := turnInPlaceResponse.GetTurnInPlaceResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) SetHeadAngle(ctx context.Context, in *extint.SetHeadAngleRequest) (*extint.SetHeadAngleResponse, error) {
	log.Println("Received rpc request SetHeadAngle(", in, ")")
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_SetHeadAngleResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoSetHeadAngleToClad(in))
	if err != nil {
		return nil, err
	}

	setHeadAngleResponse := <-responseChan
	response := setHeadAngleResponse.GetSetHeadAngleResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) SetLiftHeight(ctx context.Context, in *extint.SetLiftHeightRequest) (*extint.SetLiftHeightResponse, error) {
	log.Println("Received rpc request SetLiftHeight(", in, ")")
	f, responseChan := engineProtoManager.CreateChannel(&extint.GatewayWrapper_SetLiftHeightResponse{}, 1)
	defer f()

	_, err := engineCladManager.Write(ProtoSetLiftHeightToClad(in))
	if err != nil {
		return nil, err
	}

	setLiftHeightResponse := <-responseChan
	response := setLiftHeightResponse.GetSetLiftHeightResponse()
	response.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return response, nil
}

func (m *rpcService) SetBackpackLights(ctx context.Context, in *extint.SetBackpackLightsRequest) (*extint.SetBackpackLightsResponse, error) {
	log.Println("Received rpc request SetBackpackLights(", in, ")")
	_, err := engineCladManager.Write(ProtoSetBackpackLightsToClad(in))
	if err != nil {
		return nil, err
	}
	return &extint.SetBackpackLightsResponse{
		Status: &extint.ResponseStatus{
			Code: extint.ResponseStatus_REQUEST_PROCESSING,
		},
	}, nil
}

func (m *rpcService) BatteryState(ctx context.Context, in *extint.BatteryStateRequest) (*extint.BatteryStateResponse, error) {
	log.Println("Received rpc request BatteryState(", in, ")")

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
	payload := <-responseChan
	payload.GetBatteryStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetBatteryStateResponse(), nil
}

func (m *rpcService) VersionState(ctx context.Context, in *extint.VersionStateRequest) (*extint.VersionStateResponse, error) {
	log.Println("Received rpc request VersionState(", in, ")")

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
	payload := <-responseChan
	payload.GetVersionStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetVersionStateResponse(), nil
}

func (m *rpcService) NetworkState(ctx context.Context, in *extint.NetworkStateRequest) (*extint.NetworkStateResponse, error) {
	log.Println("Received rpc request NetworkState(", in, ")")

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
	payload := <-responseChan
	payload.GetNetworkStateResponse().Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return payload.GetNetworkStateResponse(), nil
}

func (m *rpcService) SayText(ctx context.Context, in *extint.SayTextRequest) (*extint.SayTextResponse, error) {
	log.Println("Received rpc request SayText(", in, ")")

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
	payload := <-responseChan
	sayTextResponse := payload.GetSayTextResponse()
	sayTextResponse.Status = &extint.ResponseStatus{
		Code: extint.ResponseStatus_RESPONSE_RECEIVED,
	}
	return sayTextResponse, nil
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

// Long running message for sending camera feed to listening sdk users
func (m *rpcService) CameraFeed(in *extint.CameraFeedRequest, stream extint.ExternalInterface_CameraFeedServer) error {

	log.Println("Received rpc request CameraFeed(", in, ")")

	// Enable video stream
	err := ImageSendModeRequest(extint.ImageRequest_STREAM)
	if err != nil {
		return err
	}

	// Disable video stream
	defer ImageSendModeRequest(extint.ImageRequest_OFF)

	f, cameraFeedChannel := engineProtoManager.CreateChannel(&extint.GatewayWrapper_ImageChunk{}, 10)
	defer f()

	for result := range cameraFeedChannel {
		cameraFeedResponse := &extint.CameraFeedResponse{
			ImageChunk: result.GetImageChunk(),
		}
		if err := stream.Send(cameraFeedResponse); err != nil {
			return err
		} else if err = stream.Context().Err(); err != nil {
			// This is the case where the user disconnects the stream
			// We should still return the err in case the user doesn't think they disconnected
			return err
		}
	}

	return nil
}

func newServer() *rpcService {
	return new(rpcService)
}
