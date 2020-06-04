# Chipper
Provides cloud service to classify audio/text to intents.


## Introduction
[More details here](https://docs.google.com/document/d/1H6H2xL3V8xTVVORd7PuENoZ9LdISboBTIXDwrhV6qMw/edit?usp=sharing)


## Run chipper-server on your local machine (Mac OS)

**Note: All credentials and API keys etc. are stored in the `CloudAIExploration` 1Password team vault**

1. Get  Dialogflow service accounts credentials from the team vault:
   - download `victor-dev-en-us-e54e8e28b8f5.json` and `victor-games-dev-en-us-ba82892ccc47.json` to the chipper repo dir
   - copy these json files to `/tmp`

2. Get file `chipper_local_env_var.list` from the team vault.
   - edit the file and add your AWS key and secret in the first two lines (needed for running chipper in docker)
   - export all the env vars in the file

3. Set up your AWS credentials for the main Anki account in `./aws/credentials`

4. Run `make restore-deps` to download all the dependencies

5. Run `make build` to create `sai-chipper-voice` in your `Go bin` dir (make sure this dir is in your $PATH)

6. Run chipper-server 
   - make sure you have permission to assume-role for Lex and the hypothesis-store Firehose stream, otherwise `export HSTORE_ENABLE=false` and `export LEX_ENABLE=false`
   - run `sai-chipper-voice --log-level=debug start`
   - you can do `sai-chipper-voice --help` and `sai-chipper-voice start --help` to see available options


## Run chipper-server in a docker container on Ubuntu

1. Follow steps 1 to 3 above

2. Build the docker container:
   - make sure you have access to Anki's Docker Hub
   - `docker pull anki/busybox` if you haven't done so
   - add these two lines to your local `Dockerfile`, **do not commit** this updated file to git.
     - `ADD victor-dev-en-us-e54e8e28b8f5.json /tmp/victor-dev-en-us-e54e8e28b8f5.json`
     - `ADD victor-games-dev-en-us-ba82892ccc47.json /tmp/victor-games-dev-en-us-ba82892ccc47.json`
   - run `mkdir -p bin` in the chipper repo directory
   - run `make docker`

3. **could be obselete** Run `docker run -v /etc/ssl/certs:/etc/ssl/certs --env-file [directory]/chipper_local_env_var.list -it -p 10000:10000 anki/sai-chipper-voice:[branch-name]`
   - **note**: `[directory]` is wherever you put `chipper_local_env_var.list`, and `[branch-name]` is the git branch name of your chipper repo.


## Run chipper-client

1. Run `make client`, `chipper-client` will be created in your `Go bin` directory.

3. Send requests to your local chipper-server in insecure mode:
   - text query: `chipper-client text --insecure=true --text="what time is it"`
   - audio query: `chipper-client stream-file --insecure=true --audio=./audio_files/trick.wav --noise=./audio_files/ambient.wav`
   - knowledge-graph query: `chipper-client stream-file --insecure=true --noise=./audio_files/ambient.wav --audio=./audio_files/moon.wav --mode=kg`
   - default intent-service is Default, which means the server will pick (set `--svc` to goog, or lex to use either directly)

3. Send request to cloud-chipper:
   - text query: `chipper-client text --addr=[chipper-dev-address] --port=443 --text="hello"`
   - audio query: `chipper-client stream-file --addr=[chipper-dev-address] --port=443 --noise=./audio_files/ambient.wav --audio=./audio_files/trick.wav`
   - knowledge-graph: add `--mode=kg` to the audio query
   - using live mic: `chipper-client stream-mic --addr=[chipper-dev-address] --port=443` (add `--mode=kg` to use knowledge-graph)

**Note**: audio files can be generated with Audacity on mac, with sampling rate = 16000, channels = 1, in linear-pcm

*Coming soon, using the live mic data on your machine to send requests*


## Why "chipper"?

Something to do with [this](https://en.wikipedia.org/wiki/Nipper).
