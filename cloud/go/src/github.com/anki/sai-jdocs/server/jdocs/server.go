package jdocssvc

import (
	"github.com/anki/sai-jdocs/jdocs"
	"github.com/anki/sai-jdocs/proto/jdocspb"

	"github.com/aws/aws-sdk-go/aws"
	"golang.org/x/net/context"
)

type Server struct {
	api *jdocs.API
}

type ServerOpt func(*Server)

func NewServer(opts ...ServerOpt) *Server {
	s := &Server{}
	for _, opt := range opts {
		opt(s)
	}
	s.api = jdocs.NewAPI()
	// XXX: s.api needs to be initialized separately
	return s
}

func (s *Server) InitAPI(docSpecsJson, tablePrefix string, cfg *aws.Config) error {
	return s.api.Init(docSpecsJson, tablePrefix, cfg)
}

func (s *Server) CheckTables() error {
	return s.api.CheckTables()
}

//////////////////////////////////////////////////////////////////////
// gRPC Endpoint --> API Functions
//////////////////////////////////////////////////////////////////////

// XXX: Until proper authentication + authorization is in place, allow all
// clients to do any CRUD actions.
const defaultClientType = jdocs.ClientType(jdocs.ClientTypeSuperuser)

func (s *Server) Echo(ctx context.Context, in *jdocspb.EchoReq) (*jdocspb.EchoResp, error) {
	return s.api.Echo(in)
}

func (s *Server) WriteDoc(ctx context.Context, wreq *jdocspb.WriteDocReq) (*jdocspb.WriteDocResp, error) {
	return s.api.WriteDoc(defaultClientType, wreq)
}

func (s *Server) ReadDocs(ctx context.Context, rreq *jdocspb.ReadDocsReq) (*jdocspb.ReadDocsResp, error) {
	return s.api.ReadDocs(defaultClientType, rreq)
}

func (s *Server) DeleteDoc(ctx context.Context, dreq *jdocspb.DeleteDocReq) (*jdocspb.DeleteDocResp, error) {
	return s.api.DeleteDoc(defaultClientType, dreq)
}
