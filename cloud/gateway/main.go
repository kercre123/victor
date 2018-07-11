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

func getSocketWithRetry(socket_dir string, server string, name string) ipc.Conn {
	server_path := path.Join(socket_dir, server)
	for {
		sock, err := ipc.NewUnixgramClient(server_path, name)
		if err != nil {
			log.Println("Couldn't create sockets for", server, "- retrying:", err)
			time.Sleep(time.Second)
		} else {
			return sock
		}
	}
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
			event,
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
					CladRobotObservedFaceToProto(msg.GetRobotObservedFace()),
				},
			}
			sendEventToChannel(event)
		case gw_clad.MessageRobotToExternalTag_RobotChangedObservedFaceID:
			event := &extint.Event{
				EventType: &extint.Event_RobotChangedObservedFaceId{
					CladRobotChangedObservedFaceIDToProto(msg.GetRobotChangedObservedFaceID()),
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

	srv := &http.Server{
		Addr:    demoAddr,
		Handler: grpcHandlerFunc(grpcServer, gwmux),
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
