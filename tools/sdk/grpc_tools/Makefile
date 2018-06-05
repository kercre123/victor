.PHONY:

current_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
GIT_ROOT := "../../.."
PYTHON_STRUCTURE := vector/messaging
PYTHON_SDK_ROOT := $(abspath $(current_dir)/../vector-sdk)
PYTHON_OUT := $(abspath $(PYTHON_SDK_ROOT)/$(PYTHON_STRUCTURE))

all: python

python: .PHONY
	python3 -m pip install grpcio grpcio-tools aiogrpc googleapis-common-protos
	mkdir -p $(PYTHON_OUT)
	cp $(GIT_ROOT)/cloud/gateway/external_interface.proto $(PYTHON_OUT)

	# mkdir -p $(PYTHON_OUT)
	python3 -m grpc_tools.protoc \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--python_out=$(PYTHON_SDK_ROOT) --grpc_python_out=$(PYTHON_SDK_ROOT) \
	$(PYTHON_OUT)/external_interface.proto

flat: .PHONY
	flatc \
	-I /usr/local/include \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--proto \
	$(PYTHON_OUT)/external_interface.proto

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

test_curl_events: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/event_stream -d '{}'
	@echo

test_curl: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_wheels -d '{"left_wheel_mmps":100.0,"right_wheel_mmps":-100.0,"left_wheel_mmps2":0.0,"right_wheel_mmps2":0.0}'
	@echo
	sleep 2	
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_wheels -d '{"left_wheel_mmps":0.0,"right_wheel_mmps":0.0,"left_wheel_mmps2":0.0,"right_wheel_mmps2":0.0}'
	@echo
	
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/play_animation -d '{"animation":{"name":"anim_poked_giggle"},"loops":1}'
	@echo
	
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/move_head -d '{"speed_rad_per_sec":0.5}'
	@echo
	sleep 2
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/move_head -d '{"speed_rad_per_sec":0}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/move_lift -d '{"speed_rad_per_sec":0.5}'
	@echo
	sleep 2
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/move_lift -d '{"speed_rad_per_sec":0}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_arc -d '{"speed":5, "accel":2, "curvatureRadius_mm":10}'
	@echo
	sleep 2
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/drive_arc -d '{"speed":0, "accel":0, "curvatureRadius_mm":0}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/sdk_activation -d '{"slot":"SDK0"}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/cancel_sdk_activation -d '{"slot":"SDK0"}'
	@echo

update_time: .require_ip
	ssh root@$(ANKI_ROBOT_HOST) -t date --set='@$(shell date +%s)'

help: .PHONY
	@env -i make -rnpq --no-print-directory .DEFAULT 2>/dev/null | grep "^[^%[:space:]]*:" | grep -v Makefile | awk '{ print substr($$1, 1, length($$1)-1) }' | sort | uniq | column