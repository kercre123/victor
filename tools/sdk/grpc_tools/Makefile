.PHONY:

GIT_ROOT:=$(shell git root)
GOPATH:=$(GIT_ROOT)/cloud/go

all: python golang

python: .PHONY
	python3 -m pip install grpcio grpcio-tools aiogrpc googleapis-common-protos
	python3 -m grpc_tools.protoc \
	-I $(GIT_ROOT)/cloud/gateway \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--python_out=. --grpc_python_out=. \
	$(GIT_ROOT)/cloud/gateway/external_interface.proto

golang: .PHONY
	@echo "Golang build is now part of cmake"

.require_ip: .PHONY
ifeq ("$(ANKI_ROBOT_HOST)","")
	$(error Could not find ANKI_ROBOT_HOST environment variable)
endif

gateway: .require_ip
	ssh root@$(ANKI_ROBOT_HOST) -t LD_LIBRARY_PATH=/anki/lib/ /anki/bin/vic-gateway

create_certs: .require_ip
	cp trust.conf temp_trust.conf
	echo "IP.2 = $(ANKI_ROBOT_HOST)" >> temp_trust.conf
	openssl req -config temp_trust.conf \
	-subj "/C=US/ST=California/L=SF/O=Anki/CN=localhost" \
	-new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout trust.key -out trust.cert
	rm temp_trust.conf
	scp trust.cert root@$(ANKI_ROBOT_HOST):/data/trust.cert
	scp trust.key root@$(ANKI_ROBOT_HOST):/data/trust.key

test_curl: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_wheels -d '{"left_wheel_mmps":100.0,"right_wheel_mmps":-100.0,"left_wheel_mmps2":0.0,"right_wheel_mmps2":0.0}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_wheels -d '{"left_wheel_mmps":0.0,"right_wheel_mmps":0.0,"left_wheel_mmps2":0.0,"right_wheel_mmps2":0.0}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/play_animation -d '{"animation":{"name":"anim_poked_giggle"},"loops":1}'
	@echo

help: .PHONY
	@env -i make -rnpq --no-print-directory .DEFAULT 2>/dev/null | grep "^[^%[:space:]]*:" | grep -v Makefile | awk '{ print substr($$1, 1, length($$1)-1) }' | sort | uniq | column