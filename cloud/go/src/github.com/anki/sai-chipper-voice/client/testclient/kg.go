package testclient

import (
	"io"
	"time"

	"github.com/anki/sai-go-util/log"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

func (c *ChipperClient) newKGStreamClient() *StreamClient {
	stream, err := c.Client.StreamingKnowledgeGraph(c.Ctx)
	if err != nil {
		alog.Error{"action": "create_stream", "status": "fail", "error": err}.Log()
		return nil
	}
	audioStream := make(chan []byte, bufSize)
	streamClient := &StreamClient{
		KgStream:    stream,
		AudioStream: audioStream,
		Done:        make(chan struct{}, 1),
	}

	go func() {
		for {
			select {
			case <-streamClient.Done:
				alog.Debug{"action": "receive_done_signal"}.Log()
				return
			case data := <-streamClient.AudioStream:
				r := &pb.StreamingKnowledgeGraphRequest{
					Session:         c.Opts.Session,
					DeviceId:        c.Opts.DeviceId,
					LanguageCode:    pb.LanguageCode_ENGLISH_US,
					AudioEncoding:   c.Opts.Audio,
					FirmwareVersion: c.Opts.Fw,
					AppKey:          c.Opts.AppKey,
					SaveAudio:       c.Opts.SaveAudio,
					InputAudio:      data,
					SkipDas:         c.Opts.SkipDas,
					BootId:          defaultBootId,
				}
				err := streamClient.KgStream.Send(r)
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

func (c *ChipperClient) KnowledgeGraphFromMic(audioStream <-chan []byte, micDone chan struct{}) (*pb.KnowledgeGraphResponse, error) {

	// create a new stream client
	kgStreamClient := c.newKGStreamClient()

	// receive results
	waitc := make(chan struct{})
	done := make(chan struct{}, 1)

	result := make(chan *pb.KnowledgeGraphResponse, 1)
	streamEnds := make(chan time.Time, 1)

	go getKGResult(kgStreamClient, result, streamEnds, waitc, done)

	counts := 0
sendMicDataLoop:
	for {
		select {
		case <-micDone:
			alog.Info{"action": "audiostream_done", "msg": "mic has stop sending data"}.Log()
			streamEnds <- time.Now()
			kgStreamClient.KgStream.CloseSend()
			kgStreamClient.Done <- struct{}{}
			break sendMicDataLoop
		case <-done:
			alog.Info{"action": "done", "msg": "we have result, be done"}.Log()
			streamEnds <- time.Now()
			kgStreamClient.KgStream.CloseSend()
			kgStreamClient.Done <- struct{}{}
			break sendMicDataLoop
		case data := <-audioStream:
			kgStreamClient.AudioStream <- data
			counts++
			if counts%10 == 0 {
				alog.Info{"action": "sent_mic_audio", "total_frames": counts}.Log()
			}
		default:
		}
	}

	<-waitc
	return <-result, nil
}

func (c *ChipperClient) KnowledgeGraph(d *Data) (*pb.KnowledgeGraphResponse, error) {

	// create a new stream client
	kgStreamClient := c.newKGStreamClient()

	// receive results
	waitc := make(chan struct{})
	done := make(chan struct{}, 1)

	result := make(chan *pb.KnowledgeGraphResponse, 1)
	streamEnds := make(chan time.Time, 1)

	go func() {
		in, err := kgStreamClient.KgStream.Recv()
		if err == io.EOF {
			alog.Info{"action": "recv_server_result", "status": "stream_done"}.Log()
			result <- nil
		}

		if err != nil {
			alog.Error{"action": "recv_server_result", "status": "fail", "error": err}.Log()
			result <- nil
		}

		result <- in
		done <- struct{}{} // signal to stop sending data
		latency := time.Since(<-streamEnds)
		alog.Info{
			"action":     "kg_result",
			"query_type": "stream_file",
			"query":      in.QueryText,
			"spoken":     in.SpokenText,
			"latency":    float32(latency) / float32(time.Millisecond),
			"audio_id":   in.AudioId,
		}.Log()
		close(waitc)
	}()

	audioData := d.AudioData
	noiseData := d.NoiseData

sendDataLoop:
	for {

		select {
		case <-done:
			kgStreamClient.Done <- struct{}{}
			streamEnds <- time.Now()
			break sendDataLoop
		default:

			// send chunks of audio a la BM
			if len(audioData) > bufSize {
				kgStreamClient.AudioStream <- audioData[:bufSize] // sending audio
				audioData = audioData[bufSize:]

			} else if len(audioData) > 0 {
				remaining := bufSize - len(audioData)
				data := make([]byte, bufSize)

				copy(data, audioData) // send the last few bytes of audio
				audioData = []byte{}

				copy(data[remaining:], noiseData[:remaining]) // pad noise
				noiseData = noiseData[remaining:]

				kgStreamClient.AudioStream <- data

				time.Sleep(5 * time.Millisecond)
				streamEnds <- time.Now()

			} else if len(noiseData) >= bufSize {
				kgStreamClient.AudioStream <- noiseData[:bufSize] // just sending noise
				noiseData = noiseData[bufSize:]

			} else {
				kgStreamClient.AudioStream <- noiseData[:]
				break sendDataLoop
			}

			time.Sleep(sleepTime) // sleep to simulate real-time audio streaming
		}
	}

	<-waitc
	alog.Info{"action": "closing_kg_stream"}.Log()
	kgStreamClient.KgStream.CloseSend()
	return <-result, nil
}

func getKGResult(
	streamer *StreamClient,
	result chan *pb.KnowledgeGraphResponse,
	streamEnds chan time.Time,
	waitc chan struct{},
	done chan struct{}) {

	defer close(waitc)
	in, err := streamer.KgStream.Recv()

	if err == io.EOF {
		alog.Info{"action": "recv_server_result", "status": "stream_done"}.Log()
		// result <- nil
	}

	if err != nil {
		if grpc.Code(err) != codes.DeadlineExceeded {
			alog.Error{
				"action": "recv_server_result",
				"status": "fail",
				"error":  err,
			}.Log()
		}
		alog.Error{
			"action": "recv_server_result",
			"status": "timeout",
			"error":  err,
			"result": in,
		}.Log()
		// result <- nil
	}

	result <- in
	done <- struct{}{} // signal to stop sending data
	if in != nil {
		latency := time.Since(<-streamEnds)
		alog.Info{
			"action":     "kg_result",
			"query_type": "stream_mic",
			"query":      in.QueryText,
			"spoken":     in.SpokenText,
			"latency":    float32(latency) / float32(time.Millisecond),
			"audio_id":   in.AudioId,
		}.Log()
	}

	alog.Info{"action": "exiting_kg"}.Log()
}
