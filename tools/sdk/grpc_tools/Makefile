# NOTE: this whole file has an implicit
# "TODO: remove this when it is handled by the build for real."

.PHONY:

WEBOTS ?= 0
FORCE ?= 0

current_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
GIT_ROOT := $(abspath $(current_dir)/../../..)
PYTHON_STRUCTURE := vector/messaging
PYTHON_SDK_ROOT := $(abspath $(current_dir)/../vector-sdk)
PYTHON_OUT := $(abspath $(PYTHON_SDK_ROOT)/$(PYTHON_STRUCTURE))

ifeq ("$(WEBOTS)","1")
curl_command := curl --cacert trust_mac.cert https://localhost:8443
else
curl_command := curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)
endif

all: python

pip: .PHONY
	python3 -m pip install grpcio grpcio-tools aiogrpc googleapis-common-protos

gen_py_proto: .PHONY
	mkdir -p $(PYTHON_OUT)
	cp $(GIT_ROOT)/cloud/gateway/*.proto $(PYTHON_OUT)
	sed -i -e 's/import "\([^\/]*\).proto"/import "vector\/messaging\/\1.proto"/g' $(PYTHON_OUT)/*.proto
	rm $(PYTHON_OUT)/*.proto-e

python: pip gen_py_proto
	python3 -m grpc_tools.protoc \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--python_out=$(PYTHON_SDK_ROOT) --grpc_python_out=$(PYTHON_SDK_ROOT) \
	$(PYTHON_OUT)/*.proto

# Extra verbose linting for go.
# This shouldn't be added to PR or anything, but is good to have available while developing go code.
go_vet: .PHONY
	@echo "> Running go vet"
	@GOPATH=$(GIT_ROOT)/cloud/go/:$(GIT_ROOT)/generated/go/:$(GIT_ROOT)/generated/cladgo/:$(GIT_ROOT)/tools/message-buffers/support/go go vet $(GIT_ROOT)/cloud/gateway/main.go $(GIT_ROOT)/cloud/gateway/message_handler.go $(GIT_ROOT)/cloud/gateway/config_mac.go

pylint: pylint_tests pylint_main

autopep: .PHONY
	python3 -m pip install autopep8
	python3 -m autopep8 --ignore E501,E402 --in-place ../vector-sdk/tests/*.py ../vector-sdk/tests/automated/*.py ../vector-sdk/vector/*.py

pylint_main: autopep
	@echo "> Running pylint on vector module"
	@python3 -m pylint --rcfile=.pylint $(shell find ../vector-sdk/vector -type f -d 1) || exit 0

pylint_tests: autopep
	@echo "> Running pylint on test files"
	@python3 -m pylint --rcfile=.pylint $(shell find ../vector-sdk/tests ../vector-sdk/tests/automated -type f -d 1) || exit 0

# Unused flatbuffers target. Keeping this for posterity
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
	$(warning "the gateway target is no longer necessary. To force this to run, use 'make gateway FORCE=1'")
ifeq ("$(FORCE)","1")
	ssh root@$(ANKI_ROBOT_HOST) -t LD_LIBRARY_PATH=/anki/lib/ /anki/bin/vic-gateway
endif

certs_show: .PHONY
	@echo "> Robot certificates"
	@openssl x509 -in trust.cert -text -noout || echo "\n\nFailed to output certs. Make sure they exist!\n\n" && exit 1
	@echo
	@echo "> Mac certificates"
	@openssl x509 -in trust_mac.cert -text -noout || echo "\n\nFailed to output certs. Make sure they exist!\n\n" && exit 1

create_certs: .require_ip
	cp trust.conf temp_trust.conf
	echo "IP.2 = $(ANKI_ROBOT_HOST)" >> temp_trust.conf
	openssl req -config temp_trust.conf \
	-subj "/C=US/ST=California/L=SF/O=Anki/CN=localhost" \
	-new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout trust.key -out trust.cert
	rm temp_trust.conf
	scp trust.cert root@$(ANKI_ROBOT_HOST):/data/trust.cert
	scp trust.key root@$(ANKI_ROBOT_HOST):/data/trust.key
certs: create_certs

create_certs_mac: .PHONY
	cp trust.conf temp_trust.conf
	echo "DNS.2       = localhost:8443" >> temp_trust.conf
	openssl req -config temp_trust.conf \
	-subj "/C=US/ST=California/L=SF/O=Anki/CN=localhost" \
	-new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout trust_mac.key -out trust_mac.cert
	rm temp_trust.conf
	mkdir -p /tmp/anki/gateway
	cp trust_mac.key /tmp/anki/gateway/trust.key
	cp trust_mac.cert /tmp/anki/gateway/trust.cert
certs_mac: create_certs_mac

test_curl_events: .require_ip
	$(curl_command)/v1/event_stream -d '{}'
	@echo

test_get_events: .require_ip
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/event_stream
	@echo

test_robot_state_stream: .require_ip
	$(curl_command)/v1/robot_state_stream -d '{}'
	@echo

test_eye_colors: .require_ip
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 0}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 1}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 2}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 3}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 4}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 5}}'
	@echo
	sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 6}}'
	@echo
	sleep .5
	# TODO: handle out-of-bounds properly
	# $(curl_command)/v1/update_settings -d '{"settings": {"eye_color": 7}}'
	# @echo
	# sleep .5
	$(curl_command)/v1/update_settings -d '{"settings": {"eye_color": "PURPLE"}}'
	@echo

test_drive_off_charger: .require_ip
	$(curl_command)/v1/drive_off_charger -d '{}'
	@echo

test_drive_on_charger: .require_ip
	$(curl_command)/v1/drive_on_charger -d '{}'
	@echo

test_ping_pong: .require_ip
	$(curl_command)/v1/pang -d '{"ping":10}'
	@echo
	$(curl_command)/v1/bang -d '{"bing":"SomeThing"}'
	@echo
	$(curl_command)/v1/pang -d '{"ping":20}'
	@echo
	$(curl_command)/v1/bang -d '{"bing":"zome_Thing"}'
	@echo

test_onboarding: .require_ip
	$(curl_command)/v1/get_onboarding_state -d '{}'
	@echo
	$(curl_command)/v1/send_onboarding_input -d '{"onboarding_continue":{}}'
	@echo

test_wifi: .require_ip
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_scan -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_connect -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_scan -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_remove -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_scan -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_connect -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_connect -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_scan -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_remove_all -d '{}'
	@echo
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/wifi_scan -d '{}'
	@echo

test_user_authentication: .require_ip
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/user_authentication -d '{}'
	@echo

test_curl: .require_ip
	$(curl_command)/v1/sdk_activation -d '{"slot":0, "enable":true}'
	@echo

	$(curl_command)/v1/sdk_activation -d '{"slot":0, "enable":false}'
	@echo

	$(curl_command)/v1/display_face_image_rgb -d @test_assets/oled_color.json
	@echo

	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/list_animations -d '{}'
	@echo

	curl --cacert trust.cert -X POST https://$(ANKI_ROBOT_HOST)/v1/go_to_pose \
	-d '{"x_mm":100.0, "y_mm":100.0, "rad":0.0, \
		"motion_prof":{"speed_mmps":110.0, \
				"accel_mmps2":200.0, \
				"decel_mmps2": 500.0, \
				"point_turn_speed_rad_per_sec": 2.0, \
				"point_turn_accel_rad_per_sec2": 10.0, \
				"point_turn_decel_rad_per_sec2": 10.0, \
				"dock_speed_mmps": 60.0, \
				"dock_accel_mmps2": 200.0, \
				"dock_decel_mmps2": 500.0, \
				"reverse_speed_mmps": 80.0, \
				"is_custom": false}}'
	@echo

test_meet_victor: .require_ip
	# First, turn on behaviors (by shaking), then say "Hey Vector, my name is Bobert"

	# Should return list of known people
	$(curl_command)/v1/request_enrolled_names -d '{}'
	@echo

	$(curl_command)/v1/update_enrolled_face_by_id -d '{"face_id":1,"old_name":"Bobert","new_name":"Boberta"}'
	@echo

	$(curl_command)/v1/set_face_to_enroll -d '{"name":"Boberta","observed_id":1,"save_id":0,"save_to_robot":true,"say_name":true,"use_music":true}'
	@echo

	$(curl_command)/v1/cancel_face_enrollment -d '{}'
	@echo

	$(curl_command)/v1/erase_enrolled_face_by_id -d '{"face_id":1}'
	@echo

	$(curl_command)/v1/erase_all_enrolled_faces -d '{}'
	@echo

	$(curl_command)/v1/app_intent -d '{"intent":"intent_meet_victor","param":"Bobert"}'
	@echo

	# Should be empty list of names
	$(curl_command)/v1/request_enrolled_names -d '{}'
	@echo

test_photos: .require_ip
	$(curl_command)/v1/photos_info -d '{}'
	@echo
	$(curl_command)/v1/photo -d '{"photo_id":5}'
	@echo
	$(curl_command)/v1/thumbnail -d '{"photo_id":5}'
	@echo
	$(curl_command)/v1/delete_photo -d '{"photo_id":5}'
	@echo

test_attention_transfer: .require_ip
	$(curl_command)/v1/get_latest_attention_transfer -d '{}'
	@echo

test_settings: .require_ip
	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/push_settings -d '{}'
	@echo

	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/pull_settings -d '{}'
	@echo

	curl --cacert trust.cert https://$(ANKI_ROBOT_HOST)/v1/update_settings -d '{"settings":{"clock_24_hour":true,"eye_color":"TEAL"}}'
	@echo

test_status_history: .require_ip
	$(curl_command)/v1/status_history -d '{}'
	@echo

update_time: .require_ip
	ssh root@$(ANKI_ROBOT_HOST) -t date --set='@$(shell date +%s)'

help: .PHONY
	@env -i make -rnpq --no-print-directory .DEFAULT 2>/dev/null | grep "^[^%[:space:]]*:" | grep -v Makefile | awk '{ print substr($$1, 1, length($$1)-1) }' | sort | uniq | column

IGNORE_FILES := test_backpack_light_profiles.py test_backpack_lights.py test_cube_lights.py test_say_text.py
TEST_FILES := $(shell find ../vector-sdk/tests ../vector-sdk/tests/automated -type f -d 1 $(foreach file,$(IGNORE_FILES),| grep -v $(file)))

$(addprefix run_,$(TEST_FILES)): run_%: % .require_ip
	@mkdir -p logs
	@echo "> Running $(shell tput setaf 6)$(notdir $<)$(shell tput sgr0)"
ifeq ("$(WEBOTS)","1")
	@$< localhost trust_mac.cert --port 8443 &> logs/$(notdir $<).log || ( tput setaf 1 ; cat logs/$(notdir $<).log ; tput sgr0 ; exit 1 )
else
	@$< $(ANKI_ROBOT_HOST) trust.cert &> logs/$(notdir $<).log || ( tput setaf 1 ; cat logs/$(notdir $<).log ; tput sgr0 ; exit 1 )
endif

run_python_tests: $(addprefix run_,$(TEST_FILES))
