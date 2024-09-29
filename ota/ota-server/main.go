// delta_ota_creator.go
package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"crypto/sha256"
	"flag"
	"fmt"
	"io"
	"log"
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

func cleanupWorkingDirs() {
	tmpFiles := []string{
		"old",
		"new",
		"manifest.ini",
		"manifest.sha256",
		"delta.bin",
		"delta.bin.gz.plaintext",
		"delta.bin.gz",
	}
	for _, name := range tmpFiles {
		if err := safeRemove(name); err != nil {
			log.Printf("Error removing %s: %v", name, err)
		}
	}
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

func createSignature(filePathName, sigPathName, privateKey, privateKeyPass string) (bool, int, string, string) {
	cmd := exec.Command("/usr/bin/openssl",
		"dgst",
		"-sha256",
		"-sign",
		privateKey,
		"-passin",
		fmt.Sprintf("pass:%s", privateKeyPass),
		"-out",
		sigPathName,
		filePathName)
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

func createTarArchive(outputFile string, files []string) error {
	tarFile, err := os.Create(outputFile)
	if err != nil {
		return err
	}
	defer tarFile.Close()
	tarWriter := tar.NewWriter(tarFile)
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

func main() {
	oldPath := flag.String("old", "", "File with the old version to update from")
	newPath := flag.String("new", "", "File with the new version to update to")
	outPath := flag.String("out", "", "Path to hold the delta OTA")
	rebootAfterInstall := flag.String("reboot-after-install", "", "Override value of reboot_after_install")
	otaPassPath := flag.String("ota-pass", "", "Path to file with private ota password")
	maxDeltaSize := flag.Int64("max-delta-size", 0, "If the delta OTA file is larger than this, just use a full OTA")
	verbose := flag.Bool("verbose", false, "Verbose output")
	flag.Parse()

	if *oldPath == "" || *newPath == "" {
		flag.Usage()
		os.Exit(1)
	}

	cleanupWorkingDirs()

	scriptPath, err := os.Executable()
	if err != nil {
		log.Fatalf("Failed to get executable path: %v", err)
	}
	scriptDir := filepath.Dir(scriptPath)
	topLevel := filepath.Dir(filepath.Dir(scriptDir))

	privatePass := os.Getenv("OTAPASS")
	if privatePass == "" {
		privatePass = filepath.Join(topLevel, "ota", "ota_test.pass")
	}
	if *otaPassPath != "" {
		privatePass = *otaPassPath
	}

	oldManifestVer, _, err := extractFullOta(*oldPath, "old", privatePass)
	if err != nil {
		log.Fatalf("Failed to extract old OTA: %v", err)
	}
	newManifestVer, newManifest, err := extractFullOta(*newPath, "new", privatePass)
	if err != nil {
		log.Fatalf("Failed to extract new OTA: %v", err)
	}
	ankidev := newManifest.Section("META").Key("ankidev").String()
	rebootAfterInstallValue := *rebootAfterInstall
	if rebootAfterInstallValue == "" {
		rebootAfterInstallValue = newManifest.Section("META").Key("reboot_after_install").String()
	}

	defaultDeltaOtaName := fmt.Sprintf("vicos-%s_to_%s.ota", oldManifestVer, newManifestVer)
	deltaOtaName := *outPath
	if deltaOtaName != "" {
		if fi, err := os.Stat(deltaOtaName); err == nil && fi.IsDir() {
			deltaOtaName = filepath.Join(deltaOtaName, defaultDeltaOtaName)
		}
	} else {
		deltaOtaName = defaultDeltaOtaName
	}

	deltaEnv := os.Environ()
	deltaEnv = append(deltaEnv, fmt.Sprintf("LD_LIBRARY_PATH=%s", filepath.Join(scriptDir, "lib64")))
	deltaEnv = append(deltaEnv, fmt.Sprintf("PATH=%s:%s", filepath.Join(scriptDir, "bin"), os.Getenv("PATH")))

	oldSystemImg := filepath.Join("old", "system.img")
	oldBootImg := filepath.Join("old", "boot.img")
	newSystemImg := filepath.Join("new", "system.img")
	newBootImg := filepath.Join("new", "boot.img")
	oldPartitions := fmt.Sprintf("%s:%s", oldSystemImg, oldBootImg)
	newPartitions := fmt.Sprintf("%s:%s", newSystemImg, newBootImg)

	cmd := exec.Command("/bin/bash", "-c", "delta_generator "+
		fmt.Sprintf("--old_partitions=%s", oldPartitions)+" "+
		fmt.Sprintf("--new_partitions=%s", newPartitions)+" "+
		"--out_file=delta.bin")
	cmd.Env = deltaEnv
	fmt.Println(cmd.Env)
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	if err := cmd.Run(); err != nil {
		log.Fatalf("Delta generation failed\n%s\n%s", stdoutBuf.String(), stderrBuf.String())
	}

	if err := compressGzipFile("delta.bin", "delta.bin.gz.plaintext"); err != nil {
		log.Fatalf("Failed to compress delta.bin: %v", err)
	}

	success, retCode, stdout, stderr := encryptFile("delta.bin.gz.plaintext", "delta.bin.gz", privatePass)
	if !success {
		log.Fatalf("Failed to encrypt delta payload, openssl returned %d\n%s\n%s", retCode, stdout, stderr)
	}

	deltaManifest := ini.Empty()
	deltaManifest.Section("META").Key("manifest_version").SetValue("1.0.0")
	deltaManifest.Section("META").Key("update_version").SetValue(newManifestVer)
	deltaManifest.Section("META").Key("ankidev").SetValue(ankidev)
	deltaManifest.Section("META").Key("reboot_after_install").SetValue(rebootAfterInstallValue)
	deltaManifest.Section("META").Key("num_images").SetValue("1")

	deltaBinSize, err := fileSize("delta.bin")
	if err != nil {
		log.Fatalf("Failed to get size of delta.bin: %v", err)
	}
	deltaBinSha256, err := sha256File("delta.bin")
	if err != nil {
		log.Fatalf("Failed to compute SHA256 of delta.bin: %v", err)
	}
	newSystemImgSize, err := fileSize(filepath.Join("new", "system.img"))
	if err != nil {
		log.Fatalf("Failed to get size of system.img: %v", err)
	}
	newBootImgSize, err := fileSize(filepath.Join("new", "boot.img"))
	if err != nil {
		log.Fatalf("Failed to get size of boot.img: %v", err)
	}

	deltaManifest.Section("DELTA").Key("base_version").SetValue(oldManifestVer)
	deltaManifest.Section("DELTA").Key("compression").SetValue("gz")
	deltaManifest.Section("DELTA").Key("encryption").SetValue("1")
	deltaManifest.Section("DELTA").Key("wbits").SetValue("31")
	deltaManifest.Section("DELTA").Key("bytes").SetValue(fmt.Sprintf("%d", deltaBinSize))
	deltaManifest.Section("DELTA").Key("sha256").SetValue(deltaBinSha256)
	deltaManifest.Section("DELTA").Key("system_size").SetValue(fmt.Sprintf("%d", newSystemImgSize))
	deltaManifest.Section("DELTA").Key("boot_size").SetValue(fmt.Sprintf("%d", newBootImgSize))

	if err := deltaManifest.SaveTo("manifest.ini"); err != nil {
		log.Fatalf("Failed to save manifest.ini: %v", err)
	}

	privateKey := os.Getenv("OTAKEY")
	if privateKey == "" {
		privateKey = filepath.Join(topLevel, "ota", "ota_prod.key")
	}

	filesToArchive := []string{"manifest.ini", "delta.bin.gz"}
	if err := createTarArchive(deltaOtaName, filesToArchive); err != nil {
		log.Fatalf("Failed to create tar archive: %v", err)
	}

	deltaOtaSize, err := fileSize(deltaOtaName)
	if err != nil {
		log.Fatalf("Failed to get size of %s: %v", deltaOtaName, err)
	}
	if *maxDeltaSize > 0 && deltaOtaSize > *maxDeltaSize {
		if *verbose {
			fmt.Printf("Delta OTA file is too big (%d > %d). Will substitute a full OTA instead.\n", deltaOtaSize, *maxDeltaSize)
		}
		if err := os.Remove(deltaOtaName); err != nil {
			log.Fatalf("Failed to remove %s: %v", deltaOtaName, err)
		}
		if err := copyFile(*newPath, deltaOtaName); err != nil {
			log.Fatalf("Failed to copy %s to %s: %v", *newPath, deltaOtaName, err)
		}
	}

	cleanupWorkingDirs()
}
