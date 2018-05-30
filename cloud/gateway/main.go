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
	"strings"
	"time"

	"anki/ipc"
	"anki/log"
	"clad/cloud"
	extint "proto/external_interface"

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

// TODO: improve the logic for this to allow for multiple simultaneous requests
func processEngineMessages() {
	var msg cloud.MessageRobotToExternal
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
			continue
		}
		log.Println("Action Completed!", msg)
		if c, ok := engineChanMap[msg.Tag()]; ok {
			c <- RobotToExternalResult{Message: msg}
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

	engineSock = getSocketWithRetry("/dev/socket/", "_engine_gateway_server_", "client")
	defer engineSock.Close()
	engineChanMap = make(map[cloud.MessageRobotToExternalTag](chan RobotToExternalResult))

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

	gwmux := runtime.NewServeMux()
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
