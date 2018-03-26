package main

import (
	"anki/util"
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"github.com/anki/opus-go/opus"
	wav "github.com/youpy/go-wav"
)

func main() {
	infile := flag.String("in", "", "input wav file")
	outfile := flag.String("out", "", "output prefix filename (will be appended with quality info and .opus)")
	complexity := flag.String("complexity", "10", "encoder complexit(ies) [10 is both default and maximum]")
	bitrate := flag.String("bitrate", "40000", "bitrate(s) [default 40000]")
	frameSize := flag.String("framesize", "20", "frame size(s) [default 20]")
	nuts := flag.Bool("nuts", false, "write a shitload of files of varying complexity/bitrate")
	nowrite := flag.Bool("nowrite", false, "don't write out to a file, just test")
	flag.Parse()

	if *infile == "" || *outfile == "" {
		fmt.Println("Must specify infile and outfile")
		return
	}

	f, err := os.Open(*infile)
	if err != nil {
		fmt.Println("File open error", err)
		return
	}
	reader := wav.NewReader(f)

	var data []int16
	for {
		samples, err := reader.ReadSamples()
		if err != nil {
			if err != io.EOF {
				fmt.Println("Read error:", err)
			}
			break
		}
		for _, sample := range samples {
			data = append(data, int16(reader.IntValue(sample, 0)))
		}
	}
	fmt.Println("Read", len(data), "samples")

	var bitrates []uint
	var complexities []uint
	var frameSizes []float32

	if *nuts {
		frameSizes = opus.FrameSizes
		complexities = []uint{0}
		bitrates = make([]uint, 128-6+1)
		for i := uint(0); i <= 128-6; i++ {
			bitrates[i] = (i + 6) * 1024
		}
	} else {
		frameSizes = parseFloats(*frameSize)
		complexities = parseUints(*complexity)
		bitrates = parseUints(*bitrate)
	}

	for _, comp := range complexities {
		for _, bitr := range bitrates {
			for _, fr := range frameSizes {
				var outbuf []byte
				numEncs := 1.0
				if *nuts {
					numEncs = 10.0
				}
				var encTime float64

				for i := 0; i < int(numEncs); i++ {
					stream := opus.OggStream{SampleRate: 16000, Channels: 1,
						Bitrate: bitr, Complexity: comp, FrameSize: fr}
					encTime += util.TimeFuncMs(func() {
						outbuf = encodeData(&stream, data)
					})
				}

				fname := fmt.Sprintf("%s_comp_%02d_bitr_%06d_fs_%02d.opus",
					*outfile, comp, bitr, int(fr))
				if !*nuts {
					err := writeFile(fname, outbuf)
					if err != nil {
						return
					}
					fmt.Println("Wrote", len(outbuf), "bytes to", fname)
				} else {
					if !*nowrite {
						err := writeFile(fname, outbuf)
						if err != nil {
							return
						}
						fmt.Printf("Wrote to %s (encodes took avg: %6.2f ms) (file %6d bytes)\n",
							fname, encTime/numEncs, len(outbuf))
					} else {
						fmt.Printf("%6.2f ms, %6d bytes: %s\n",
							encTime/numEncs, len(outbuf), fname)
					}
				}
			}
		}
	}
}

func writeFile(filename string, data []byte) error {
	err := ioutil.WriteFile(filename, data, os.ModePerm)
	if err != nil {
		fmt.Println("Error writing to", filename, ":", err)
		return err
	}
	return nil
}

func encodeData(stream *opus.OggStream, buf []int16) []byte {
	outbuf := bytes.Buffer{}
	frameSamples := int(48000.0 * stream.FrameSize / 1000)
	for len(buf) >= frameSamples {
		temp := buf[:frameSamples]
		buf = buf[frameSamples:]

		chunk, err := stream.Encode(temp)
		if err != nil {
			fmt.Println("Write error:", err)
			return []byte{}
		}
		chunk = append(chunk, stream.Flush()...)
		if n, err := outbuf.Write(chunk); n != len(chunk) || err != nil {
			fmt.Println("Write error:", n, "/", len(chunk), err)
		}
	}
	return outbuf.Bytes()
}

func parseFloats(arg string) []float32 {
	strs := strings.Split(arg, ",")
	ret := make([]float32, len(strs))
	for i, str := range strs {
		val, _ := strconv.ParseFloat(str, 32)
		ret[i] = float32(val)
	}
	return ret
}

func parseUints(arg string) []uint {
	strs := strings.Split(arg, ",")
	ret := make([]uint, len(strs))
	for i, str := range strs {
		val, _ := strconv.ParseUint(str, 10, 32)
		ret[i] = uint(val)
	}
	return ret
}
