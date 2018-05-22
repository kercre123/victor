package main

import (
	"bytes"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"strings"
	"time"

	"anki/ipc"
	"clad/cloud"
	"proto/external_interface"

	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

const (
	Key  = "/data/trust.key"
	Cert = "/data/trust.cert"
	Port = 443
)

type RobotToExternalResult struct {
	Message cloud.MessageRobotToExternal
	Error   error
}

var (
	demoKeyPair   *tls.Certificate
	demoCertPool  *x509.CertPool
	demoAddr      string
	engineSock    ipc.Conn
	engineChanMap map[cloud.MessageRobotToExternalTag](chan RobotToExternalResult)
)

func getSocketWithRetry(client string, server string) ipc.Conn {
	for {
		sock, err := ipc.NewEngineUnixgramClient("/dev/socket", client, server)
		if err != nil {
			fmt.Println("Couldn't create sockets", client, ",", server, "- retrying:", err)
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

// TODO: improve the logic for this to allow for multiple simultaneous requests
func processEngineMessages() {
	var msg cloud.MessageRobotToExternal
	var b []byte
	buf := bytes.NewBuffer(make([]byte, 1))
	for true {
		buf.Reset()
		b = engineSock.ReadBlock()[2:] // dropping the length from the start
		buf.Write(b)
		if err := msg.Unpack(buf); err != nil {
			continue
		}
		fmt.Println("Action Completed!", msg)
		if c, ok := engineChanMap[msg.Tag()]; ok {
			c <- RobotToExternalResult{Message: msg}
		}
	}
}

func main() {
	fmt.Println("Launching vic-gateway")

	var err error
	pair, err := tls.LoadX509KeyPair(Cert, Key)
	if err != nil {
		panic(err)
	}
	demoKeyPair = &pair
	demoCertPool = x509.NewCertPool()
	caCert, err := ioutil.ReadFile(Cert)
	if err != nil {
		log.Fatal(err)
	}
	ok := demoCertPool.AppendCertsFromPEM(caCert)
	if !ok {
		panic("bad certs")
	}
	demoAddr = fmt.Sprintf("localhost:%d", Port)

	engineSock = getSocketWithRetry("_engine_gateway_client_", "_engine_gateway_server_")
	defer engineSock.Close()
	engineChanMap = make(map[cloud.MessageRobotToExternalTag](chan RobotToExternalResult))

	fmt.Println("Sockets successfully created")

	creds, _ := credentials.NewServerTLSFromFile(Cert, Key)
	grpcServer := grpc.NewServer(grpc.Creds(creds))
	external_interface.RegisterExternalInterfaceServer(grpcServer, newServer())
	ctx := context.Background()

	dcreds := credentials.NewTLS(&tls.Config{
		ServerName:   demoAddr,
		Certificates: []tls.Certificate{*demoKeyPair},
		RootCAs:      demoCertPool,
	})
	dopts := []grpc.DialOption{grpc.WithTransportCredentials(dcreds)}

	gwmux := runtime.NewServeMux()
	err = external_interface.RegisterExternalInterfaceHandlerFromEndpoint(ctx, gwmux, demoAddr, dopts)
	if err != nil {
		fmt.Printf("serve: %v\n", err)
		return
	}

	conn, err := net.Listen("tcp", fmt.Sprintf(":%d", Port))
	if err != nil {
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

	go processEngineMessages()

	fmt.Printf("grpc on Port: %d\n", Port)
	err = srv.Serve(tls.NewListener(conn, srv.TLSConfig))

	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}

	return
}
