package riff

import (
	"io/ioutil"
	"testing"
)

func TestReadRIFF(t *testing.T) {
	testFiles := []testFile{
		testFile{
			"a.wav",
			3,
			243800,
			"WAVE"},
		testFile{
			"1_webp_a.webp",
			3,
			23396,
			"WEBP"}}

	for _, testFile := range testFiles {
		file, err := fixtureFile(testFile.Name)

		if err != nil {
			t.Fatalf("Failed to open fixture file")
		}

		reader := NewReader(file)
		riff, err := reader.Read()

		if err != nil {
			t.Fatal(err)
		}

		for _, chunk := range riff.Chunks {
			t.Logf("Chunk ID: %s", string(chunk.ChunkID[:]))
		}

		if len(riff.Chunks) != testFile.ChunkSize {
			t.Fatalf("Invalid length of chunks")
		}

		if riff.FileSize != testFile.FileSize {
			t.Fatalf("File size is invalid: %d", riff.FileSize)
		}

		if string(riff.FileType[:]) != testFile.FileType {
			t.Fatalf("File type is invalid: %s", riff.FileType)
		}

		data, err := ioutil.ReadAll(riff.Chunks[0])

		if err != nil {
			t.Fatalf("Can't read data from chunk")
		}

		t.Logf("Length of the first chunk: %d", len(data))

		file.Close()
	}
}

func TestReadNonRIFF(t *testing.T) {
	file, err := fixtureFile("../reader.go")

	if err != nil {
		t.Fatalf("Failed to open fixture file")
	}

	reader := NewReader(file)
	_, err = reader.Read()

	if err.Error() != "Given bytes is not a RIFF format" {
		t.Fatal("Non-RIFF file should not be read")
	}
}
