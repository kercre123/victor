# NOTE: this whole file has an implicit
# "TODO: remove this when it is handled by the build for real."

.PHONY:

WEBOTS ?= 0
FORCE ?= 0

current_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
GIT_ROOT := $(abspath $(current_dir)/../../..)
PYTHON_STRUCTURE := anki_vector/messaging
PYTHON_SDK_ROOT := $(abspath $(current_dir)/../vector-sdk)
PYTHON_OUT := $(abspath $(PYTHON_SDK_ROOT)/$(PYTHON_STRUCTURE))
GATEWAY_GO_FILES := $(filter-out $(GIT_ROOT)/cloud/gateway/config_mac.go,$(wildcard $(GIT_ROOT)/cloud/gateway/*.go))

ifneq ("$(ANKI_ROBOT_HOST)","")
ROBOT_HOSTNAME := $(shell ssh -o ConnectTimeout=3 root@$(ANKI_ROBOT_HOST) -t echo "\$$HOSTNAME" 2>/dev/null)
endif

MAC_VECTOR_HOSTNAME := Vector-Local

ifeq ("$(WEBOTS)","1")
curl_command := curl --resolve $(MAC_VECTOR_HOSTNAME):8443:127.0.0.1 --cacert trust_mac.cert --header "Authorization: Bearer 8675309" https://$(MAC_VECTOR_HOSTNAME):8443
else
curl_command := curl --resolve $(ROBOT_HOSTNAME):443:$(ANKI_ROBOT_HOST) --cacert trust.cert --header "Authorization: Bearer 8675309" https://$(ROBOT_HOSTNAME)
endif

all: go python

pip: .PHONY
	python3 -m pip install grpcio grpcio-tools aiogrpc googleapis-common-protos

gen_py_proto: .PHONY
	mkdir -p $(PYTHON_OUT)
	cp $(GIT_ROOT)/cloud/gateway/*.proto $(PYTHON_OUT)
	sed -i -e 's/import "\([^\/]*\).proto"/import "anki_vector\/messaging\/\1.proto"/g' $(PYTHON_OUT)/*.proto
	rm $(PYTHON_OUT)/*.proto-e

go: .PHONY
	if [ ! -d "$(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway" ]; then \
		GOPATH=$(GIT_ROOT)/cloud/go/ go get github.com/grpc-ecosystem/grpc-gateway/...; \
	fi

python: go pip gen_py_proto
	python3 -m grpc_tools.protoc \
	-I $(PYTHON_SDK_ROOT) \
	-I $(GIT_ROOT)/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
	--python_out=$(PYTHON_SDK_ROOT) --grpc_python_out=$(PYTHON_SDK_ROOT) \
	$(PYTHON_OUT)/*.proto

# Extra verbose linting for go.
# This shouldn't be added to PR or anything, but is good to have available while developing go code.
go_vet: .PHONY
	@echo "> Running go vet"
	@GOPATH=$(GIT_ROOT)/cloud/go/:$(GIT_ROOT)/generated/go/:$(GIT_ROOT)/generated/cladgo/:$(GIT_ROOT)/tools/message-buffers/support/go go vet $(GATEWAY_GO_FILES)

pylint: pylint_tests pylint_examples pylint_main

autopep: .PHONY
	python3 -m pip install autopep8
	python3 -m autopep8 --ignore E501,E402 --in-place $(shell find ../vector-sdk/examples -type f -name "*.py") ../vector-sdk/tests/*.py ../vector-sdk/tests/automated/*.py ../vector-sdk/anki_vector/*.py

pylint_main: autopep
	@echo "> Running pylint on anki_vector module"
	@python3 -m pylint --rcfile=.pylint $(shell find ../vector-sdk/anki_vector -type f -d 1 -name "*.py") || exit 0

pylint_tests: autopep
	@echo "> Running pylint on test files"
	@python3 -m pylint --rcfile=.pylint $(shell find ../vector-sdk/tests ../vector-sdk/tests/automated -type f -d 1 -name "*.py") || exit 0

pylint_examples: autopep
	@echo "> Running pylint on anki_vector examples"
	@python3 -m pylint --rcfile=.pylint $(shell find ../vector-sdk/examples -type f -name "*.py") || exit 0


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
ifeq ("$(ROBOT_HOSTNAME)","")
	$(error Failed to find ROBOT_HOSTNAME)
endif

certs_show: .PHONY
	@echo "> Robot certificates"
	@openssl x509 -in trust.cert -text -noout || echo "\n\nFailed to output certs. Make sure they exist!\n\n" && exit 1
	@echo
	@echo "> Mac certificates"
	@openssl x509 -in trust_mac.cert -text -noout || echo "\n\nFailed to output certs. Make sure they exist!\n\n" && exit 1

create_certs: .require_ip
	@echo "> Certs are now on the robot to start with. This will pull gateway.cert to your mac"
	@echo scp root@$(ANKI_ROBOT_HOST):/data/vic-gateway/gateway.cert trust.cert
	@scp root@$(ANKI_ROBOT_HOST):/data/vic-gateway/gateway.cert trust.cert || (echo "You may need to remove trust.cert manually" && exit 1)
	chmod 660 trust.cert
certs: create_certs

download_cert:
ifeq ("$(ANKI_ROBOT_SERIAL)","")
	$(error Could not find ANKI_ROBOT_SERIAL environment variable)
endif
	curl https://session-certs.token.global.anki-services.com/vic/$(ANKI_ROBOT_SERIAL) > trust.cert
	@if grep -xq "\-----BEGIN CERTIFICATE-----" trust.cert; \
	then \
		echo "Download successful!"; \
	else \
		echo "Download failed!\n"; \
		cat trust.cert ; \
		echo "\n\nVerify that your robot has been logged in properly, and the certificate is in the cloud"; \
		exit 1; \
	fi

create_certs_mac: .PHONY
	cp trust.conf temp_trust.conf
	echo "DNS.1 = $(MAC_VECTOR_HOSTNAME)" >> temp_trust.conf
	openssl req -config temp_trust.conf \
	-subj "/C=US/ST=California/L=SF/O=Anki/CN=$(MAC_VECTOR_HOSTNAME)" \
	-new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout trust_mac.key -out trust_mac.cert
	rm temp_trust.conf
	mkdir -p /tmp/anki/gateway
	cp trust_mac.key /tmp/anki/gateway/trust.key
	cp trust_mac.cert /tmp/anki/gateway/trust.cert
	python3 configure-local.py
certs_mac: create_certs_mac

example_curl: .require_ip
	@echo $(curl_command)/v1/event_stream -d '{}'

test_curl_version: .require_ip
	$(curl_command)/v1/protocol_version -d '{}'
	@echo

test_curl_events: .require_ip
	$(curl_command)/v1/event_stream -d '{}'
	@echo

test_curl_events_white: .require_ip
	$(curl_command)/v1/event_stream -d '{"white_list": {"list": ["wake_word"]}}'
	@echo

test_curl_events_black: .require_ip
	$(curl_command)/v1/event_stream -d '{"black_list": {"list": ["robot_state", "robot_observed_face"]}}'
	@echo

test_get_events: .require_ip
	$(curl_command)/v1/event_stream
	@echo

test_robot_state_stream: .require_ip
	$(curl_command)/v1/robot_state_stream -d '{}'
	@echo

test_curl_battery_state: .require_ip
	$(curl_command)/v1/battery_state -d '{}'
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
	$(curl_command)/v1/pang -d '{"ping":20}'
	@echo

test_onboarding: .require_ip
	$(curl_command)/v1/get_onboarding_state -d '{}'
	@echo
	$(curl_command)/v1/send_onboarding_input -d '{"onboarding_get_step":{}}'
	@echo
	$(curl_command)/v1/send_onboarding_input -d '{"onboarding_continue":{}}'
	@echo

test_wifi: .require_ip
	$(curl_command)/v1/wifi_scan -d '{}'
	@echo
	$(curl_command)/v1/wifi_connect -d '{}'
	@echo
	$(curl_command)/v1/wifi_scan -d '{}'
	@echo
	$(curl_command)/v1/wifi_remove -d '{}'
	@echo
	$(curl_command)/v1/wifi_scan -d '{}'
	@echo
	$(curl_command)/v1/wifi_connect -d '{}'
	@echo
	$(curl_command)/v1/wifi_connect -d '{}'
	@echo
	$(curl_command)/v1/wifi_scan -d '{}'
	@echo
	$(curl_command)/v1/wifi_remove_all -d '{}'
	@echo
	$(curl_command)/v1/wifi_scan -d '{}'
	@echo

test_check_update_status: .require_ip
	$(curl_command)/v1/check_update_status -d '{}'
	@echo

test_update_and_restart: .require_ip
	$(curl_command)/v1/update_and_restart -d '{}'
	@echo

test_check_cloud_connection: .require_ip
	$(curl_command)/v1/check_cloud_connection -d '{}'
	@echo

test_user_authentication: .require_ip
	# Temporarily curl with "this is a test" base64 encoded until VIC-3490 gets done
	# Then the user_session_id will be the login token that the user received from
	# the Accounts service when logging in.
	$(curl_command)/v1/user_authentication -d '{ "user_session_id" : "dGhpcyBpcyBhIHRlc3Q=" }'
	@echo

test_list_animations: .require_ip
	$(curl_command)/v1/list_animations -d '{}'
	@echo

test_curl: .require_ip
	$(curl_command)/v1/sdk_activation -d '{"slot":0, "enable":true}'
	@echo

	$(curl_command)/v1/sdk_activation -d '{"slot":0, "enable":false}'
	@echo

	$(curl_command)/v1/display_face_image_rgb -d @test_assets/oled_color.json
	@echo

	$(curl_command)/v1/list_animations -d '{}'
	@echo

	$(curl_command)/v1/go_to_pose \
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

	$(curl_command)/v1/set_preferred_cube -d '{"factory_id":"11:11:11:11:11:11"}'
	@echo

	$(curl_command)/v1/forget_preferred_cube -d '{}'
	@echo

	$(curl_command)/v1/connect_cube -d '{}'
	@echo

	$(curl_command)/v1/flash_cube_lights -d '{}'
	@echo

	sleep 2
	$(curl_command)/v1/disconnect_cube -d '{}'
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

test_pull_jdocs: .require_ip
	$(curl_command)/v1/pull_jdocs -d '{"jdoc_types":["ROBOT_SETTINGS", "ROBOT_LIFETIME_STATS"]}'
	@echo

test_account_settings: .require_ip
	$(curl_command)/v1/update_account_settings -d '{"account_settings": {"app_locale":"en-US","data_collection":true}}'
	@echo

test_user_entitlements: .require_ip
	$(curl_command)/v1/update_user_entitlements -d '{"user_entitlements": {"kickstarter_eyes":true}}'
	@echo

test_status_history: .require_ip
	$(curl_command)/v1/status_history -d '{}'
	@echo

test_cubes_available: .require_ip
	$(curl_command)/v1/cubes_available -d '{}'
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
	@$< --port 8443 &> logs/$(notdir $<).log || ( tput setaf 1 ; cat logs/$(notdir $<).log ; tput sgr0 ; exit 1 )
else
	@$< &> logs/$(notdir $<).log || ( tput setaf 1 ; cat logs/$(notdir $<).log ; tput sgr0 ; exit 1 )
endif

run_python_tests: $(addprefix run_,$(TEST_FILES))
