// delta_ota_creator.go
package dgen

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"crypto/sha256"
	"fmt"
	"io"

	"os"
	"os/exec"
	"path/filepath"

	"gopkg.in/ini.v1"
)

func safeRemove(name string) error {
	if _, err := os.Stat(name); os.IsNotExist(err) {
		return nil
	}
	return os.RemoveAll(name)
}

func sha256File(name string) (string, error) {
	file, err := os.Open(name)
	if err != nil {
		return "", err
	}
	defer file.Close()
	hash := sha256.New()
	if _, err := io.Copy(hash, file); err != nil {
		return "", err
	}
	return fmt.Sprintf("%x", hash.Sum(nil)), nil
}

func decryptFile(ciphertextPath, plaintextPath, keyPath string) (bool, int, string, string) {
	cmd := exec.Command("/usr/bin/openssl",
		"enc",
		"-d",
		"-aes-256-ctr",
		"-pass",
		fmt.Sprintf("file:%s", keyPath),
		"-md",
		"md5",
		"-in",
		ciphertextPath,
		"-out",
		plaintextPath)
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	err := cmd.Run()
	exitCode := 0
	if exitErr, ok := err.(*exec.ExitError); ok {
		exitCode = exitErr.ExitCode()
	}
	success := err == nil
	return success, exitCode, stdoutBuf.String(), stderrBuf.String()
}

func encryptFile(plaintextPath, ciphertextPath, keyPath string) (bool, int, string, string) {
	cmd := exec.Command("/usr/bin/openssl",
		"aes-256-ctr",
		"-pass",
		fmt.Sprintf("file:%s", keyPath),
		"-md",
		"md5",
		"-in",
		plaintextPath,
		"-out",
		ciphertextPath)
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	err := cmd.Run()
	exitCode := 0
	if exitErr, ok := err.(*exec.ExitError); ok {
		exitCode = exitErr.ExitCode()
	}
	success := err == nil
	return success, exitCode, stdoutBuf.String(), stderrBuf.String()
}

func decompressGzipFile(src, dst string) error {
	inFile, err := os.Open(src)
	if err != nil {
		return err
	}
	defer inFile.Close()
	gzipReader, err := gzip.NewReader(inFile)
	if err != nil {
		return err
	}
	defer gzipReader.Close()
	outFile, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer outFile.Close()
	_, err = io.Copy(outFile, gzipReader)
	return err
}

func compressGzipFile(src, dst string) error {
	inFile, err := os.Open(src)
	if err != nil {
		return err
	}
	defer inFile.Close()
	outFile, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer outFile.Close()
	gzipWriter := gzip.NewWriter(outFile)
	defer gzipWriter.Close()
	_, err = io.Copy(gzipWriter, inFile)
	return err
}

func extractFullOta(name, tmpdir, privatePass string) (string, *ini.File, error) {
	if err := safeRemove(tmpdir); err != nil {
		return "", nil, fmt.Errorf("Failed to remove %s: %v", tmpdir, err)
	}
	if err := os.Mkdir(tmpdir, 0755); err != nil {
		return "", nil, fmt.Errorf("Failed to create dir %s: %v", tmpdir, err)
	}
	// Open the tar file
	tarFile, err := os.Open(name)
	if err != nil {
		return "", nil, fmt.Errorf("Couldn't open %s as tar file: %v", name, err)
	}
	defer tarFile.Close()
	tarReader := tar.NewReader(tarFile)
	// Extract all files
	for {
		header, err := tarReader.Next()
		if err == io.EOF {
			break
		}
		if err != nil {
			return "", nil, fmt.Errorf("Error reading tar file %s: %v", name, err)
		}
		target := filepath.Join(tmpdir, header.Name)
		switch header.Typeflag {
		case tar.TypeDir:
			if err := os.MkdirAll(target, os.FileMode(header.Mode)); err != nil {
				return "", nil, fmt.Errorf("Error creating directory %s: %v", target, err)
			}
		case tar.TypeReg:
			outFile, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY, os.FileMode(header.Mode))
			if err != nil {
				return "", nil, fmt.Errorf("Error creating file %s: %v", target, err)
			}
			if _, err := io.Copy(outFile, tarReader); err != nil {
				outFile.Close()
				return "", nil, fmt.Errorf("Error writing to file %s: %v", target, err)
			}
			outFile.Close()
		default:
		}
	}
	manifestPath := filepath.Join(tmpdir, "manifest.ini")
	manifest, err := ini.Load(manifestPath)
	if err != nil {
		return "", nil, fmt.Errorf("Failed to load manifest.ini: %v", err)
	}
	encryption, err := manifest.Section("BOOT").Key("encryption").Int()
	if err != nil {
		return "", nil, fmt.Errorf("Failed to get BOOT encryption value: %v", err)
	}
	bootImgGz := filepath.Join(tmpdir, "apq8009-robot-boot.img.gz")
	if encryption == 1 {
		ciphertext := bootImgGz
		bootImgGz = filepath.Join(tmpdir, "boot.img.gz")
		success, retCode, stdout, stderr := decryptFile(ciphertext, bootImgGz, privatePass)
		if !success {
			return "", nil, fmt.Errorf("Failed to decrypt boot image, openssl returned %d\n%s\n%s", retCode, stdout, stderr)
		}
	}
	bootImg := filepath.Join(tmpdir, "boot.img")
	if err := decompressGzipFile(bootImgGz, bootImg); err != nil {
		return "", nil, fmt.Errorf("Failed to decompress boot image: %v", err)
	}
	encryption, err = manifest.Section("SYSTEM").Key("encryption").Int()
	if err != nil {
		return "", nil, fmt.Errorf("Failed to get SYSTEM encryption value: %v", err)
	}
	sysImgGz := filepath.Join(tmpdir, "apq8009-robot-sysfs.img.gz")
	if encryption == 1 {
		ciphertext := sysImgGz
		sysImgGz = filepath.Join(tmpdir, "system.img.gz")
		success, retCode, stdout, stderr := decryptFile(ciphertext, sysImgGz, privatePass)
		if !success {
			return "", nil, fmt.Errorf("Failed to decrypt system image, openssl returned %d\n%s\n%s", retCode, stdout, stderr)
		}
	}
	sysImg := filepath.Join(tmpdir, "system.img")
	if err := decompressGzipFile(sysImgGz, sysImg); err != nil {
		return "", nil, fmt.Errorf("Failed to decompress system image: %v", err)
	}
	updateVersion := manifest.Section("META").Key("update_version").String()
	return updateVersion, manifest, nil
}

func createTarArchive(w io.Writer, files []string) error {
	tarWriter := tar.NewWriter(w)
	defer tarWriter.Close()
	for _, fileName := range files {
		fileInfo, err := os.Stat(fileName)
		if err != nil {
			return err
		}
		header, err := tar.FileInfoHeader(fileInfo, "")
		if err != nil {
			return err
		}
		header.Name = filepath.Base(fileName)
		header.Uid = 0
		header.Gid = 0
		header.Uname = "root"
		header.Gname = "root"
		header.Mode = 0400
		if err := tarWriter.WriteHeader(header); err != nil {
			return err
		}
		file, err := os.Open(fileName)
		if err != nil {
			return err
		}
		if _, err := io.Copy(tarWriter, file); err != nil {
			file.Close()
			return err
		}
		file.Close()
	}
	return nil
}

func fileSize(name string) (int64, error) {
	fi, err := os.Stat(name)
	if err != nil {
		return 0, err
	}
	return fi.Size(), nil
}

func copyFile(src, dst string) error {
	inFile, err := os.Open(src)
	if err != nil {
		return err
	}
	defer inFile.Close()
	outFile, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer outFile.Close()
	if _, err := io.Copy(outFile, inFile); err != nil {
		return err
	}
	return nil
}

type DeltaOTAOptions struct {
	OldPath            string
	NewPath            string
	RebootAfterInstall string
	OTAPassPath        string
	MaxDeltaSize       int64
	Verbose            bool
}

func CreateDeltaOTA(options DeltaOTAOptions) ([]byte, error) {
	if options.OldPath == "" || options.NewPath == "" {
		return nil, fmt.Errorf("OldPath and NewPath must be provided")
	}

	// Create temporary directories
	tempDirOld, err := os.MkdirTemp("", "deltaota_old_*")
	if err != nil {
		return nil, fmt.Errorf("Failed to create temporary directory for old OTA: %v", err)
	}
	defer os.RemoveAll(tempDirOld)

	tempDirNew, err := os.MkdirTemp("", "deltaota_new_*")
	if err != nil {
		return nil, fmt.Errorf("Failed to create temporary directory for new OTA: %v", err)
	}
	defer os.RemoveAll(tempDirNew)

	tempDirDelta, err := os.MkdirTemp("", "deltaota_delta_*")
	if err != nil {
		return nil, fmt.Errorf("Failed to create temporary directory for delta OTA: %v", err)
	}
	defer os.RemoveAll(tempDirDelta)

	privatePass := options.OTAPassPath
	if privatePass == "" {
		privatePass = os.Getenv("OTAPASS")
		if privatePass == "" {
			return nil, fmt.Errorf("OTAPassPath is required")
		}
	}

	oldManifestVer, _, err := extractFullOta(options.OldPath, tempDirOld, privatePass)
	if err != nil {
		return nil, fmt.Errorf("Failed to extract old OTA: %v", err)
	}
	newManifestVer, newManifest, err := extractFullOta(options.NewPath, tempDirNew, privatePass)
	if err != nil {
		return nil, fmt.Errorf("Failed to extract new OTA: %v", err)
	}
	ankidev := newManifest.Section("META").Key("ankidev").String()
	rebootAfterInstallValue := options.RebootAfterInstall
	if rebootAfterInstallValue == "" {
		rebootAfterInstallValue = newManifest.Section("META").Key("reboot_after_install").String()
	}

	oldSystemImg := filepath.Join(tempDirOld, "system.img")
	oldBootImg := filepath.Join(tempDirOld, "boot.img")
	newSystemImg := filepath.Join(tempDirNew, "system.img")
	newBootImg := filepath.Join(tempDirNew, "boot.img")
	oldPartitions := fmt.Sprintf("%s:%s", oldSystemImg, oldBootImg)
	newPartitions := fmt.Sprintf("%s:%s", newSystemImg, newBootImg)

	deltaBinPath := filepath.Join(tempDirDelta, "delta.bin")
	deltaBinGzPlaintextPath := filepath.Join(tempDirDelta, "delta.bin.gz.plaintext")
	deltaBinGzPath := filepath.Join(tempDirDelta, "delta.bin.gz")
	manifestIniPath := filepath.Join(tempDirDelta, "manifest.ini")

	deltaEnv := os.Environ()
	deltaEnv = append(deltaEnv, fmt.Sprintf("LD_LIBRARY_PATH=%s", "/usr/local/lib64"))
	deltaEnv = append(deltaEnv, fmt.Sprintf("PATH=%s:%s", "/usr/local/bin", os.Getenv("PATH")))

	cmd := exec.Command("/bin/bash", "-c", fmt.Sprintf("delta_generator --old_partitions=%s --new_partitions=%s --out_file=%s", oldPartitions, newPartitions, deltaBinPath))
	cmd.Env = deltaEnv
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	if err := cmd.Run(); err != nil {
		return nil, fmt.Errorf("Delta generation failed\n%s\n%s", stdoutBuf.String(), stderrBuf.String())
	}

	if err := compressGzipFile(deltaBinPath, deltaBinGzPlaintextPath); err != nil {
		return nil, fmt.Errorf("Failed to compress delta.bin: %v", err)
	}

	success, retCode, stdout, stderr := encryptFile(deltaBinGzPlaintextPath, deltaBinGzPath, privatePass)
	if !success {
		return nil, fmt.Errorf("Failed to encrypt delta payload, openssl returned %d\n%s\n%s", retCode, stdout, stderr)
	}

	deltaManifest := ini.Empty()
	deltaManifest.Section("META").Key("manifest_version").SetValue("1.0.0")
	deltaManifest.Section("META").Key("update_version").SetValue(newManifestVer)
	deltaManifest.Section("META").Key("ankidev").SetValue(ankidev)
	deltaManifest.Section("META").Key("reboot_after_install").SetValue(rebootAfterInstallValue)
	deltaManifest.Section("META").Key("num_images").SetValue("1")

	deltaBinSize, err := fileSize(deltaBinPath)
	if err != nil {
		return nil, fmt.Errorf("Failed to get size of delta.bin: %v", err)
	}
	deltaBinSha256, err := sha256File(deltaBinPath)
	if err != nil {
		return nil, fmt.Errorf("Failed to compute SHA256 of delta.bin: %v", err)
	}
	newSystemImgSize, err := fileSize(newSystemImg)
	if err != nil {
		return nil, fmt.Errorf("Failed to get size of system.img: %v", err)
	}
	newBootImgSize, err := fileSize(newBootImg)
	if err != nil {
		return nil, fmt.Errorf("Failed to get size of boot.img: %v", err)
	}

	deltaManifest.Section("DELTA").Key("base_version").SetValue(oldManifestVer)
	deltaManifest.Section("DELTA").Key("compression").SetValue("gz")
	deltaManifest.Section("DELTA").Key("encryption").SetValue("1")
	deltaManifest.Section("DELTA").Key("wbits").SetValue("31")
	deltaManifest.Section("DELTA").Key("bytes").SetValue(fmt.Sprintf("%d", deltaBinSize))
	deltaManifest.Section("DELTA").Key("sha256").SetValue(deltaBinSha256)
	deltaManifest.Section("DELTA").Key("system_size").SetValue(fmt.Sprintf("%d", newSystemImgSize))
	deltaManifest.Section("DELTA").Key("boot_size").SetValue(fmt.Sprintf("%d", newBootImgSize))

	if err := deltaManifest.SaveTo(manifestIniPath); err != nil {
		return nil, fmt.Errorf("Failed to save manifest.ini: %v", err)
	}

	var deltaOtaBuffer bytes.Buffer
	filesToArchive := []string{manifestIniPath, deltaBinGzPath}
	if err := createTarArchive(&deltaOtaBuffer, filesToArchive); err != nil {
		return nil, fmt.Errorf("Failed to create tar archive: %v", err)
	}

	deltaOtaSize := int64(deltaOtaBuffer.Len())
	if options.MaxDeltaSize > 0 && deltaOtaSize > options.MaxDeltaSize {
		if options.Verbose {
			fmt.Printf("Delta OTA file is too big (%d > %d). Will substitute a full OTA instead.\n", deltaOtaSize, options.MaxDeltaSize)
		}
		fullOtaData, err := os.ReadFile(options.NewPath)
		if err != nil {
			return nil, fmt.Errorf("Failed to read full OTA file %s: %v", options.NewPath, err)
		}
		return fullOtaData, nil
	}

	return deltaOtaBuffer.Bytes(), nil
}
