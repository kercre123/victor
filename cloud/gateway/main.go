package main

import (
	"bytes"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"path"
	"reflect"
	"strings"
	"time"

	"anki/ipc"
	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	"github.com/golang/protobuf/proto"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type RobotToExternalCladResult struct {
	Message gw_clad.MessageRobotToExternal
	Error   error
}

// Enable logs about the messages coming and going from the engine.
// Most useful for debugging the json output being sent to the app.
const logVerbose = false

var (
	demoKeyPair  *tls.Certificate
	demoCertPool *x509.CertPool
	demoAddr     string

	// protobuf equivalents for clad socket and chan map
	protoEngineSock    ipc.Conn
	engineProtoChanMap map[string](chan<- extint.GatewayWrapper)

	// TODO: remove clad socket and map when there are no more clad messages being used
	engineSock    ipc.Conn
	engineChanMap map[gw_clad.MessageRobotToExternalTag](chan RobotToExternalCladResult)
)

func getSocketWithRetry(socketDir string, server string, name string) ipc.Conn {
	serverPath := path.Join(socketDir, server)
	for {
		sock, err := ipc.NewUnixgramClient(serverPath, name)
		if err != nil {
			log.Println("Couldn't create sockets for", server, "- retrying:", err)
			time.Sleep(time.Second)
		} else {
			return sock
		}
	}
}

type wrappedResponseWriter struct {
	http.ResponseWriter
	tag string
}

func (wrap *wrappedResponseWriter) WriteHeader(code int) {
	log.Printf("%s.WriteHeader(): %d", wrap.tag, code)
	wrap.ResponseWriter.WriteHeader(code)
}

func (wrap *wrappedResponseWriter) Write(b []byte) (int, error) {
	if wrap.tag == "json" && len(b) > 1 {
		log.Printf("%s.Write(): %s", wrap.tag, string(b))
	} else if wrap.tag == "grpc" && len(b) > 1 {
		log.Printf("%s.Write(): [% x]", wrap.tag, b)
	}
	return wrap.ResponseWriter.Write(b)
}

func (wrap *wrappedResponseWriter) Header() http.Header {
	h := wrap.ResponseWriter.Header()
	log.Printf("%s.Header(): %+v", wrap.tag, h)
	return h
}

func (wrap *wrappedResponseWriter) Flush() {
	wrap.ResponseWriter.(http.Flusher).Flush()
}

func (wrap *wrappedResponseWriter) CloseNotify() <-chan bool {
	return wrap.ResponseWriter.(http.CloseNotifier).CloseNotify()
}

func printRequest(r *http.Request, tag string) {
	log.Printf("%s.Request: %+v", tag, r)
}

func verboseHandlerFunc(grpcServer *grpc.Server, otherHandler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.ProtoMajor == 2 && strings.Contains(r.Header.Get("Content-Type"), "application/grpc") {
			printRequest(r, "grpc")
			wrap := wrappedResponseWriter{w, "grpc"}
			grpcServer.ServeHTTP(&wrap, r)
		} else {
			printRequest(r, "json")
			wrap := wrappedResponseWriter{w, "json"}
			otherHandler.ServeHTTP(&wrap, r)
		}
	})
}

func grpcHandlerFunc(grpcServer *grpc.Server, otherHandler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.ProtoMajor == 2 && strings.Contains(r.Header.Get("Content-Type"), "application/grpc") {
			grpcServer.ServeHTTP(w, r)
		} else {
			otherHandler.ServeHTTP(w, r)
		}
	})
}

func sendEventToChannel(event *extint.Event) {
	var protoMsg extint.GatewayWrapper
	msgType := reflect.TypeOf(&extint.GatewayWrapper_Event{}).String()
	protoMsg = extint.GatewayWrapper{
		OneofMessageType: &extint.GatewayWrapper_Event{
			// TODO: Convert all events into proto events
			Event: event,
		},
	}
	if c, ok := engineProtoChanMap[msgType]; ok {
		log.Println("Sending", msgType, "to listener")
		select {
		case c <- protoMsg:
			log.Println("Sent:", msgType)
		default:
			log.Println("Failed to send message. There might be a problem with the channel.")
		}
	}
}

// TODO: improve the logic for this to allow for multiple simultaneous requests
func processCladEngineMessages() {
	var msg gw_clad.MessageRobotToExternal
	var b, block []byte
	var buf bytes.Buffer
	for {
		buf.Reset()
		block = engineSock.ReadBlock()
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
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotObservedFace:
			event := &extint.Event{
				EventType: &extint.Event_RobotObservedFace{
					RobotObservedFace: CladRobotObservedFaceToProto(msg.GetRobotObservedFace()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotChangedObservedFaceID:
			event := &extint.Event{
				EventType: &extint.Event_RobotChangedObservedFaceId{
					RobotChangedObservedFaceId: CladRobotChangedObservedFaceIDToProto(msg.GetRobotChangedObservedFaceID()),
				},
			}
			sendEventToChannel(event)
			// @TODO: Convert all object events to proto VIC-4643
		case gw_clad.MessageRobotToExternalTag_ObjectConnectionState:
			event := &extint.Event{
				EventType: &extint.Event_ObjectConnectionState{
					ObjectConnectionState: CladObjectConnectionStateToProto(msg.GetObjectConnectionState()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectMoved:
			event := &extint.Event{
				EventType: &extint.Event_ObjectMoved{
					ObjectMoved: CladObjectMovedToProto(msg.GetObjectMoved()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectStoppedMoving:
			event := &extint.Event{
				EventType: &extint.Event_ObjectStoppedMoving{
					ObjectStoppedMoving: CladObjectStoppedMovingToProto(msg.GetObjectStoppedMoving()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectUpAxisChanged:
			event := &extint.Event{
				EventType: &extint.Event_ObjectUpAxisChanged{
					ObjectUpAxisChanged: CladObjectUpAxisChangedToProto(msg.GetObjectUpAxisChanged()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_ObjectTapped:
			event := &extint.Event{
				EventType: &extint.Event_ObjectTapped{
					ObjectTapped: CladObjectTappedToProto(msg.GetObjectTapped()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotObservedObject:
			event := &extint.Event{
				EventType: &extint.Event_RobotObservedObject{
					RobotObservedObject: CladRobotObservedObjectToProto(msg.GetRobotObservedObject()),
				},
			}
			sendEventToChannel(event)

		default:
			c, ok := engineChanMap[msg.Tag()]
			if ok {
				log.Println("Sending", msg.Tag(), "to listener")
				select {
				case c <- RobotToExternalCladResult{Message: msg}:
					log.Println("Sent:", msg.Tag())
				default:
					log.Println("Failed to send message. There might be a problem with the channel.")
				}
			}
		}
	}
}

// TODO: improve the logic for this to allow for multiple simultaneous requests
func processEngineMessages() {
	var msg extint.GatewayWrapper
	var b, block []byte
	for {
		msg.Reset()
		block = protoEngineSock.ReadBlock()
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
		msgType := reflect.TypeOf(msg.OneofMessageType).String()
		if c, ok := engineProtoChanMap[msgType]; ok {
			log.Println("Sending", msgType, "to listener")
			select {
			case c <- msg:
				log.Println("Sent:", msgType)
			default:
				log.Println("Failed to send", msgType, "message on the given channel.")
			}
		}
	}
}

func main() {
	log.Tag = "vic-gateway"
	log.Println("Launching vic-gateway")

	pair, err := tls.LoadX509KeyPair(Cert, Key)
	if err != nil {
		panic(err)
	}
	demoKeyPair = &pair
	demoCertPool = x509.NewCertPool()
	caCert, err := ioutil.ReadFile(Cert)
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
	ok := demoCertPool.AppendCertsFromPEM(caCert)
	if !ok {
		log.Println("Error: Bad certificates.")
		panic("Bad certificates.")
	}
	demoAddr = fmt.Sprintf("localhost:%d", Port)

	engineSock = getSocketWithRetry(SocketPath, "_engine_gateway_server_", "client")
	defer engineSock.Close()
	engineChanMap = make(map[gw_clad.MessageRobotToExternalTag](chan RobotToExternalCladResult))

	protoEngineSock = getSocketWithRetry(SocketPath, "_engine_gateway_proto_server_", "client")
	defer protoEngineSock.Close()
	engineProtoChanMap = make(map[string](chan<- extint.GatewayWrapper))

	log.Println("Sockets successfully created")

	creds, _ := credentials.NewServerTLSFromFile(Cert, Key)
	grpcServer := grpc.NewServer(grpc.Creds(creds))
	extint.RegisterExternalInterfaceServer(grpcServer, newServer())
	ctx := context.Background()

	dcreds := credentials.NewTLS(&tls.Config{
		ServerName:   demoAddr,
		Certificates: []tls.Certificate{*demoKeyPair},
		RootCAs:      demoCertPool,
	})
	dopts := []grpc.DialOption{grpc.WithTransportCredentials(dcreds)}

	gwmux := runtime.NewServeMux(runtime.WithMarshalerOption(runtime.MIMEWildcard, &runtime.JSONPb{EmitDefaults: true, OrigName: true, EnumsAsInts: true}))
	err = extint.RegisterExternalInterfaceHandlerFromEndpoint(ctx, gwmux, demoAddr, dopts)
	if err != nil {
		log.Println("Error during RegisterExternalInterfaceHandlerFromEndpoint:", err)
		os.Exit(1)
	}

	conn, err := net.Listen("tcp", fmt.Sprintf(":%d", Port))
	if err != nil {
		log.Println("Error during Listen:", err)
		panic(err)
	}

	handlerFunc := grpcHandlerFunc(grpcServer, gwmux)
	if logVerbose {
		handlerFunc = verboseHandlerFunc(grpcServer, gwmux)
	}

	srv := &http.Server{
		Addr:    demoAddr,
		Handler: handlerFunc,
		TLSConfig: &tls.Config{
			Certificates: []tls.Certificate{*demoKeyPair},
			NextProtos:   []string{"h2"},
		},
	}

	go processCladEngineMessages()
	go processEngineMessages()

	log.Println("Listening on Port:", Port)
	err = srv.Serve(tls.NewListener(conn, srv.TLSConfig))

	if err != nil {
		log.Println("Error during Serve:", err)
		os.Exit(1)
	}
	log.Println("Closing vic-gateway")
	return
}
