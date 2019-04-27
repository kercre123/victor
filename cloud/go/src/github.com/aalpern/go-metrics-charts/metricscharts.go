package metricscharts

import (
	"log"
	"net/http"

	"github.com/aalpern/go-metrics-charts/bindata"
)

func Register() {
	http.HandleFunc("/debug/metrics/charts/", handleAsset("static/index.html"))
	http.HandleFunc("/debug/metrics/charts/main.js", handleAsset("static/main.js"))
}

func handleAsset(path string) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		data, err := bindata.Asset(path)
		if err != nil {
			log.Print(err)
			return
		}

		n, err := w.Write(data)
		if err != nil {
			log.Print(err)
			return
		}

		if n != len(data) {
			log.Print("wrote less than supposed to")
			return
		}
	}
}
