CLEAN_FILES+=\
	$(GOBINDIR)/kinloader \
	$(GOBINDIR)/log2kinesis \
	$(GOBINDIR)/dumpstream \
	$(GOBINDIR)/logdecode

PACKAGE=sai-kinlog

#
# Standard library makefile shim.
#

-include $(GOPATH)/src/github.com/anki/sai-go-build/makefiles/library.mk
$(GOPATH)/src/github.com/anki/sai-go-build/makefiles/library.mk:
	GO111MODULE=off go get github.com/anki/sai-go-build

all: build

build: $(GOBINDIR)/kinloader $(GOBINDIR)/log2kinesis $(GOBINDIR)/dumpstream $(GOBINDIR)/logdecode


splunkinput: $(GOBINDIR)/kinloader
	cp -a $(GOBINDIR)/kinloader splunkinput/app/kinloader/bin
	tar -C splunkinput/app -cvzf kinloader.tar.gz kinloader

dockerimage: splunkinput
	cp kinloader.tar.gz splunkinput/docker/
	docker build -t $(DOCKER_TAG) splunkinput/docker


$(GOBINDIR)/kinloader: $(GOFILES)
	go install -ldflags \
		"-X github.com/anki/sai-go-util/buildinfo.product=kinloader  \
		-X github.com/anki/sai-go-util/buildinfo.buildTimeUnix=`date +%s` \
		-X github.com/anki/sai-go-util/buildinfo.commit=$(GIT_COMMIT) \
		-X github.com/anki/sai-go-util/buildinfo.branch=$(GIT_BRANCH) \
		-X github.com/anki/sai-go-util/buildinfo.buildHost=$(HOSTNAME) \
		-X github.com/anki/sai-go-util/buildinfo.buildUser=$(BUILD_USER)" \
		github.com/anki/sai-kinlog/splunkinput/cmd/kinloader

$(GOBINDIR)/log2kinesis: $(GOFILES)
	go install -ldflags \
		"-X github.com/anki/sai-go-util/buildinfo.product=log2kinesis  \
		-X github.com/anki/sai-go-util/buildinfo.buildTimeUnix=`date +%s` \
		-X github.com/anki/sai-go-util/buildinfo.commit=$(GIT_COMMIT) \
		-X github.com/anki/sai-go-util/buildinfo.branch=$(GIT_BRANCH) \
		-X github.com/anki/sai-go-util/buildinfo.buildHost=$(HOSTNAME) \
		-X github.com/anki/sai-go-util/buildinfo.buildUser=$(BUILD_USER)" \
		github.com/anki/sai-kinlog/logsender/cmd/log2kinesis

$(GOBINDIR)/dumpstream: $(GOFILES)
	go install -ldflags \
		"-X github.com/anki/sai-go-util/buildinfo.product=dumpstream  \
		-X github.com/anki/sai-go-util/buildinfo.buildTimeUnix=`date +%s` \
		-X github.com/anki/sai-go-util/buildinfo.commit=$(GIT_COMMIT) \
		-X github.com/anki/sai-go-util/buildinfo.branch=$(GIT_BRANCH) \
		-X github.com/anki/sai-go-util/buildinfo.buildHost=$(HOSTNAME) \
		-X github.com/anki/sai-go-util/buildinfo.buildUser=$(BUILD_USER)" \
		github.com/anki/sai-kinlog/kinreader/cmd/dumpstream

$(GOBINDIR)/logdecode: $(GOFILES)
	go install -ldflags \
		"-X github.com/anki/sai-go-util/buildinfo.product=logdecode  \
		-X github.com/anki/sai-go-util/buildinfo.buildTimeUnix=`date +%s` \
		-X github.com/anki/sai-go-util/buildinfo.commit=$(GIT_COMMIT) \
		-X github.com/anki/sai-go-util/buildinfo.branch=$(GIT_BRANCH) \
		-X github.com/anki/sai-go-util/buildinfo.buildHost=$(HOSTNAME) \
		-X github.com/anki/sai-go-util/buildinfo.buildUser=$(BUILD_USER)" \
		github.com/anki/sai-kinlog/kinreader/cmd/logdecode
