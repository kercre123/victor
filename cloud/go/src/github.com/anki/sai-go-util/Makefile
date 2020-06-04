PACKAGE=sai-go-util

GOBINDIR=$(shell echo $(GOPATH) |  awk -F: '{ print $$NF }')/bin

.PHONY: test racetest save-deps restore-deps

.DEFAULT_GOAL := all

# Set the NO_SAML environment variable to avoid linking with libxmlsec1 et al
ifdef NO_SAML
SAMLTAG=nolibxmlsec1
SAMLBUILD=-tags $(SAMLTAG)
endif

all: test

ifdef GOBINDIR
clean:
	rm -f $(GOBINDIR)/*
endif

#
# Dependencies
#

$(GOBINDIR)/sai-godep:
	go get github.com/anki/sai-godep

save-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep save github.com/anki/$(PACKAGE)/...

restore-deps: $(GOBINDIR)/sai-godep
	$(GOBINDIR)/sai-godep restore -v

#
# Testing
#

test:
	go test $(SAMLBUILD) github.com/anki/$(PACKAGE)/... && echo "Tests Passed"

test-integration:
	go test -v -timeout 10m -tags 'integration $(SAMLTAG)' github.com/anki/$(PACKAGE)/... && echo "Tests Passed"

racetest:
	go test -v -race $(SAMLBUILD) github.com/anki/$(PACKAGE)/...

$(GOBINDIR)/pkgcover:
	go get github.com/anki/sai-tools/pkgcover

cover: $(GOBINDIR)/pkgcover
	mkdir -p _coverage
	$(GOBINDIR)/pkgcover \
		-testflags "$(SAMLBUILD)" \
		_coverage \
		github.com/anki/$(PACKAGE)/...

cover-integration: $(GOBINDIR)/pkgcover
	mkdir -p _coverage
	$(GOBINDIR)/pkgcover \
		-testflags "-timeout 10m -tags 'integration $(SAMLTAG)'" \
		_coverage \
		github.com/anki/$(PACKAGE)/...
