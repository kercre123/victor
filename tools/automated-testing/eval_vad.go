package main

import (
    "encoding/csv"
    "encoding/json"
    "fmt"
    "log"
    "math"
    "net/url"
    "os"
    "os/signal"
    "path/filepath"
    "strconv"
    "strings"
    "sync"
    "time"

    // for mixing and getting callbacks of audio    
    "github.com/faiface/beep"
    "github.com/faiface/beep/effects"
    "github.com/faiface/beep/speaker"
    "github.com/faiface/beep/wav"

    // for checking whether a file is not silent
    "github.com/mjibson/go-dsp/spectral"

    // note: this is not made for concurrent reads or concurrent writes
    "github.com/gorilla/websocket"
)

const (
    kTimeBetweenJobs = 10.0
    kAudioThreshold = 1e-10
)

type TimeInfo struct {
    sentTime int64
    receivedTime int64
    roundTrip_ms int64
    robotTime int64
}
var referenceTimes []TimeInfo

func GetTime() int64 {
    return time.Now().UnixNano() / int64(time.Millisecond)
}

func FloatToStr(input float64) string {
    return strconv.FormatFloat(input, 'f', -1, 64)
}


type Source int
const (
   Silence  Source = iota
   Noise
   Music
   Birds
   Doorbell
   Voice
)

// TODO: add tone??

func (source Source) String() string {
    names := [...]string {
        "Silence", 
        "Noise", 
        "Music", 
        "Birds",
        "Doorbell",
        "Voice"}
    if source < Silence {
        return "Unknown"
    } else if source >= Voice {
        return names[Voice] + strconv.Itoa(int(source - Voice))
    } else {
        return names[source]
    }
}

var voiceFiles []string

func (source Source) Filename() string {
    filenames := [...]string {
        "<SILENCE>", 
        "samples/noise.wav", 
        "samples/music.wav", 
        "samples/bird.wav", 
        "samples/doorbell.wav"}
    if source < Silence {
        return "Unknown"
    } else if source >= Voice {
        if len(voiceFiles) == 0 || int(source) > int(Voice)+len(voiceFiles) {
            log.Fatalln("No source file for source ", source.String())
        }
        return voiceFiles[int(source - Voice)]
    } else {
        return filenames[source]
    }
}

// sources: array of source types
// patterns: array of pattern arrays.
//   {0.0} is just playing the clip. 
//   {2.5, 1.0, 3.0, 2.0, -1.0} is {play for 2.5, silence for 1.0, 
//                                  play for 3, silence for 2, then loop
//                                  forever starting from ofset 3.5}. 
//   If any source's last entry is negative, the master mix will terminate when 
//   the shortest pattern ends. Otherwise it ends when the longest source ends.
type Job struct {
    sources  []Source
    patterns [][]float64 // see note above
    gains    []float64
    orders   []bool // true: starts with sample, false: starts w silence 
}

func (job *Job) Description() []string {
    var descs []string
    for idx := range job.sources {
        var pat []string
        for _,f := range job.patterns[idx] {
            pat = append(pat, FloatToStr(f))
        }
        str := job.sources[idx].String() + " (Gain=" + FloatToStr(job.gains[idx]) + ", pat=" + strings.Join(pat, " ") + ")"
        descs = append(descs, str)
    }
    return descs
}

func (job *Job) Add( source Source, pattern []float64, gain float64 ) {
    job.sources = append(job.sources, source)
    job.patterns = append(job.patterns, pattern)
    job.gains = append(job.gains, gain)
    job.orders = append(job.orders, true)
}
func (job *Job) Clear() {
    job.sources = nil
    job.patterns = nil
    job.gains = nil
    job.orders = nil
}


func MakeJobs() []Job {
    var jobs []Job
    var job Job
    

    gains := []float64{0.0, -0.5, -0.75}
    
    // voice
    for idx := range voiceFiles {
        for _,gain := range gains {
            job.Clear()
            job.Add( Source(int(Voice)+idx), []float64{0.0}, gain )
            jobs = append(jobs, job)
        }
    }

    // {noise, music} x {constant, variable} x {pause/duration}
    sources := []Source{Noise, Music}
    for _,source := range sources {
        for _,gain := range gains {
            // constant
            job.Clear()
            job.Add( source, []float64{15.0}, gain )
            jobs = append(jobs, job)

            // variable
            duration := []float64{10.0, 5.0, 2.0, 1.0, 0.5}
            pause := []float64{10.0, 5.0, 2.0, 1.0, 0.5}
            for _, d := range( duration ) {
                for _, p := range( pause ) {
                    job.Clear()
                    job.Add( source, []float64{d, p, d, p, d}, gain )
                    jobs = append(jobs, job)
                }
            }
        }
    }

    // bird
    for _,gain := range gains {
        job.Clear()
        job.Add( Birds, []float64{0.0}, gain )
        jobs = append(jobs, job)
    }

    // doorbell
    for _,gain := range gains {
        job.Clear()
        job.Add( Doorbell, []float64{0.0}, gain )
        jobs = append(jobs, job)
    }

    // voice + noise
    noiseGain := []float64{-0.9, -0.6, -0.3, 0.0}
    for _,gain := range noiseGain {
        for idx := range voiceFiles {
            job.Clear()
            job.Add( Source(int(Voice)+idx), []float64{5.0, 0.0, 5.0}, 0.0 )
            job.orders[0] = false
            job.Add( Noise, []float64{-1.0}, gain)
            jobs = append(jobs, job)
        }
    }

    // voice + looping music
    for _,gain := range noiseGain {
        for idx := range voiceFiles {
            job.Clear()
            job.Add( Source(int(Voice)+idx), []float64{5.0, 0.0, 5.0}, 0.0 )
            job.orders[0] = false
            job.Add( Music, []float64{-1.0}, gain)
            jobs = append(jobs, job)
        }
    }

    // voice + looping bird
    for _,gain := range noiseGain {
        for idx := range voiceFiles {
            job.Clear()
            job.Add( Source(int(Voice)+idx), []float64{5.0, 0.0, 5.0}, 0.0 )
            job.orders[0] = false
            job.Add( Birds, []float64{-1.0}, gain)
            jobs = append(jobs, job)
        }
    }

    // voice + looping doorbell
    for _,gain := range noiseGain {
        for idx := range voiceFiles {
            job.Clear()
            job.Add( Source(int(Voice)+idx), []float64{5.0, 0.0, 5.0}, 0.0 )
            job.orders[0] = false
            job.Add( Doorbell, []float64{-1.0}, gain)
            jobs = append(jobs, job)
        }
    }

    return jobs
}

func min(a, b int) int {
    if a < b {
        return a
    }
    return b
}

var csvFile *csv.Writer
var csvMux sync.Mutex

type FFTStreamer struct {
    Streamer beep.Streamer
    SampleRate beep.SampleRate
    Value int
    Channel int
}

func NewFFTStreamer(streamer beep.Streamer, sampleRate beep.SampleRate, channel int) *FFTStreamer {
    return &FFTStreamer{Streamer: streamer, SampleRate: sampleRate, Value: -1, Channel: channel}
}

func (streamer *FFTStreamer) Stream(samples [][2]float64) (n int, ok bool) {
    n, ok = streamer.Streamer.Stream(samples)
    // for i := range samples[:n] {
    //     mix := (samples[i][0] + samples[i][1]) / 2
    //     samples[i][0], samples[i][1] = mix, mix
    // }
    monoSamples := make([]float64, n)
    for i := range samples[:n] {
        monoSamples[i] = (samples[i][0] + samples[i][1])/2
    }
    Pxx, _ := spectral.Pwelch(monoSamples, float64(streamer.SampleRate), &spectral.PwelchOptions{})
    var sum float64
    max := -1.0
    for _, pxx := range Pxx {
        sum += pxx
        if pxx > max {
            max = pxx
        }
    }
    avg := sum / float64(len(Pxx))
    vad := (avg >= kAudioThreshold)
    if vad != (streamer.Value == 1) {
        newVal := 1
        if !vad {
            newVal = 0
        }
        streamer.Value = newVal
        if streamer.Channel == 0 {
            fmt.Println("audioVad", newVal, avg, max)
        }
        
        lineTime := GetTime()
        out := []string{"audioVad", strconv.FormatInt(lineTime), strconv.Itoa(streamer.Channel), strconv.Itoa(newVal)}
        csvMux.Lock()
        csvFile.Write(out)
        csvMux.Unlock()
    }
    return n, ok
}

func (streamer *FFTStreamer) Err() error {
    return streamer.Streamer.Err()
}

type FlatLineStreamer struct {
    Streamer beep.Streamer
    Value int
    Channel int
    PrevValue int
}

func NewFlatLineStreamer(streamer beep.Streamer, channel int) *FlatLineStreamer {
    return &FlatLineStreamer{Streamer: streamer, Value: -1, PrevValue:-1, Channel: channel}
}

func (streamer *FlatLineStreamer) Stream(samples [][2]float64) (n int, ok bool) {
    n, ok = streamer.Streamer.Stream(samples)

    // f, err := os.OpenFile("access.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
    // if err != nil {
    //     log.Fatal(err)
    // }

    monoSamples := make([]float64, n)
    for i := range samples[:n] {
        monoSamples[i] = (samples[i][0] + samples[i][1])/2
        // if _, err := f.Write([]byte(FloatToStr(monoSamples[i]) + "\n")); err != nil {
        //     log.Fatal(err)
        // }
    }

    // if err := f.Close(); err != nil {
    //     log.Fatal(err)
    // }

    newVal := 0
    max := math.Inf(-1)
    min := math.Inf(1)
    for _,amp := range monoSamples {
        if amp > max {
            max = amp
        }
        if amp < min {
            min = amp
        }
    }
    if max-min > 0.01 {
        newVal = 1
    }
    if streamer.PrevValue == newVal && newVal != streamer.Value {
        streamer.Value = newVal
        lineTime := GetTime()
        out := []string{"audioVad", strconv.FormatInt(lineTime), strconv.Itoa(streamer.Channel), strconv.Itoa(newVal)}
        csvMux.Lock()
        csvFile.Write(out)
        csvMux.Unlock()
    }
    streamer.PrevValue = newVal
    return n, ok
}

func (streamer *FlatLineStreamer) Err() error {
    return streamer.Streamer.Err()
}

func GetCSVCallback(started bool, str string, extraParams ...int) beep.Streamer {
    return beep.Callback(func() {
        lineTime := GetTime()
        startStr := "audioStart"
        if !started {
            startStr = "audioEnd"
        }
        out := []string{startStr, strconv.FormatInt(lineTime), str}
        for _,param := range extraParams {
            out = append(out, strconv.Itoa(param))
        }
        
        csvMux.Lock()
        csvFile.Write(out)
        csvMux.Unlock()
    })
}

var inputFormat beep.Format
func runJob( job Job ) {

    fmt.Println("source =", job.sources[0].Filename())

    var channels []beep.Streamer

    done := make(chan bool)

    numInfiniteLoops := 0

    if len(job.sources)>1 && job.sources[0] == Silence {
        log.Fatalln("First source cant be silence")
    }

    for idx, source := range job.sources {

        var buffer *beep.Buffer

        if source == Silence {

            buffer = beep.NewBuffer(inputFormat)
            buffer.Append(beep.Silence(int(inputFormat.SampleRate*1000)))

        } else {


            f, err := os.Open( source.Filename() )
            if err != nil {
                log.Fatal(err)
            }

            decodedStreamer, format, err := wav.Decode(f)
            if err != nil {
                log.Fatal(err)
            }
            
            if idx == 0 {
                inputFormat = format
                speaker.Init(format.SampleRate, format.SampleRate.N(time.Second/10))
            } else {
                if format.SampleRate != inputFormat.SampleRate {
                    log.Fatalln("Format differs")
                }
                inputFormat = format
            }

            // get buffer for streamer
            buffer = beep.NewBuffer(format)
            buffer.Append(decodedStreamer)
            decodedStreamer.Close()
        }
        
        

        var sequence []beep.Streamer

        var sampleOffset int
        sampleOffset = 0
        
        for patternIdx, patternTime := range job.patterns[idx] {
            if math.Abs(patternTime) <= 1e-5 {
                numSamples := buffer.Len()
                sequence = append(sequence, 
                                  GetCSVCallback(true, source.String(), idx, numSamples), 
                                  buffer.Streamer(0, buffer.Len()),
                                  GetCSVCallback(false, source.String(), idx))
            } else if (patternIdx % 2 == 1 && job.orders[idx]) || (patternIdx % 2 == 0 && !job.orders[idx]) {
                // make silence
                numSamples := int(patternTime * float64(inputFormat.SampleRate))
                // todo: fade out/in. needs a custom streamer to replace Seq but would probably simplify this code
                sequence = append(sequence, 
                                  GetCSVCallback(true,"silence", idx, numSamples), 
                                  beep.Silence(numSamples), 
                                  GetCSVCallback(false, "silence", idx))
                fmt.Println("Adding silence for ", numSamples)
            } else if patternTime < 0.0 {
                current := buffer.Streamer(sampleOffset, buffer.Len())
                looped := beep.Loop(-1, buffer.Streamer(0, buffer.Len()))
                sequence = append(sequence, 
                                  GetCSVCallback(true,source.String(), idx, -1),
                                  current, 
                                  looped)
                numInfiniteLoops += 1
                if patternIdx != len(job.patterns[idx]) - 1 {
                    fmt.Println("WARNING: Source", idx, "has infinitely repeating portion before the end")
                }
                fmt.Println("Looping forever")
                break
            } else {
                numSamples := int(patternTime * float64(inputFormat.SampleRate))
                
                sequence = append(sequence, GetCSVCallback(true,source.String(),idx, numSamples))
                
                for numSamples > 0 {
                    toRead := min(buffer.Len() - sampleOffset, numSamples)
                    fmt.Println("Adding clip from", sampleOffset, "to", sampleOffset + toRead)
                    streamer := buffer.Streamer(sampleOffset, sampleOffset+toRead)
                    sequence = append(sequence, streamer)
                    sampleOffset += toRead
                    numSamples -= toRead
                    if sampleOffset == buffer.Len() {
                        sampleOffset = 0
                    }
                }
                sequence = append(sequence, GetCSVCallback(false,source.String(), idx))
            }
            fmt.Println(patternTime)
        } // for each play duration

        channel := beep.Seq(sequence...)
        mixedChannel := &effects.Gain{ 
            Streamer: channel, Gain: job.gains[idx],
        }
        //fftStreamer := NewFFTStreamer(mixedChannel, inputFormat.SampleRate, idx)
        fftStreamer := NewFlatLineStreamer(mixedChannel, idx)
        channels = append(channels, fftStreamer)
    }

    
    // if no tracks loop forever, then end when all tracks end
    var toPlay beep.Streamer
    if numInfiniteLoops == 0 {
        master := beep.Mix(channels...)
        toPlay = beep.Seq(master, beep.Callback(func() {
            fmt.Println("All done")
            done <- true
        }))
    } else {
        for chanIdx := range channels {
            channels[chanIdx] = beep.Seq(channels[chanIdx], beep.Callback(func() {
                numInfiniteLoops -= 1
                    if numInfiniteLoops <= 0 {
                        fmt.Println("Non-looping done")
                        // todo: this can now be called multiple times. fix this
                        done <- true
                    }
            }) )
        }
        toPlay = beep.Mix(channels...)
    }
    speaker.Play(toPlay)

    for {
        select {
        case <-done:
            speaker.Clear()
            return
        }
    }
}

func main() {

    if len(os.Args) < 4 {
        log.Fatalln("usage: ", os.Args[0], " <voice directory> <robot IP> <output csv>")
    }

    voiceDir := os.Args[1]
    robotIP := os.Args[2]
    outCSV := os.Args[3]

    if len(voiceDir) == 0 || len(robotIP) == 0 || len(outCSV) == 0 {
        log.Fatalln("usage: ", os.Args[0], " <voice directory> <robot IP> <output csv>")
    }

    // list files in voice path
    err := filepath.Walk(voiceDir, func(path string, info os.FileInfo, err error) error {
        if info.IsDir() {
            return nil
        }
        voiceFiles = append(voiceFiles, path)
        return nil
    })
    if err != nil {
        panic(err)
    }
    fmt.Println("Using",len(voiceFiles),"files for voice")

    // try to connect to robot
    interrupt := make(chan os.Signal, 1)
    signal.Notify(interrupt, os.Interrupt)
    u := url.URL{Scheme: "ws", Host: robotIP + ":8889", Path: "/socket"}
    fmt.Printf("connecting to %s", u.String())
    c, resp, err := websocket.DefaultDialer.Dial(u.String(), nil)
    if err == websocket.ErrBadHandshake {
         fmt.Printf("handshake failed with status %d", resp.StatusCode)
    } else if err != nil {
        log.Fatal("dial:", err)
    }
    if c == nil {
        log.Fatalln("Could not connect to websocket")
    }
    defer c.Close()
    fmt.Println("Connected to socket")
    subscribeData := `{"type": "subscribe", "module": "vad"}`
    err = c.WriteMessage(websocket.TextMessage, []byte(subscribeData))
    if err != nil {
        log.Fatalln("error writing:", err)
    }
    fmt.Println("Subscribed")

    f, err := os.OpenFile(outCSV, os.O_WRONLY|os.O_CREATE, 0644)
    if err != nil {
        log.Fatalln(err)
    }
    csvFile = csv.NewWriter(f)
    defer csvFile.Flush()
    defer f.Close() // is this needed?


    // receive from websocket
    done := make(chan bool)
    go func() {
        defer close(done)
        for {
            _, message, err := c.ReadMessage()
            if err != nil {
                fmt.Println("error reading:", err)
                return
            }

            var raw struct {
                Module string `json:"module"`
                Data json.RawMessage `json:"data"`
            }
            if err := json.Unmarshal(message, &raw); err != nil {
                log.Fatalln("err", err)
            }

            var rawData map[string]interface{}
            err = json.Unmarshal(raw.Data, &rawData)
            if err != nil {
                log.Fatalln("err", err)
            }            

            fmt.Println("rawData", rawData)
            lineTime := GetTime()
            switch rawData["type"] {
            case "time":
                refIdx := int64(math.Round(rawData["index"].(float64)))
                referenceTimes[refIdx].receivedTime = lineTime
                referenceTimes[refIdx].robotTime = int64(math.Round(rawData["time"].(float64)))
                referenceTimes[refIdx].roundTrip_ms = referenceTimes[refIdx].receivedTime - referenceTimes[refIdx].sentTime
                fmt.Println("RefTime", strconv.Itoa(int(referenceTimes[refIdx].robotTime)), "rt time=", referenceTimes[refIdx].roundTrip_ms)
                csvMux.Lock()
                csvFile.Write([]string{"time", strconv.FormatInt(lineTime), strconv.Itoa(int(referenceTimes[refIdx].robotTime)), strconv.Itoa(int(referenceTimes[refIdx].sentTime)), strconv.Itoa(int(referenceTimes[refIdx].receivedTime)), strconv.Itoa(int(referenceTimes[refIdx].roundTrip_ms))})
                csvMux.Unlock()
            case "VAD":
                vadType := rawData["VAD"].(string)
                vadValue := int64(math.Round(rawData["value"].(float64)))
                vadTime := int64(math.Round(rawData["time"].(float64)))
                fmt.Printf("recv VAD: %s\n", rawData)
                csvMux.Lock()
                csvFile.Write([]string{"VAD", strconv.FormatInt(lineTime), strconv.Itoa(int(vadTime)), vadType, strconv.Itoa(int(vadValue)) })
                csvMux.Unlock()
            case "noise":
                power := rawData["power"].(float64)
                noise := rawData["noise"].(float64)
                noiseTime := int64(math.Round(rawData["time"].(float64)))
                csvMux.Lock()
                csvFile.Write([]string{"noise", strconv.FormatInt(lineTime), strconv.Itoa(int(noiseTime)), FloatToStr(power), FloatToStr(noise) })
                csvMux.Unlock()
            case "trigger":
                triggerTime := int64(math.Round(rawData["time"].(float64)))
                trigger := rawData["trigger"].(string)
                csvMux.Lock()
                csvFile.Write([]string{"trigger", strconv.FormatInt(lineTime), strconv.Itoa(int(triggerTime)), trigger})
                csvMux.Unlock()
            }
            
        }
    }()


    // create jobs

    silenceJob := Job { 
        sources: []Source{ Silence },
        patterns: [][]float64{
            {kTimeBetweenJobs},
        },
        gains: []float64{0.0},
        orders: []bool{true},
    }

    jobs := MakeJobs()

    
    // start playing audio
    go func() {
        for jobIdx, job := range jobs {
            
            descs := job.Description()
            csvMux.Lock()
            csvFile.Write(append([]string{"job", strconv.FormatInt(lineTime), strconv.Itoa(jobIdx)}, descs...)) 
            csvMux.Unlock()

            runJob(job)
            // pad a job with silence to hopefully reset VADs.
            // maybe we should instead instruct robot to reset VADs
            runJob(silenceJob)
        }
        done <- true
    }()


    // handle closing websocket and periodic time requests
    ticker := time.NewTicker(time.Second)
    defer ticker.Stop()
    for {
        select {
        case <-done:
            csvMux.Lock()
            csvFile.Flush()
            csvMux.Unlock()
            return
        case <-ticker.C:
            csvMux.Lock()
            csvFile.Flush()
            csvMux.Unlock()
            newTime := TimeInfo{ 
                sentTime: GetTime(),
            }
            referenceTimes = append(referenceTimes, newTime)
            index := len(referenceTimes) - 1
            timeRequest := `{"type": "data", "module": "vad", "data": {"type": "time", "index": ` + strconv.Itoa(index) + `} }`
            err := c.WriteMessage(websocket.TextMessage, []byte(timeRequest))
            fmt.Println("sent time request", timeRequest)
            if err != nil {
                fmt.Println("error writing time request:", err)
                return
            }
        case <-interrupt:
            csvMux.Lock()
            csvFile.Flush()
            csvMux.Unlock()
            err := c.WriteMessage( websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseNormalClosure, "") )
            if err != nil {
                fmt.Println("write close:", err)
                return
            }
            select {
            case <-done:
            case <-time.After(time.Second):
            }
            return
        }
    }

// SE angle?
// motor noise?
// speaker noise?

// sources = { noise, voice }
// pattern = {constant, constant}
// save pattern = {0, 1} // if constant and save pattern, analyze energy? if not constant, save pattern

// record noise floor and noise too
}