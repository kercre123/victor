package main

import (
	"anki/ipc"
	"bytes"
	"encoding/binary"
	"path"
	"reflect"
	"runtime"
	"sync"
	"time"

	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// Note: If there is a function that can be shared, use the engine manager.
//       The only reason I duplicated the functions so far was interfaces
//       were strangely causing the deleteFromMap to fail. - shawn 7/17/18
type IpcManager struct {
	Conn ipc.Conn
}

func (manager *IpcManager) Connect(socketDir string, server string) func() error {
	const name = "client"
	serverPath := path.Join(socketDir, server)
	for {
		conn, err := ipc.NewUnixgramClient(serverPath, name)
		if err != nil {
			log.Println("Couldn't create sockets for", server, "- retrying:", err)
			time.Sleep(time.Second)
		} else {
			manager.Conn = conn
			return manager.Conn.Close
		}
	}
}

type EngineProtoIpcManager struct {
	IpcManager
	ManagedChannels map[string]([]chan<- extint.GatewayWrapper)
}

func (manager *EngineProtoIpcManager) WriteToEngine(msg proto.Message) (int, error) {
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
	return manager.Conn.Write(buf.Bytes())
}

// TODO: make this generic (via interfaces). Comparison failed when not specific.
func (manager *EngineProtoIpcManager) deleteFromMap(listener chan extint.GatewayWrapper, tag string) func() {
	return func() {
		chanSlice := manager.ManagedChannels[tag]
		for idx, v := range chanSlice {
			if v == listener {
				chanSlice[idx] = chanSlice[len(chanSlice)-1]
				manager.ManagedChannels[tag] = chanSlice[:len(chanSlice)-1]
				break
			}
			if len(chanSlice)-1 == idx {
				log.Println("Warning: failed to remove listener:", listener, "from array:", chanSlice)
			}
		}
		if len(manager.ManagedChannels[tag]) == 0 {
			delete(manager.ManagedChannels, tag)
		}
	}
}

func (manager *EngineProtoIpcManager) CreateChannel(tag interface{}, numChannels int) (func(), chan extint.GatewayWrapper) {
	result := make(chan extint.GatewayWrapper, numChannels)
	reflectedType := reflect.TypeOf(tag).String()
	log.Println("Listening for", reflectedType)
	slice := manager.ManagedChannels[reflectedType]
	if slice == nil {
		slice = make([]chan<- extint.GatewayWrapper, 0)
	}
	manager.ManagedChannels[reflectedType] = append(slice, result)
	return manager.deleteFromMap(result, reflectedType), result
}

func (manager *EngineProtoIpcManager) CreateUniqueChannel(tag interface{}, numChannels int) (func(), chan extint.GatewayWrapper, bool) {
	reflectedType := reflect.TypeOf(tag).String()
	if _, ok := engineProtoManager.ManagedChannels[reflectedType]; ok {
		return nil, nil, false
	}

	f, c := manager.CreateChannel(tag, numChannels)
	return f, c, true
}

func (manager *EngineProtoIpcManager) SendToListeners(tag string, msg extint.GatewayWrapper) {
	if chanList, ok := manager.ManagedChannels[tag]; ok {
		log.Printf("Sending %s to listener\n", tag)
		var wg sync.WaitGroup
		for idx, listener := range chanList {
			wg.Add(1)
			go func(idx int, listener chan<- extint.GatewayWrapper, msg extint.GatewayWrapper) {
				select {
				case listener <- msg:
					log.Printf("Sent %d:%s\n", idx, tag)
				case <-time.After(100 * time.Millisecond):
					log.Printf("Failed to send message %s for listener #%d. There might be a problem with the channel.\n", tag, idx)
				}
				wg.Done()
			}(idx, listener, msg)
		}
		wg.Wait()
		runtime.Gosched()
	}
}

func (manager *EngineProtoIpcManager) ProcessMessages() {
	var msg extint.GatewayWrapper
	var b, block []byte
	for {
		msg.Reset()
		block = manager.Conn.ReadBlock()
		if block == nil {
			log.Println("Error: engine socket returned empty message")
			continue
		} else if len(block) < 2 {
			log.Println("Error: engine socket message too small")
			continue
		}
		b = block[2:]

		if err := proto.Unmarshal(b, &msg); err != nil {
			log.Println("Decoding error (", err, "):", b)
			continue
		}
		tag := reflect.TypeOf(msg.OneofMessageType).String()
		manager.SendToListeners(tag, msg)
	}
}

type SwitchboardIpcManager struct {
	IpcManager
}

func (manager *SwitchboardIpcManager) ProcessMessages() {

}

// TODO: Remove CLAD manager once it's no longer needed

type EngineCladIpcManager struct {
	IpcManager
	ManagedChannels map[gw_clad.MessageRobotToExternalTag]([]chan<- gw_clad.MessageRobotToExternal)
}

func (manager *EngineCladIpcManager) WriteToEngine(msg *gw_clad.MessageExternalToRobot) (int, error) {
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
	return manager.Conn.Write(buf.Bytes())
}

func (manager *EngineCladIpcManager) deleteFromMap(listener chan gw_clad.MessageRobotToExternal, tag gw_clad.MessageRobotToExternalTag) func() {
	return func() {
		chanSlice := manager.ManagedChannels[tag]
		for idx, v := range chanSlice {
			if v == listener {
				chanSlice[idx] = chanSlice[len(chanSlice)-1]
				manager.ManagedChannels[tag] = chanSlice[:len(chanSlice)-1]
				break
			}
			if len(chanSlice)-1 == idx {
				log.Println("Warning: failed to remove listener:", listener, "from array:", chanSlice)
			}
		}
		if len(manager.ManagedChannels[tag]) == 0 {
			delete(manager.ManagedChannels, tag)
		}
	}
}

func (manager *EngineCladIpcManager) CreateChannel(tag gw_clad.MessageRobotToExternalTag, numChannels int) (func(), chan gw_clad.MessageRobotToExternal) {
	result := make(chan gw_clad.MessageRobotToExternal, numChannels)
	log.Printf("Listening for %+v", tag)
	slice := manager.ManagedChannels[tag]
	if slice == nil {
		slice = make([]chan<- gw_clad.MessageRobotToExternal, 0)
	}
	manager.ManagedChannels[tag] = append(slice, result)
	return manager.deleteFromMap(result, tag), result
}

func (manager *EngineCladIpcManager) SendToListeners(msg gw_clad.MessageRobotToExternal) {
	tag := msg.Tag()
	if chanList, ok := manager.ManagedChannels[tag]; ok {
		log.Println("Sending", tag, "to listener")
		var wg sync.WaitGroup
		for idx, listener := range chanList {
			wg.Add(1)
			go func(idx int, listener chan<- gw_clad.MessageRobotToExternal, msg gw_clad.MessageRobotToExternal) {
				select {
				case listener <- msg:
					log.Printf("Sent %d:%s\n", idx, tag)
				case <-time.After(100 * time.Millisecond):
					log.Printf("Failed to send message %s for listener #%d. There might be a problem with the channel.\n", tag, idx)
				}
				wg.Done()
			}(idx, listener, msg)
		}
		wg.Wait()
		runtime.Gosched()
	}
}

func (manager *EngineCladIpcManager) SendEventToChannel(event *extint.Event) {
	tag := reflect.TypeOf(&extint.GatewayWrapper_Event{}).String()
	msg := extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_Event{
			// TODO: Convert all events into proto events
			Event: event,
		},
	}
	engineProtoManager.SendToListeners(tag, msg)
}

func (manager *EngineCladIpcManager) ProcessMessages() {
	var msg gw_clad.MessageRobotToExternal
	var b, block []byte
	var buf bytes.Buffer
	for {
		buf.Reset()
		block = engineCladManager.Conn.ReadBlock()
		if block == nil {
			log.Println("Error: engine socket returned empty message")
			continue
		} else if len(block) < 2 {
			log.Println("Error: engine socket message too small")
			continue
		}
		b = block[2:]
		buf.Write(b)
		if err := msg.Unpack(&buf); err != nil {
			// Intentionally ignoring errors for unknown messages
			// TODO: treat this as an error condition once VIC-3186 is completed
			continue
		}

		// TODO: Refactor after RobotObservedFace and RobotChangedObservedFaceID are added to events
		switch msg.Tag() {
		case gw_clad.MessageRobotToExternalTag_Event:
			event := CladEventToProto(msg.GetEvent())
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotObservedFace:
			event := &extint.Event{
				EventType: &extint.Event_RobotObservedFace{
					RobotObservedFace: CladRobotObservedFaceToProto(msg.GetRobotObservedFace()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotChangedObservedFaceID:
			event := &extint.Event{
				EventType: &extint.Event_RobotChangedObservedFaceId{
					RobotChangedObservedFaceId: CladRobotChangedObservedFaceIDToProto(msg.GetRobotChangedObservedFaceID()),
				},
			}
			manager.SendEventToChannel(event)
			// @TODO: Convert all object events to proto VIC-4643
		case gw_clad.MessageRobotToExternalTag_ObjectConnectionState:
			event := &extint.Event{
				EventType: &extint.Event_ObjectConnectionState{
					ObjectConnectionState: CladObjectConnectionStateToProto(msg.GetObjectConnectionState()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectMoved:
			event := &extint.Event{
				EventType: &extint.Event_ObjectMoved{
					ObjectMoved: CladObjectMovedToProto(msg.GetObjectMoved()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectStoppedMoving:
			event := &extint.Event{
				EventType: &extint.Event_ObjectStoppedMoving{
					ObjectStoppedMoving: CladObjectStoppedMovingToProto(msg.GetObjectStoppedMoving()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectUpAxisChanged:
			event := &extint.Event{
				EventType: &extint.Event_ObjectUpAxisChanged{
					ObjectUpAxisChanged: CladObjectUpAxisChangedToProto(msg.GetObjectUpAxisChanged()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectTapped:
			event := &extint.Event{
				EventType: &extint.Event_ObjectTapped{
					ObjectTapped: CladObjectTappedToProto(msg.GetObjectTapped()),
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotObservedObject:
			event := &extint.Event{
				EventType: &extint.Event_RobotObservedObject{
					RobotObservedObject: CladRobotObservedObjectToProto(msg.GetRobotObservedObject()),
				},
			}
			manager.SendEventToChannel(event)

		default:
			manager.SendToListeners(msg)
		}
	}
}
