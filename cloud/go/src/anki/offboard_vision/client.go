package offboard_vision

import (
  "anki/config"
  "anki/ipc"
  "anki/log"
  "anki/util"
  "anki/util/cvars"
  "bytes"
  "clad/vision"
  "context"
  "fmt"
  "io/ioutil"

  pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
  "github.com/google/uuid"
  "github.com/gwatts/rootcerts"
  "google.golang.org/grpc"
  "google.golang.org/grpc/credentials"
)

var deviceID = "mac-build"
var defaultGroupName = "offboard_vision"

func init() {
  cvars.AddString("FaceGroup", func(x string) interface{} {
    defaultGroupName = x
    return x
  }, func() interface{} {
    return defaultGroupName
  })
}

type client struct {
  ipc.Conn
}

var (
  defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

var devURLReader func(string) ([]byte, error, bool)

func (c *client) handleConn(ctx context.Context) {
  for {
    msgbuf := c.ReadBlock()
    if msgbuf == nil || len(msgbuf) == 0 {
      return
    }
    var msg vision.OffboardImageReady
    if err := msg.Unpack(bytes.NewBuffer(msgbuf)); err != nil {
      log.Println("Could not unpack offboard vision request:", err)
      continue
    }

    resp, err := c.handleRequest(ctx, &msg)
    if err != nil {
      log.Println("Error handling offboard vision request:", err)
    }

    var buf bytes.Buffer
    if err := resp.Pack(&buf); err != nil {
      log.Println("Error packing offboard vision response:", err)
    } else if n, err := c.Write(buf.Bytes()); n != buf.Len() || err != nil {
      log.Println("Error sending offboard vision response:", fmt.Sprintf("%d/%d,", n, buf.Len()), err)
    }
  }
}

func (c *client) handleRequest(ctx context.Context, msg *vision.OffboardImageReady) (*vision.OffboardResultReady, error) {
  var dialOpts []grpc.DialOption
  dialOpts = append(dialOpts, util.CommonGRPC()...)
  dialOpts = append(dialOpts, grpc.WithInsecure())

  var wg util.SyncGroup

  // dial server and read file data in parallel
  var rpcConn *grpc.ClientConn
  var rpcErr error
  rpcClose := func() error { return nil }

  var fileData []byte
  var fileErr error

  // dial server, make it blocking
  wg.AddFunc(func() {
    rpcConn, rpcErr = grpc.DialContext(ctx, config.Env.OffboardVision, append(dialOpts, grpc.WithBlock())...)
    if rpcErr == nil {
      rpcClose = rpcConn.Close
    }
  })

  // read file data
  wg.AddFunc(func() {
    if devURLReader != nil {
      var handled bool
      if fileData, fileErr, handled = devURLReader(msg.Filename); handled {
        return
      }
    }
    fileData, fileErr = ioutil.ReadFile(msg.Filename)
  })

  // wait for both routines above to finish
  wg.Wait()
  // if rpc connection didn't fail, this will be set to rpcConn.Close()
  defer rpcClose()

  err := util.NewErrors(rpcErr, fileErr).Error()
  if err != nil {
    return nil, err
  }

  // TODO: Actually read this in instead of hard coding it. (VIC-13955)
  var modes = []string{"people", "faces"}

  sessionID := uuid.New().String()[:16]
  r := &pb.ImageRequest{
    Session:      sessionID,
    DeviceId:     deviceID,
    Lang:         "en",
    ImageData:    fileData,
    TimestampMs:  msg.Timestamp,
    Modes:        modes,
  }
  // todo: possibly not hardcode this?
  r.Configs = &pb.ImageConfig{}
  r.Configs.GroupName = defaultGroupName

  client := pb.NewChipperGrpcClient(rpcConn)
  resp, err := client.AnalyzeImage(ctx, r)
  if err != nil {
    log.Println("image analysis error: ", err)
    return nil, err
  }
  log.Println("image analysis response: ", resp.String())

  var resultReady vision.OffboardResultReady
  resultReady.JsonResult = resp.RawResult
  resultReady.Timestamp = resp.TimestampMs

  return &resultReady, nil
}
