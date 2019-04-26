GOFILES=$(shell find . -name '*.go'  \! -name '*.rice-box.go')
TEMPLATES=$(shell find email/templates -name '*.txt' -o -name '*.html')
GOBINDIR=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/bin

ifndef GIT_BRANCH
GIT_BRANCH=$(shell git symbolic-ref --short HEAD)
endif
ifndef GIT_COMMIT
GIT_COMMIT=$(shell git rev-parse HEAD)
endif

ifeq ($(GIT_BRANCH),)
GIT_BRANCH="unknown"
endif

# Set the NO_SAML environment variable to avoid linking with libxmlsec1 et al
ifdef NO_SAML
SAMLBUILD=-tags nolibxmlsec1
endif

HOSTNAME=$(shell hostname -f)
BUILD_USER=$(shell whoami)
PWD=$(shell pwd)

.PHONY: build clean save-deps restore-deps test sqlitetest mysqltest racetest integration-docker

.DEFAULT_GOAL := all



all: build

build: $(GOBINDIR)/sai-go-accounts

ifdef GOBINDIR
clean:
	rm -f $(GOBINDIR)/sai-go-accounts
endif

$(GOBINDIR)/godep:
	# Fetch a fixed version of Godep that uses the old file format
	mkdir -p $(GOBINDIR)
	curl -o $(GOBINDIR)/godep https://anki-sai.s3.amazonaws.com/utils/$(shell uname -s)/godep
	chmod +x $(GOBINDIR)/godep

save-deps: $(GOBINDIR)/godep
	rm -f Godeps  # only needed as godep update is broken for copy=false
	$(GOBINDIR)/godep save -copy=false github.com/anki/sai-go-accounts/...

restore-deps: $(GOBINDIR)/godep
	# Bug with godep necessitates running this twice until a proper fix is supplied
	$(GOBINDIR)/godep restore || $(GOBINDIR)/godep restore

sqlitetest:
	#go vet github.com/anki/sai-go-accounts/...
	POSTMARK_API_KEY=NO_EMAIL \
		URL_ROOT="https://accounts.api.anki.com" \
		DB_TYPE=sqlite go test $(SAMLBUILD) -timeout 5m github.com/anki/sai-go-accounts/... && echo "Tests" "Passed"

mysqltest:
	#go vet github.com/anki/sai-go-accounts/...
	POSTMARK_API_KEY=NO_EMAIL \
		URL_ROOT="https://accounts.api.anki.com" \
		DB_TYPE=mysql go test -timeout 5m github.com/anki/sai-go-accounts/...

racetest:
	POSTMARK_API_KEY=NO_EMAIL \
		URL_ROOT="https://accounts.api.anki.com" \
		DB_TYPE=mysql go test $(SAMLBUILD) -timeout 10m -race github.com/anki/sai-go-accounts/...

integration-docker: build
	 DUMP_LOG=true USE_DOCKER=true ACCOUNTS_BIN=$(GOBINDIR)/sai-go-accounts \
			  go test -timeout 5m github.com/anki/sai-go-accounts/integration -integration

# Run some named benchmarks from the integration library
# eg. make integration-bench BENCHTEST=BenchmarkLogin
# This will run the benchmark for one minute and collect memory profile infomration
# change mem to cpu to collect cpu profiles
# Note: The mysql docker environment is setup and torn down for each run of the benchmark
# and Go's benchmark code may run a benchmark multiple time sto get an accurate number, so this
# may take several minutes to run.
integration-bench: build
	 DUMP_LOG=true USE_DOCKER=true ACCOUNTS_BIN=$(GOBINDIR)/sai-go-accounts \
			  go test -v -bench $(BENCHTEST) -benchtime 1m github.com/anki/sai-go-accounts/integration -profile-mode mem


# Gareth's experimental package coverage
pkgcover: email/templates.rice-box.go
	POSTMARK_API_KEY=NO_EMAIL \
	URL_ROOT="https://accounts.api.anki.com" \
    DB_TYPE=sqlite gopkgcover \
    -exclude github.com/anki/sai-go-accounts \
    -exclude github.com/anki/sai-go-accounts/apis \
    -exclude github.com/anki/sai-go-accounts/apierrors \
    -exclude github.com/anki/sai-go-accounts/db/dbtest \
    -exclude github.com/anki/sai-go-accounts/integration/... \
    -exclude github.com/anki/sai-go-accounts/models/user/usertest \
    -exclude github.com/anki/sai-go-accounts/testutil \
    $(COVERARGS)  github.com/anki/sai-go-accounts/...


test: sqlitetest racetest

# rice handles embedding files into .go files
$(GOBINDIR)/rice:
	go get github.com/GeertJohan/go.rice/rice
	go install github.com/GeertJohan/go.rice/rice

# compile the email templates into a .go file to be linked in
email/templates.rice-box.go: $(GOBINDIR)/rice $(GOFILES) $(TEMPLATES)
	cd email &&  $(GOBINDIR)/rice embed-go

emailbox: email/templates.rice-box.go

BUILDVAR=github.com/anki/sai-go-util/buildinfo
$(GOBINDIR)/sai-go-accounts: email/templates.rice-box.go $(GOFILES)
	go install $(SAMLBUILD) -ldflags \
		"-X $(BUILDVAR).product=accounts \
		-X $(BUILDVAR).buildTimeUnix=`date +%s` \
		-X $(BUILDVAR).commit=$(GIT_COMMIT) \
		-X $(BUILDVAR).branch=$(GIT_BRANCH) \
		-X $(BUILDVAR).buildHost=$(HOSTNAME)     \
		-X $(BUILDVAR).buildUser=$(BUILD_USER)"  \
                github.com/anki/sai-go-accounts
