
# Run before pushing code

1. `make integrationtest` creates `integrationtest` binary in `Go bin`
2. start local chipper-server
2. from sai-chipper-voice repo dir: `integrationtest --cert-file=./127.0.0.1.crt -svc=goog --noise=./audio_files/ambient.wav --audio=./audio_files/trick.wav`
