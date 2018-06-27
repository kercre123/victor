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
	cp $(GIT_ROOT)/cloud/gateway/*.proto $(PYTHON_OUT)
	sed -i -e 's/"messages.proto"/"vector\/messaging\/messages.proto"/g' $(PYTHON_OUT)/external_interface.proto

	# mkdir -p $(PYTHON_OUT)
	python3 -m grpc_tools.protoc \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--python_out=$(PYTHON_SDK_ROOT) --grpc_python_out=$(PYTHON_SDK_ROOT) \
	$(PYTHON_OUT)/*.proto

flat: .PHONY
	flatc \
	-I /usr/local/include \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--proto \
	$(PYTHON_OUT)/*.proto

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

create_certs_mac:
	cp trust.conf temp_trust.conf
	echo "DNS.2       = localhost:8443" >> temp_trust.conf
	openssl req -config temp_trust.conf \
	-subj "/C=US/ST=California/L=SF/O=Anki/CN=localhost" \
	-new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout trust.key -out trust.cert
	rm temp_trust.conf
	mkdir -p /tmp/anki/gateway
	cp trust.key /tmp/anki/gateway
	cp trust.cert /tmp/anki/gateway


test_curl_events: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/event_stream -d '{}'
	@echo

test_robot_state_stream: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/robot_state_stream -d '{}'
	@echo

test_settings_fake: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/settings -d '{}'
	@echo

test_ping_pong: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/pang -d '{"ping":10}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/bang -d '{"bing":"SomeThing"}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/pang -d '{"ping":20}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/bang -d '{"bing":"zome_Thing"}'
	@echo

test_onboarding: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/get_onboarding_state -d '{}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/send_onboarding_input -d '{"onboarding_continue":{}}'
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

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/display_face_image_rgb -d @test_assets/oled_color.json
	@echo

test_meet_victor: .require_ip
	# First, turn on behaviors (by shaking), then say "Hey Vector, my name is Bobert"

	# Should return list of known people
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/request_enrolled_names -d '{}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/update_enrolled_face_by_id -d '{"face_id":1,"old_name":"Bobert","new_name":"Boberta"}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/set_face_to_enroll -d '{"name":"Boberta","observed_id":1,"save_id":0,"save_to_robot":true,"say_name":true,"use_music":true}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/cancel_face_enrollment -d '{}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/erase_enrolled_face_by_id -d '{"face_id":1}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/erase_all_enrolled_faces -d '{}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/app_intent -d '{"intent":"intent_meet_victor","param":"Bobert"}'
	@echo

	# Should be empty list of names
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/request_enrolled_names -d '{}'
	@echo

test_photos: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/photos_info -d '{}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/photo -d '{"photo_id":5}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/thumbnail -d '{"photo_id":5}'
	@echo
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/delete_photo -d '{"photo_id":5}'
	@echo

test_attention_transfer: .require_ip
	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/get_latest_attention_transfer -d '{}'
	@echo


update_time: .require_ip
	ssh root@$(ANKI_ROBOT_HOST) -t date --set='@$(shell date +%s)'

help: .PHONY
	@env -i make -rnpq --no-print-directory .DEFAULT 2>/dev/null | grep "^[^%[:space:]]*:" | grep -v Makefile | awk '{ print substr($$1, 1, length($$1)-1) }' | sort | uniq | column
