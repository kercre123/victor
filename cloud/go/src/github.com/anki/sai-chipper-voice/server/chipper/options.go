package chipper

import (
	"github.com/anki/houndify-sdk-go/houndify"

	"github.com/anki/sai-chipper-voice/audiostore"
	"github.com/anki/sai-chipper-voice/client/chipperfn"
	ms "github.com/anki/sai-chipper-voice/conversation/microsoft"
	"github.com/anki/sai-chipper-voice/hstore"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/requestrouter"
)

type ServerOpts func(s *Server)

func WithDialogflow(config, gamesConfig DialogflowConfig) ServerOpts {
	return func(s *Server) {
		if *config.ProjectName != "" {
			s.dialogflowProject = config
			s.dialogflowGamesProject = gamesConfig
			s.intentSvcEnabled[pb.IntentService_DIALOGFLOW] = true
		}
	}
}

func WithMicrosoft(c *ms.LuisClient, opt MicrosoftConfig) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.microsoftConfig = opt
			s.luisClient = c
			s.intentSvcEnabled[pb.IntentService_BING_LUIS] = true

		}
	}
}

func WithAudioStore(c *audiostore.Client) ServerOpts {
	return func(s *Server) {
		s.saveAudio = false
		if c != nil {
			s.audioStoreClient = c
			s.saveAudio = true
		}
	}
}

func WithDas(c *DasClient) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.dasClient = c
		}
	}
}

func WithLex(c *LexClient) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.lexClient = c
			s.intentSvcEnabled[pb.IntentService_LEX] = true
		}
	}
}

func WithHoundify(c *houndify.Client) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.houndifyClient = c
		}
	}
}

func WithChipperFnClient(c chipperfn.Client) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.chipperfnClient = c
		}
	}
}

func WithHypothesisStoreClient(c *hstore.Client) ServerOpts {
	return func(s *Server) {
		if c != nil {
			s.hypostoreClient = c
		}
	}
}

func WithRequestRouter(r *requestrouter.RequestRouter) ServerOpts {
	return func(s *Server) {
		if r != nil {
			s.requestRouter = r
		}
	}
}

func WithLogTranscript(b *bool) ServerOpts {
	return func(s *Server) {
		if b != nil {
			s.logTranscript = *b
		}
	}
}
