package main

import (
	"anki/ipc"
	"bytes"
	"encoding/binary"
	"reflect"
	"runtime"
	"sync"
	"time"

	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

// Note: If there is a function that can be shared, use the engine manager.
//       The only reason I duplicated the functions so far was interfaces
//       were strangely causing the deleteFromMap to fail. - shawn 7/17/18
type IpcManager struct {
	connMutex sync.Mutex
	conn      ipc.Conn
}

func (manager *IpcManager) Connect(path string, name string) func() error {
	for {
		conn, err := ipc.NewUnixgramClient(path, name)
		if err != nil {
			log.Printf("Couldn't create sockets for %s & %s_%s - retrying: %s", path, path, name, err.Error())
			time.Sleep(time.Second)
		} else {
			manager.conn = conn
			return manager.conn.Close
		}
	}
}

type EngineProtoIpcManager struct {
	IpcManager
	managerMutex    sync.RWMutex
	managedChannels map[string]([]chan<- extint.GatewayWrapper)
}

func (manager *EngineProtoIpcManager) Init() {
	manager.managedChannels = make(map[string]([]chan<- extint.GatewayWrapper))
}

func (manager *EngineProtoIpcManager) Write(msg proto.Message) (int, error) {
	var err error
	var buf bytes.Buffer
	if msg == nil {
		return -1, grpc.Errorf(codes.InvalidArgument, "Unable to parse request")
	}
	if err = binary.Write(&buf, binary.LittleEndian, uint16(proto.Size(msg))); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}
	data, err := proto.Marshal(msg)
	if err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}
	if err = binary.Write(&buf, binary.LittleEndian, data); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}

	manager.connMutex.Lock()
	defer manager.connMutex.Unlock()
	return manager.conn.Write(buf.Bytes())
}

// TODO: make this generic (via interfaces). Comparison failed when not specific.
func (manager *EngineProtoIpcManager) deleteFromMap(listener chan extint.GatewayWrapper, tag string) func() {
	return func() {
		manager.managerMutex.Lock()
		defer manager.managerMutex.Unlock()
		chanSlice := manager.managedChannels[tag]
		for idx, v := range chanSlice {
			if v == listener {
				chanSlice[idx] = chanSlice[len(chanSlice)-1]
				manager.managedChannels[tag] = chanSlice[:len(chanSlice)-1]
				break
			}
			if len(chanSlice)-1 == idx {
				log.Println("Warning: failed to remove listener:", listener, "from array:", chanSlice)
			}
		}
		if len(manager.managedChannels[tag]) == 0 {
			delete(manager.managedChannels, tag)
		}
	}
}

func (manager *EngineProtoIpcManager) CreateChannel(tag interface{}, numChannels int) (func(), chan extint.GatewayWrapper) {
	result := make(chan extint.GatewayWrapper, numChannels)
	reflectedType := reflect.TypeOf(tag).String()
	log.Println("Listening for", reflectedType)
	manager.managerMutex.Lock()
	defer manager.managerMutex.Unlock()
	slice := manager.managedChannels[reflectedType]
	if slice == nil {
		slice = make([]chan<- extint.GatewayWrapper, 0)
	}
	manager.managedChannels[reflectedType] = append(slice, result)
	return manager.deleteFromMap(result, reflectedType), result
}

func (manager *EngineProtoIpcManager) CreateUniqueChannel(tag interface{}, numChannels int) (func(), chan extint.GatewayWrapper, bool) {
	reflectedType := reflect.TypeOf(tag).String()
	manager.managerMutex.RLock()
	_, ok := manager.managedChannels[reflectedType]
	manager.managerMutex.RUnlock()
	if ok {
		return nil, nil, false
	}

	f, c := manager.CreateChannel(tag, numChannels)
	return f, c, true
}

func (manager *EngineProtoIpcManager) SendToListeners(tag string, msg extint.GatewayWrapper) {
	manager.managerMutex.RLock()
	chanList, ok := manager.managedChannels[tag]
	manager.managerMutex.RUnlock()
	if !ok {
		return // No listeners for message
	}
	if logVerbose {
		log.Printf("Sending %s to listeners\n", tag)
	}
	var wg sync.WaitGroup
	for idx, listener := range chanList {
		wg.Add(1)
		go func(idx int, listener chan<- extint.GatewayWrapper, msg extint.GatewayWrapper) {
			defer wg.Done()
			select {
			case listener <- msg:
				if logVerbose {
					log.Printf("Sent to listener #%d: %s\n", idx, tag)
				}
			case <-time.After(100 * time.Millisecond):
				log.Printf("Failed to send message %s for listener #%d. There might be a problem with the channel.\n", tag, idx)
			}
		}(idx, listener, msg)
	}
	wg.Wait()
	runtime.Gosched()
}

func (manager *EngineProtoIpcManager) ProcessMessages() {
	var msg extint.GatewayWrapper
	var b, block []byte
	for {
		msg.Reset()
		block = manager.conn.ReadBlock()
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
	managerMutex    sync.RWMutex
	managedChannels map[gw_clad.SwitchboardResponseTag]([]chan<- gw_clad.SwitchboardResponse)
}

func (manager *SwitchboardIpcManager) Init() {
	manager.managedChannels = make(map[gw_clad.SwitchboardResponseTag]([]chan<- gw_clad.SwitchboardResponse))
}

func (manager *SwitchboardIpcManager) ProcessMessages() {
	var msg gw_clad.SwitchboardResponse
	var b, block []byte
	var buf bytes.Buffer
	for {
		buf.Reset()
		block = manager.conn.ReadBlock()
		if block == nil {
			log.Println("Error: switchboard socket returned empty message")
			continue
		} else if len(block) < 2 {
			log.Println("Error: switchboard socket message too small")
			continue
		}
		b = block[2:]
		buf.Write(b)
		if err := msg.Unpack(&buf); err != nil {
			// Intentionally ignoring errors for unknown messages
			// TODO: treat this as an error condition once VIC-3186 is completed
			continue
		}

		manager.SendToListeners(msg)
	}
}

func (manager *SwitchboardIpcManager) Write(msg *gw_clad.SwitchboardRequest) (int, error) {
	var err error
	var buf bytes.Buffer
	if msg == nil {
		return -1, grpc.Errorf(codes.InvalidArgument, "Unable to parse request")
	}
	if err = binary.Write(&buf, binary.LittleEndian, uint16(msg.Size())); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}
	if err = msg.Pack(&buf); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}

	manager.connMutex.Lock()
	defer manager.connMutex.Unlock()
	return manager.conn.Write(buf.Bytes())
}

// TODO: make this generic (via interfaces). Comparison failed when not specific.
func (manager *SwitchboardIpcManager) deleteFromMap(listener chan gw_clad.SwitchboardResponse, tag gw_clad.SwitchboardResponseTag) func() {
	return func() {
		manager.managerMutex.Lock()
		defer manager.managerMutex.Unlock()
		chanSlice := manager.managedChannels[tag]
		for idx, v := range chanSlice {
			if v == listener {
				chanSlice[idx] = chanSlice[len(chanSlice)-1]
				manager.managedChannels[tag] = chanSlice[:len(chanSlice)-1]
				break
			}
			if len(chanSlice)-1 == idx {
				log.Println("Warning: failed to remove listener:", listener, "from array:", chanSlice)
			}
		}
		if len(manager.managedChannels[tag]) == 0 {
			delete(manager.managedChannels, tag)
		}
	}
}

func (manager *SwitchboardIpcManager) CreateChannel(tag gw_clad.SwitchboardResponseTag, numChannels int) (func(), chan gw_clad.SwitchboardResponse) {
	result := make(chan gw_clad.SwitchboardResponse, numChannels)
	log.Printf("Listening for %+v", tag)
	manager.managerMutex.Lock()
	defer manager.managerMutex.Unlock()
	slice := manager.managedChannels[tag]
	if slice == nil {
		slice = make([]chan<- gw_clad.SwitchboardResponse, 0)
	}
	manager.managedChannels[tag] = append(slice, result)
	return manager.deleteFromMap(result, tag), result
}

func (manager *SwitchboardIpcManager) SendToListeners(msg gw_clad.SwitchboardResponse) {
	tag := msg.Tag()
	manager.managerMutex.RLock()
	chanList, ok := manager.managedChannels[tag]
	manager.managerMutex.RUnlock()
	if !ok {
		return // No listeners for message
	}
	if logVerbose {
		log.Printf("Sending %s to listeners\n", tag)
	}
	var wg sync.WaitGroup
	for idx, listener := range chanList {
		wg.Add(1)
		go func(idx int, listener chan<- gw_clad.SwitchboardResponse, msg gw_clad.SwitchboardResponse) {
			defer wg.Done()
			select {
			case listener <- msg:
				if logVerbose {
					log.Printf("Sent to listener #%d: %s\n", idx, tag)
				}
			case <-time.After(100 * time.Millisecond):
				log.Printf("Failed to send message %s for listener #%d. There might be a problem with the channel.\n", tag, idx)
			}
		}(idx, listener, msg)
	}
	wg.Wait()
	runtime.Gosched()
}

// TODO: Remove CLAD manager once it's no longer needed

type EngineCladIpcManager struct {
	IpcManager
	managerMutex    sync.RWMutex
	managedChannels map[gw_clad.MessageRobotToExternalTag]([]chan<- gw_clad.MessageRobotToExternal)
}

func (manager *EngineCladIpcManager) Init() {
	manager.managedChannels = make(map[gw_clad.MessageRobotToExternalTag]([]chan<- gw_clad.MessageRobotToExternal))
}

func (manager *EngineCladIpcManager) Write(msg *gw_clad.MessageExternalToRobot) (int, error) {
	var err error
	var buf bytes.Buffer
	if msg == nil {
		return -1, grpc.Errorf(codes.InvalidArgument, "Unable to parse request")
	}
	if err = binary.Write(&buf, binary.LittleEndian, uint16(msg.Size())); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}
	if err = msg.Pack(&buf); err != nil {
		return -1, grpc.Errorf(codes.Internal, err.Error())
	}

	manager.connMutex.Lock()
	defer manager.connMutex.Unlock()
	return manager.conn.Write(buf.Bytes())
}

func (manager *EngineCladIpcManager) deleteFromMap(listener chan gw_clad.MessageRobotToExternal, tag gw_clad.MessageRobotToExternalTag) func() {
	return func() {
		manager.managerMutex.Lock()
		defer manager.managerMutex.Unlock()
		chanSlice := manager.managedChannels[tag]
		for idx, v := range chanSlice {
			if v == listener {
				chanSlice[idx] = chanSlice[len(chanSlice)-1]
				manager.managedChannels[tag] = chanSlice[:len(chanSlice)-1]
				break
			}
			if len(chanSlice)-1 == idx {
				log.Println("Warning: failed to remove listener:", listener, "from array:", chanSlice)
			}
		}
		if len(manager.managedChannels[tag]) == 0 {
			delete(manager.managedChannels, tag)
		}
	}
}

func (manager *EngineCladIpcManager) CreateChannel(tag gw_clad.MessageRobotToExternalTag, numChannels int) (func(), chan gw_clad.MessageRobotToExternal) {
	result := make(chan gw_clad.MessageRobotToExternal, numChannels)
	log.Printf("Listening for %+v", tag)
	manager.managerMutex.Lock()
	defer manager.managerMutex.Unlock()
	slice := manager.managedChannels[tag]
	if slice == nil {
		slice = make([]chan<- gw_clad.MessageRobotToExternal, 0)
	}
	manager.managedChannels[tag] = append(slice, result)
	return manager.deleteFromMap(result, tag), result
}

func (manager *EngineCladIpcManager) SendToListeners(msg gw_clad.MessageRobotToExternal) {
	tag := msg.Tag()
	manager.managerMutex.RLock()
	chanList, ok := manager.managedChannels[tag]
	manager.managerMutex.RUnlock()
	if !ok {
		return // No listeners for message
	}
	if logVerbose {
		log.Printf("Sending %s to listeners\n", tag)
	}
	var wg sync.WaitGroup
	for idx, listener := range chanList {
		wg.Add(1)
		go func(idx int, listener chan<- gw_clad.MessageRobotToExternal, msg gw_clad.MessageRobotToExternal) {
			defer wg.Done()
			select {
			case listener <- msg:
				if logVerbose {
					log.Printf("Sent to listener #%d: %s\n", idx, tag)
				}
			case <-time.After(100 * time.Millisecond):
				log.Printf("Failed to send message %s for listener #%d. There might be a problem with the channel.\n", tag, idx)
			}
		}(idx, listener, msg)
	}
	wg.Wait()
	runtime.Gosched()
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
		block = manager.conn.ReadBlock()
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
		case gw_clad.MessageRobotToExternalTag_ObjectAvailable:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectAvailable{
							ObjectAvailable: CladObjectAvailableToProto(msg.GetObjectAvailable()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectConnectionState:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectConnectionState{
							ObjectConnectionState: CladObjectConnectionStateToProto(msg.GetObjectConnectionState()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectMoved:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectMoved{
							ObjectMoved: CladObjectMovedToProto(msg.GetObjectMoved()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectStoppedMoving:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectStoppedMoving{
							ObjectStoppedMoving: CladObjectStoppedMovingToProto(msg.GetObjectStoppedMoving()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectUpAxisChanged:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectUpAxisChanged{
							ObjectUpAxisChanged: CladObjectUpAxisChangedToProto(msg.GetObjectUpAxisChanged()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectTapped:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_ObjectTapped{
							ObjectTapped: CladObjectTappedToProto(msg.GetObjectTapped()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotObservedObject:
			event := &extint.Event{
				EventType: &extint.Event_ObjectEvent{
					ObjectEvent: &extint.ObjectEvent{
						ObjectEventType: &extint.ObjectEvent_RobotObservedObject{
							RobotObservedObject: CladRobotObservedObjectToProto(msg.GetRobotObservedObject()),
						},
					},
				},
			}
			manager.SendEventToChannel(event)

		default:
			manager.SendToListeners(msg)
		}
	}
}
