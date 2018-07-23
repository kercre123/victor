package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"syscall"

	"anki/log"
	gw_clad "clad/gateway"
	extint "proto/external_interface"

	grpcRuntime "github.com/grpc-ecosystem/grpc-gateway/runtime"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

// Enables logs about the requests coming and going from the gateway.
// Most useful for debugging the json output being sent to the app.
const (
	logVerbose              = false
	cladDomainSocket        = "_engine_gateway_server_"
	protoDomainSocket       = "_engine_gateway_proto_server_"
	switchboardDomainSocket = "_switchboard_gateway_server_"
)

var (
	robotHostname      string
	signalHandler      chan os.Signal
	demoKeyPair        *tls.Certificate
	demoCertPool       *x509.CertPool
	switchboardManager SwitchboardIpcManager
	engineProtoManager EngineProtoIpcManager

	// TODO: remove clad socket and map when there are no more clad messages being used
	engineCladManager EngineCladIpcManager
)

func verboseHandlerFunc(grpcServer *grpc.Server, otherHandler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.ProtoMajor == 2 && strings.Contains(r.Header.Get("Content-Type"), "application/grpc") {
			LogRequest(r, "grpc")
			wrap := WrappedResponseWriter{w, "grpc"}
			grpcServer.ServeHTTP(&wrap, r)
		} else {
			LogRequest(r, "json")
			wrap := WrappedResponseWriter{w, "json"}
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

func main() {
	log.Tag = "vic-gateway"
	log.Println("Launching vic-gateway")

	signalHandler = make(chan os.Signal, 1)
	signal.Notify(signalHandler, syscall.SIGTERM)
	go func() {
		sig := <-signalHandler
		log.Println("Received signal:", sig)
		os.Exit(0)
	}()

	if _, err := os.Stat(Cert); os.IsNotExist(err) {
		log.Println("Cannot find cert:", Cert)
		os.Exit(1)
	}
	if _, err := os.Stat(Key); os.IsNotExist(err) {
		log.Println("Cannot find key: ", Key)
		os.Exit(1)
	}

	pair, err := tls.LoadX509KeyPair(Cert, Key)
	if err != nil {
		log.Println("Failed to initialize key pair")
		os.Exit(1)
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
	addr := fmt.Sprintf("localhost:%d", Port)

	engineCladManager.ManagedChannels = make(map[gw_clad.MessageRobotToExternalTag]([]chan<- gw_clad.MessageRobotToExternal))
	closeEngineClad := engineCladManager.Connect(SocketPath, cladDomainSocket)
	defer closeEngineClad()

	engineProtoManager.ManagedChannels = make(map[string]([]chan<- extint.GatewayWrapper))
	closeEngineProto := engineProtoManager.Connect(SocketPath, protoDomainSocket)
	defer closeEngineProto()

	// if IsSwitchboardAvailable {
	// 	engineProtoManager.ManagedChannels = make(map[string]([]chan<- gw_clad.MessageSwitchboardGateway))
	// 	closeSwitchboardClad := engineCladManager.Connect(SocketPath, switchboardDomainSocket)
	// 	defer closeSwitchboardClad()
	// }

	log.Println("Sockets successfully created")

	creds, err := credentials.NewServerTLSFromFile(Cert, Key)
	if err != nil {
		log.Println("Error creating server tls:", err)
		os.Exit(1)
	}
	grpcServer := grpc.NewServer(grpc.Creds(creds))
	extint.RegisterExternalInterfaceServer(grpcServer, newServer())
	ctx := context.Background()

	if runtime.GOOS == "darwin" {
		robotHostname = "Vector-Local"
	} else {
		robotHostname, err = os.Hostname()
		if err != nil {
			log.Println("Failed to get Hostname:", err)
			os.Exit(1)
		}
	}

	log.Println("Hostname:", robotHostname)

	dcreds := credentials.NewTLS(&tls.Config{
		ServerName:   robotHostname,
		Certificates: []tls.Certificate{*demoKeyPair},
		RootCAs:      demoCertPool,
	})
	dopts := []grpc.DialOption{grpc.WithTransportCredentials(dcreds)}

	gwmux := grpcRuntime.NewServeMux(grpcRuntime.WithMarshalerOption(grpcRuntime.MIMEWildcard, &grpcRuntime.JSONPb{EmitDefaults: true, OrigName: true, EnumsAsInts: true}))
	err = extint.RegisterExternalInterfaceHandlerFromEndpoint(ctx, gwmux, addr, dopts)
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
		Addr:    addr,
		Handler: handlerFunc,
		TLSConfig: &tls.Config{
			Certificates: []tls.Certificate{*demoKeyPair},
			NextProtos:   []string{"h2"},
		},
	}

	go engineCladManager.ProcessMessages()
	go engineProtoManager.ProcessMessages()
	if IsSwitchboardAvailable {
		go switchboardManager.ProcessMessages()
	}

	log.Println("Listening on Port:", Port)
	err = srv.Serve(tls.NewListener(conn, srv.TLSConfig))

	if err != http.ErrServerClosed {
		log.Println("Error during Serve:", err)
		os.Exit(1)
	}
	log.Println("Closing vic-gateway")
	return
}
