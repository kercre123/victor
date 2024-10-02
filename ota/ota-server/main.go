package main

import (
	"errors"
	"fmt"
	"net/http"
	"os"
	dgen "ota-server/pkg/d-gen"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"unicode"
)

var (
	LatestVersionFile      = "/wire/otas/latest"
	DoNotAcceptRequestFile = "/wire/otas/dnar"
	DiffPath               = "/wire/otas/diff"
	FullPath               = "/wire/otas/full"
	PassPath               = "/wire/ota.pas"
)

var (
	inProgressOTAs      = make(map[string]chan struct{})
	inProgressOTAsMutex sync.Mutex
)

func ShouldNotDoWork() bool {
	return FileExists(DoNotAcceptRequestFile)
}

func FileExists(file string) bool {
	_, err := os.Open(file)
	return err == nil
}

func GenDiffFilePath(old string, new string, target string) string {
	return filepath.Join(DiffPath, target, old+"-"+new+".ota")
}

func GetFullOTAPath(ota string, target string) string {
	return filepath.Join(FullPath, target, ota+".ota")
}

// remove letters
func NormVer(version string) string {
	var result []rune
	for _, char := range version {
		if !unicode.IsLetter(char) {
			result = append(result, char)
		}
	}
	return strings.TrimSpace(string(result))
}

func GetLatestVersion() string {
	out, err := os.ReadFile(LatestVersionFile)
	if err != nil {
		return ""
	}
	return strings.TrimSpace(string(out))
}

func FileOpen(path string) *[]byte {
	out, _ := os.ReadFile(path)
	return &out
}

func GetOTA(version string, target string, diff bool) (*[]byte, error) {
	if ShouldNotDoWork() {
		return nil, errors.New("do not accept request file in place")
	}
	version = NormVer(version)
	latestVer := GetLatestVersion()
	if version == latestVer {
		return nil, errors.New("already on the latest version")
	}
	diffPath := GenDiffFilePath(version, latestVer, target)
	fullPath := GetFullOTAPath(version, target)
	latestFullPath := GetFullOTAPath(latestVer, target)
	if diff {
		if FileExists(diffPath) {
			return FileOpen(diffPath), nil
		} else if FileExists(fullPath) {
			inProgressOTAsMutex.Lock()
			if ch, exists := inProgressOTAs[diffPath]; exists {
				// another generation in progress; wait for it to complete
				inProgressOTAsMutex.Unlock()
				<-ch
				if FileExists(diffPath) {
					return FileOpen(diffPath), nil
				} else {
					return nil, errors.New("ota generation failed after waiting")
				}
			} else {
				ch := make(chan struct{})
				inProgressOTAs[diffPath] = ch
				inProgressOTAsMutex.Unlock()
				options := dgen.DeltaOTAOptions{
					OldPath:            fullPath,
					NewPath:            latestFullPath,
					RebootAfterInstall: "1",
					OTAPassPath:        PassPath,
					MaxDeltaSize:       80000000, // 80 MB
					Verbose:            true,
				}
				otaData, err := dgen.CreateDeltaOTA(options)
				inProgressOTAsMutex.Lock()
				delete(inProgressOTAs, diffPath)
				close(ch)
				inProgressOTAsMutex.Unlock()

				if err == nil {
					os.MkdirAll(filepath.Dir(diffPath), 0777)
					os.WriteFile(diffPath, otaData, 0777)
					return &otaData, nil
				} else {
					fmt.Println("error creating delta OTA: " + err.Error())
					return nil, err
				}
			}
		}
	} else {
		if FileExists(latestFullPath) {
			return FileOpen(latestFullPath), nil
		}
	}
	return nil, errors.New("ota doesn't exist")
}

var TargetMap []string = []string{"dev", "oskr", "whiskey", "orange", "dvt3"}

func targetToString(target string) string {
	targ, err := strconv.Atoi(target)
	if err != nil {
		return ""
	} else {
		return TargetMap[targ]
	}
}

func otaHTTPHandler(w http.ResponseWriter, r *http.Request) {
	if ShouldNotDoWork() {
		// we want the build server to be able to initiate a diff update
		if r.FormValue("isBuildServer") != "true" {
			http.Error(w, "a release is currently being uploaded, please wait", 500)
			return
		}
	}
	fmt.Println(r.URL.Path)
	if strings.HasPrefix(r.URL.Path, "/vic/full") {
		version := NormVer(r.FormValue("victorversion"))
		target := r.FormValue("victortarget")
		if target == "" {
			fmt.Println("no target :(")
			http.Error(w, "no target", 404)
			return
		}
		target = targetToString(target)
		out, err := GetOTA(version, target, false)
		if err != nil {
			http.Error(w, err.Error(), 404)
			return
		}
		w.Header().Set("Content-Length", fmt.Sprintf("%d", len(*out)))
		w.Write(*out)
	} else if strings.HasPrefix(r.URL.Path, "/vic/diff") {
		version := NormVer(r.FormValue("victorversion"))
		target := r.FormValue("victortarget")
		if target == "" {
			fmt.Println("no target :(")
			http.Error(w, "no target", 404)
			return
		}
		target = targetToString(target)
		out, err := GetOTA(version, target, true)
		if err != nil {
			fmt.Println(err)
			http.Error(w, err.Error(), 404)
			return
		}
		w.Header().Set("Content-Length", fmt.Sprintf("%d", len(*out)))
		w.Write(*out)
	} else if strings.HasPrefix(r.URL.Path, "/vic/raw") {
		splitURL := strings.Split(r.URL.Path, "/")
		if len(splitURL) < 5 {
			http.Error(w, "not found", 404)
			return
		}
		target := splitURL[3]
		ver := splitURL[4]
		var filePath string
		if ver == "latest.ota" {
			filePath = GetFullOTAPath(GetLatestVersion(), target)
		} else {
			filePath = GetFullOTAPath(strings.Replace(ver, ".ota", "", -1), target)
		}
		if FileExists(filePath) {
			data := FileOpen(filePath)
			w.Header().Set("Content-Length", fmt.Sprintf("%d", len(*data)))
			w.Write(*data)
		} else {
			http.Error(w, "not found", 404)
			return
		}
	} else if strings.HasPrefix(r.URL.Path, "/base") {
		data := *FileOpen("/wire/otas/base/2.0.1.0.ota")
		w.Header().Set("Content-Length", fmt.Sprintf("%d", len(data)))
		w.Write(data)
	}
}

func main() {
	http.HandleFunc("/vic/", otaHTTPHandler)
	http.HandleFunc("/base/", otaHTTPHandler)
	http.Handle("/", http.FileServer(http.Dir(FullPath+"/")))
	fmt.Println("listening on 5901")
	http.ListenAndServe(":5901", nil)
}
