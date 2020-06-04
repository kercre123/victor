package testclient

import (
	"errors"
	"io"
	"os"

	"github.com/anki/sai-go-util/log"
)

type Data struct {
	AudioData []byte
	NoiseData []byte
}

func ReadFiles(noise, audio string) (*Data, error) {
	noiseData, err := readData(noise)
	if err != nil {
		return nil, errors.New("Error reading noise file. " + err.Error())
	}

	audioData, err := readData(audio)
	if err != nil {
		return nil, errors.New("Error reading audio file. " + err.Error())
	}

	return &Data{
		NoiseData: noiseData,
		AudioData: audioData,
	}, nil
}

func readData(filename string) ([]byte, error) {
	if filename == "" {
		return nil, nil
	}

	fp, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer fp.Close()

	var data, buffer []byte
	for {
		buffer = make([]byte, 2048)
		n, err := fp.Read(buffer)
		if err == io.EOF || n <= 0 {
			break
		} else if err != nil {
			alog.Error{"action": "read_file", "status": "fail", "filename": filename, "error": err}.Log()
		}
		data = append(data, buffer[:]...)
	}
	return data, nil
}
