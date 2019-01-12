// +build !shipping

package box

import (
	"anki/ipc"
	"anki/log"
	"bytes"
	"clad/vision"
	"encoding/json"
	"fmt"
	"html/template"
	"io/ioutil"
	"net/http"
	"os"
	"path"
)

const baseDir = "/anki/data/assets/cozmo_resources/webserver/cloud/box"
const imageDir = baseDir + "/images"
const cacheDir = "/data/data/com.anki.victor/cache/boxtest"

func init() {
	devHandlers = func(s *http.ServeMux) [][]string {
		s.HandleFunc("/box/", boxHandler)
		s.HandleFunc("/box/request", reqHandler)

		imgPrefix := "/box/images/"
		s.Handle(imgPrefix, http.StripPrefix(imgPrefix, http.HandlerFunc(imgHandler)))

		log.Println("Box dev handlers added")
		return [][]string{[]string{"/box", "Send test images to Snapper and see the results from MS"}}
	}
}

func boxHandler(w http.ResponseWriter, r *http.Request) {
	files, err := ioutil.ReadDir(imageDir)

	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error listing images: ", err)
		return
	}

	// cache dir is optional, skip it if it causes an error
	if cacheFiles, err := ioutil.ReadDir(cacheDir); err == nil {
		files = append(files, cacheFiles...)
	}

	t, err := template.ParseFiles(baseDir + "/index.html")
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error parsing template: ", err)
		return
	}

	if err := t.Execute(w, files); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error executing template: ", err)
		return
	}
}

var boxClient ipc.Conn

func reqHandler(w http.ResponseWriter, r *http.Request) {
	// initialize connection to ipc server
	if boxClient == nil {
		var err error
		if boxClient, err = ipc.NewUnixgramClient(ipc.GetSocketPath("box_server"), "box_dev_client"); err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			fmt.Fprint(w, "Error connecting to server: ", err)
			return
		}
	}

	reqFile := r.URL.Query().Get("file")
	if reqFile == "" {
		w.WriteHeader(http.StatusBadRequest)
		fmt.Fprint(w, "Request must include file=[filename]")
		return
	}

	// make sure file exists - first try image path, then cache dir
	file := path.Join(imageDir, reqFile)
	if _, err := os.Stat(file); os.IsNotExist(err) {
		file = path.Join(cacheDir, reqFile)
		if _, err := os.Stat(file); os.IsNotExist(err) {
			w.WriteHeader(http.StatusBadRequest)
			fmt.Fprint(w, "File does not exist: ", reqFile)
			return
		}
	}

	var msg vision.OffboardImageReady
	msg.Filename = file

	var buf bytes.Buffer
	if err := msg.Pack(&buf); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error packing message: ", err)
		return
	}

	if _, err := boxClient.Write(buf.Bytes()); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error sending message: ", err)
		return
	}

	respBuf := boxClient.ReadBlock()
	var resp vision.OffboardResultReady
	if err := resp.Unpack(bytes.NewBuffer(respBuf)); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error unpacking response: ", err)
		return
	}

	var prettyJSON bytes.Buffer
	json.Indent(&prettyJSON, []byte(resp.JsonResult), "", "\t")

	fmt.Fprint(w, prettyJSON.String())
}

func imgHandler(w http.ResponseWriter, r *http.Request) {
	file := path.Join(imageDir, r.URL.Path)
	var dir string
	if _, err := os.Stat(file); !os.IsNotExist(err) {
		dir = imageDir
	} else {
		dir = cacheDir
	}
	http.FileServer(http.Dir(dir)).ServeHTTP(w, r)
}
