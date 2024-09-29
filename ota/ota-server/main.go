package main

import (
	"errors"
	"log"
	"net/http"
	"os"
	dgen "ota-server/pkg/d-gen"
	"path/filepath"
	"strings"
	"unicode"
)

var (
	LatestVersionFile      = "/wire/otas/latest"
	DoNotAcceptRequestFile = "/wire/otas/dnar"
	DiffPath               = "/wire/otas/diff"
	FullPath               = "/wire/otas/full"
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
	return string(out)
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
	if FileExists(diffPath) {
		return FileOpen(diffPath), nil
	} else if FileExists(fullPath) {
		options := dgen.DeltaOTAOptions{
			OldPath:            fullPath,
			NewPath:            "/path/to/new.ota",
			RebootAfterInstall: "1",
			OTAPassPath:        "/path/to/ota.pass",
			MaxDeltaSize:       80000000, // 80 MB
			Verbose:            true,
		}
		otaData, err := dgen.CreateDeltaOTA(options)
		if err != nil {
			log.Fatalf("Failed to create delta OTA: %v", err)
		}
		return FileOpen(fullPath), nil
	}

}

func otaHTTPHandler(r *http.Request, w http.ResponseWriter) {
	if strings.HasPrefix(r.URL.Path, "/full") {

	}
}

func main() {
	options := dgen.DeltaOTAOptions{
		OldPath:            "/path/to/old.ota",
		NewPath:            "/path/to/new.ota",
		RebootAfterInstall: "1",
		OTAPassPath:        "/path/to/ota.pass",
		MaxDeltaSize:       80000000, // 80 MB
		Verbose:            true,
	}

	otaData, err := dgen.CreateDeltaOTA(options)
	if err != nil {
		log.Fatalf("Failed to create delta OTA: %v", err)
	}

	// otaData now contains the OTA bytes, which you can write to a file or process further
	err = os.WriteFile("delta.ota", otaData, 0644)
	if err != nil {
		log.Fatalf("Failed to write OTA data to file: %v", err)
	}
}
