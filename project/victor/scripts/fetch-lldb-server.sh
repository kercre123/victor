#!/bin/bash

DOWNLOAD_DIR="${HOME}/.anki/android/downloads/"
INSTALL_ROOT="${HOME}/.anki/android/lldb-server"

EXECUTABLE_NAME="lldb-server"
VERSION_TAG="6.0.0-0da3ed6"
PLATFORM_TAG="android-arm"
DIGEST_TAG="sha256.txt"

ARCHIVE_NAME="${EXECUTABLE_NAME}-${VERSION_TAG}_${PLATFORM_TAG}.zip"
BASE_URL="http://sai-general.s3.amazonaws.com/build-assets"
ARCHIVE_URL="${BASE_URL}/${ARCHIVE_NAME}"
ARCHIVE_DIGEST_URL="${BASE_URL}/${ARCHIVE_NAME}.${DIGEST_TAG}"

ARCHIVE_DOWNLOAD_PATH="${DOWNLOAD_DIR}/${ARCHIVE_NAME}"
ARCHIVE_DIGEST_PATH="${DOWNLOAD_DIR}/${ARCHIVE_NAME}.${DIGEST_TAG}"

mkdir -p "${DOWNLOAD_DIR}"
mkdir -p "${INSTALL_ROOT}"

INSTALL_PATH="${INSTALL_ROOT}/${VERSION_TAG}/lldb-server"

# Already installed
if [ -f "${INSTALL_PATH}" ]; then
    exit 0
fi

# Download / Install
if [ ! -f "${ARCHIVE_DOWNLOAD_PATH}" ]; then
    curl -o "${ARCHIVE_DOWNLOAD_PATH}" "${ARCHIVE_URL}"
fi

if [ ! -f "${ARCHIVE_DIGEST_PATH}" ]; then
    curl -o "${ARCHIVE_DIGEST_PATH}" "${ARCHIVE_DIGEST_URL}"
fi

pushd "${DOWNLOAD_DIR}" > /dev/null 2>&1
shasum -a 256 -c "${ARCHIVE_NAME}.${DIGEST_TAG}"
SUCCESS=$?
popd > /dev/null 2>&1

if [ $SUCCESS -ne 0 ]; then
    echo "Failed to download or verify archive"
    exit 1
fi

pushd "${INSTALL_ROOT}" > /dev/null 2>&1
    mkdir -p "${VERSION_TAG}"
    pushd "${VERSION_TAG}" > /dev/null 2>&1
        unzip "${ARCHIVE_DOWNLOAD_PATH}"
    popd > /dev/null 2>&1
    rm -f current
    ln -s "${VERSION_TAG}" current
popd > /dev/null 2>&1
