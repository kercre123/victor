package main

import (
	"io/ioutil"
	"log"
	"net/http"
	dgen "ota-server/pkg/d-gen"
	"strings"
)

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
	err = ioutil.WriteFile("delta.ota", otaData, 0644)
	if err != nil {
		log.Fatalf("Failed to write OTA data to file: %v", err)
	}
}
