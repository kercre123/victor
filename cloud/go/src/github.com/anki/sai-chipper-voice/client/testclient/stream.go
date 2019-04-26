package testclient

import (
	"io"
	"time"

	"github.com/anki/opus-go/opus"
	"github.com/anki/sai-go-util/log"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

const (
	chunkHz       = 10                                      // samples every 100ms
	sampleRate    = 16000                                   // audio sample rate
	sampleBits    = 16                                      // size of each sample
	bufSize       = sampleRate / chunkHz * (sampleBits / 8) // how many bytes will be sent per stream
	sleepTime     = 100 * time.Millisecond                  // upload sleep time to simulate realtime audio
	defaultBootId = "chipper-test-client"
)

type StreamClient struct {
	Stream       pb.ChipperGrpc_StreamingIntentClient
	KgStream     pb.ChipperGrpc_StreamingKnowledgeGraphClient
	AudioStream  chan []byte
	ResultStream chan pb.IntentResult
	Done         chan struct{}
}

func (c *ChipperClient) newStreamClient() *StreamClient {
	stream, err := c.Client.StreamingIntent(c.Ctx)
	if err != nil {
		alog.Error{"action": "create_stream", "status": "fail", "error": err}.Log()
		return nil
	}
	audioStream := make(chan []byte, bufSize)
	streamClient := &StreamClient{
		Stream:      stream,
		AudioStream: audioStream,
		Done:        make(chan struct{}, 1),
	}

	var encoder *opus.OggStream
	if c.Opts.Audio == pb.AudioEncoding_OGG_OPUS {
		encoder = &opus.OggStream{
			SampleRate: 16000,
			Channels:   1,
			FrameSize:  60,
			Bitrate:    66 * 1024,
			Complexity: 0,
		}
	}

	first := true
	go func() {
		for {
			select {
			case <-streamClient.Done:
				alog.Debug{"action": "receive_done_signal"}.Log()
				return
			case data := <-streamClient.AudioStream:
				if encoder != nil {
					// compress
					var err error
					data, err = encoder.EncodeBytes(data)
					if err != nil {
						return
					}
					flushData := encoder.Flush()
					data = append(data, flushData...)
					alog.Debug{"action": "compress", "size": len(data)}.Log()
				}

				if len(data) == 0 {
					continue
				}

				var r *pb.StreamingIntentRequest
				if first {
					r = &pb.StreamingIntentRequest{
						DeviceId:        c.Opts.DeviceId,
						Session:         c.Opts.Session,
						LanguageCode:    c.Opts.LangCode,
						AudioEncoding:   c.Opts.Audio,
						SingleUtterance: c.Opts.SingleUtterance,
						IntentService:   c.Opts.Service,
						AppKey:          c.Opts.AppKey,
						SaveAudio:       c.Opts.SaveAudio,
						Mode:            c.Opts.Mode,
						InputAudio:      data,
						BootId:          defaultBootId,
						FirmwareVersion: c.Opts.Fw,
						SkipDas:         c.Opts.SkipDas,
					}
					first = false
				} else {
					r = &pb.StreamingIntentRequest{
						InputAudio: data,
					}
				}
				err := streamClient.Stream.Send(r)
				if err == io.EOF {
					alog.Info{"action": "client_send_request", "status": "done_streaming"}.Log()
					return
				}
				if err != nil {
					alog.Error{"action": "client_send_request", "status": "unexpected_error", "error": err}.Log()
					return
				}
			}
		}
	}()

	return streamClient

}

func (c *ChipperClient) StreamFromMic(audioStream <-chan []byte, micDone chan struct{}) (*pb.IntentResult, error) {
	// create a new stream client
	streamer := c.newStreamClient()

	// receive results
	waitc := make(chan struct{})
	done := make(chan struct{}, 1)

	result := make(chan *pb.IntentResult, 1)
	streamEnds := make(chan time.Time, 1)

	go getResults(streamer, result, streamEnds, waitc, done)

	counts := 0

sendMicDataLoop:
	for {
		select {
		case <-micDone:
			alog.Info{"action": "audiostream_done", "msg": "mic has stop"}.Log()
			streamer.Done <- struct{}{}
			streamer.Stream.CloseSend()
			streamEnds <- time.Now()
			break sendMicDataLoop
		case <-done:
			alog.Info{"action": "done", "msg": "we have final result"}.Log()
			streamer.Done <- struct{}{}
			streamer.Stream.CloseSend()
			streamEnds <- time.Now()
			break sendMicDataLoop
		case data := <-audioStream:
			streamer.AudioStream <- data
			counts++
			if counts%5 == 0 {
				alog.Info{"action": "sent_mic_audio", "total_frames": counts}.Log()
			}
		default:
		}
	}
	<-waitc
	alog.Info{"action": "close_send"}.Log()
	return <-result, nil
}

func (c *ChipperClient) StreamFromFile(d *Data) (*pb.IntentResult, error) {

	// create a new stream client
	streamer := c.newStreamClient()

	// receive results
	waitc := make(chan struct{})
	done := make(chan struct{}, 1)

	result := make(chan *pb.IntentResult, 1)
	streamEnds := make(chan time.Time, 1)

	go getResults(streamer, result, streamEnds, waitc, done)

	audioData := d.AudioData
	noiseData := d.NoiseData

sendDataLoop:
	for {

		select {
		case <-done:
			streamer.Done <- struct{}{}
			streamEnds <- time.Now()
			break sendDataLoop
		default:

			// send chunks of audio a la BM
			if len(audioData) > bufSize {
				streamer.AudioStream <- audioData[:bufSize] // sending audio
				audioData = audioData[bufSize:]

			} else if len(audioData) > 0 {
				remaining := bufSize - len(audioData)
				data := make([]byte, bufSize)

				copy(data, audioData) // send the last few bytes of audio
				audioData = []byte{}

				copy(data[remaining:], noiseData[:remaining]) // pad noise
				noiseData = noiseData[remaining:]

				streamer.AudioStream <- data

				time.Sleep(5 * time.Millisecond)
				streamEnds <- time.Now()

			} else if len(noiseData) >= bufSize {
				streamer.AudioStream <- noiseData[:bufSize] // just sending noise
				noiseData = noiseData[bufSize:]

			} else {
				streamer.AudioStream <- noiseData[:]
				break sendDataLoop
			}

			time.Sleep(sleepTime) // sleep to simulate real-time audio streaming
		}
	}

	<-waitc
	alog.Info{"action": "close_send"}.Log()
	streamer.Stream.CloseSend()
	return <-result, nil
}

func getResults(
	streamer *StreamClient,
	result chan *pb.IntentResult,
	streamEnds chan time.Time,
	waitc chan struct{},
	done chan struct{}) {

	defer close(waitc)
	for {
		in, err := streamer.Stream.Recv()
		if err == io.EOF {
			alog.Info{"action": "recv_server_result", "status": "stream_done"}.Log()
			result <- nil
			done <- struct{}{} // signal to stop sending data
			return
		}

		if err != nil {
			alog.Error{"action": "recv_server_result", "status": "fail", "error": err}.Log()
			result <- nil
			done <- struct{}{} // signal to stop sending data

			return
		}

		if in.IsFinal {
			result <- in.IntentResult
			done <- struct{}{} // signal to stop sending data
			latency := time.Since(<-streamEnds)
			alog.Info{
				"action":   "intent_result",
				"service":  in.IntentResult.Service,
				"intent":   in.IntentResult.Action,
				"query":    in.IntentResult.QueryText,
				"latency":  float32(latency) / float32(time.Millisecond),
				"audio_id": in.AudioId,
			}.Log()
			return
		}

	}
}
