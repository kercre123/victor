package main

import (
	"fmt"
	"io"
	"log"
	"mime/multipart"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
)

func main() {
	http.HandleFunc("/wakeword/upload", uploadHandler)

	log.Println("Starting server on :8012...")
	if err := http.ListenAndServe(":8012", nil); err != nil {
		log.Fatalf("Could not start server: %v", err)
	}
}

func uploadHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Invalid request method", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseMultipartForm(32 << 20); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		fmt.Println("error parsing")
		return
	}

	files := r.MultipartForm.File["wavfiles"]
	if len(files) < 3 || len(files) > 20 {
		http.Error(w, "You must upload between 3 and 20 .wav files", http.StatusBadRequest)
		fmt.Println("file amount")
		return
	}

	tmpDir := createTmpDir()
	defer os.RemoveAll(tmpDir)

	for _, fileHeader := range files {
		if err := saveFile(fileHeader, tmpDir); err != nil {
			http.Error(w, "Failed to save file: "+err.Error(), http.StatusInternalServerError)
			return
		}
	}

	cmd := exec.Command("./gen-wakeword.sh", tmpDir)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		http.Error(w, "Error generating hotword model: "+err.Error(), http.StatusInternalServerError)
		fmt.Println(err)
		return
	}

	hotwordFilePath := filepath.Join(tmpDir, "hotword.pmdl")
	hotwordFile, err := os.Open(hotwordFilePath)
	if err != nil {
		http.Error(w, "Could not open generated hotword file", http.StatusInternalServerError)
		fmt.Println(err)
		return
	}
	defer hotwordFile.Close()

	w.Header().Set("Content-Disposition", "attachment; filename=hotword.pmdl")
	w.Header().Set("Content-Type", "application/octet-stream")
	if _, err := io.Copy(w, hotwordFile); err != nil {
		http.Error(w, "Error sending hotword file", http.StatusInternalServerError)
		return
	}
}

func createTmpDir() string {
	tmpDir, _ := os.MkdirTemp(os.TempDir(), "wakeword_*")
	//if err := os.Mkdir(tmpDir, 0755); err != nil {
	//	log.Fatalf("Could not create temp dir: %v", err)
	//}
	return tmpDir
}

func saveFile(fileHeader *multipart.FileHeader, destDir string) error {
	file, err := fileHeader.Open()
	if err != nil {
		return fmt.Errorf("could not open file: %w", err)
	}
	defer file.Close()

	destinationPath := filepath.Join(destDir, fileHeader.Filename)
	destFile, err := os.Create(destinationPath)
	if err != nil {
		return fmt.Errorf("could not create destination file: %w", err)
	}
	defer destFile.Close()

	if _, err := io.Copy(destFile, file); err != nil {
		return fmt.Errorf("could not copy file: %w", err)
	}

	return nil
}
