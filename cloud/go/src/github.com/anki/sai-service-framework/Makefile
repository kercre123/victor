#
#
# Service Name (change this per project). This is the main go package,
# and should match the repository name.
#
#

SERVICE=sai-service-framework

#
#
# Standard Boilerplate (don't change this)
#
#

GOVERSION:=$(shell go version | awk '{print $3}')
GOFILES:=$(shell find . -name '*.go'  \! -name '*.rice-box.go')
GOBINDIR:=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/bin
GOSRCDIR:=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/src
PATH:=$(GOBINDIR):${PATH}

ifndef GIT_BRANCH
GIT_BRANCH:=$(shell git symbolic-ref --short HEAD)
endif
ifndef GIT_COMMIT
GIT_COMMIT:=$(shell git rev-parse HEAD)
endif
ifeq ($(GIT_BRANCH),)
GIT_BRANCH="unknown"
endif

HOSTNAME:=$(shell hostname -f)
BUILD_USER:=$(shell whoami)
PWD:=$(shell pwd)

.PHONY: build clean save-deps restore-deps test racetest test-integration

.DEFAULT_GOAL := all

all: test

#
# Dependencies
#

$(GOBINDIR)/sai-godep:
	go get github.com/anki/sai-godep

save-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep save github.com/anki/$(SERVICE)/...

restore-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep restore -v

#
# Protobufs
#

PROTO_FILES:=$(shell find . -name '*.proto')
PB_FILES=$(patsubst %.proto,%.pb.go, $(PROTO_FILES))

genpb: $(PB_FILES)

$(GOBINDIR)/protoc-gen-go:
	go install github.com/golang/protobuf/protoc-gen-go

%.pb.go: %.proto $(GOBINDIR)/protoc-gen-go
	protoc --go_out=plugins=grpc:. $<

#
# Testing
#

test:	genpb
	go test -v -timeout 5m github.com/anki/$(SERVICE)/... && echo "Tests Passed"

test-integration integration-docker: genpb
	go test -v -timeout 10m -tags integration github.com/anki/$(SERVICE)/... && echo "Tests Passed"

racetest:
	go test -v -timeout 5m -race github.com/anki/$(SERVICE)/... && echo "Tests Passed"

$(GOBINDIR)/pkgcover:
	go get github.com/anki/sai-tools/pkgcover

cover: $(GOBINDIR)/pkgcover
	mkdir -p _coverage
	$(GOBINDIR)/pkgcover \
		_coverage \
		github.com/anki/$(SERVICE)/...

cover-integration: $(GOBINDIR)/pkgcover
	mkdir -p _coverage
	$(GOBINDIR)/pkgcover \
		-testflags "-timeout 10m -tags integration" \
		_coverage \
		github.com/anki/$(SERVICE)/...

#
# Clean
#

CLEAN_FILES=\
	$(PB_FILES)

clean:
	rm -f $(CLEAN_FILES)
