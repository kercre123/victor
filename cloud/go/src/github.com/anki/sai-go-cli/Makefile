GOFILES=$(shell find . -name '*.go'  \! -name '*.rice-box.go')
GOBINDIR=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/bin
GOSRCDIR=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/src
BIN=sai-go-cli


ifndef GIT_BRANCH
	GIT_BRANCH=$(shell git symbolic-ref --short HEAD)
endif
ifndef GIT_COMMIT
	GIT_COMMIT=$(shell git rev-parse HEAD)
endif

ifeq ($(GIT_BRANCH),)
	GIT_BRANCH="unknown"
endif

HOSTNAME=$(shell hostname -f)
BUILD_USER=$(shell whoami)
PWD=$(shell pwd)

BUILDVAR=github.com/anki/sai-go-util/buildinfo

define BUILDFLAGS
-tags accounts_no_db \
-ldflags \
	"-X $(BUILDVAR).product=cli \
	-X $(BUILDVAR).buildTimeUnix=`date +%s` \
	-X $(BUILDVAR).commit=$(GIT_COMMIT) \
	-X $(BUILDVAR).branch=$(GIT_BRANCH) \
	-X $(BUILDVAR).buildHost=$(HOSTNAME)     \
	-X $(BUILDVAR).buildUser=$(BUILD_USER)"
endef

.PHONY: build buildall clean save-deps restore-deps

build: $(GOBINDIR)/$(BIN)

buildall: $(GOBINDIR)/$(BIN) $(GOBINDIR)/$(BIN)-mac-amd64 $(GOBINDIR)/$(BIN)-linux-amd64

$(GOBINDIR)/$(BIN): $(GOFILES)
	go install $(BUILDFLAGS) github.com/anki/sai-go-cli

$(GOBINDIR)/$(BIN)-mac-amd64: $(GOFILES)
	GOOS=darwin GOARCH=amd64 go build -o $@ $(BUILDFLAGS) github.com/anki/sai-go-cli

$(GOBINDIR)/$(BIN)-linux-amd64: $(GOFILES)
	GOOS=linux GOARCH=amd64 go build -o $@ $(BUILDFLAGS) github.com/anki/sai-go-cli


clean:
	rm -f $(GOBINDIR)/$(BIN)*

#
# Dependencies
#

$(GOBINDIR)/sai-godep:
	go get github.com/anki/sai-godep

save-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep save github.com/anki/$(BIN)/...

restore-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep restore -v
