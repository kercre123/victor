FROM ubuntu:16.04

ENV ENVIRONMENT loadtest
ENV ENABLE_ACCOUNT_CREATION false

ENV TEST_USER_PASSWORD ankisecret

ENV TEST_LOG_FILE /var/log/dmesg
ENV URL_CONFIG_FILE /integrationtest/server_config.json

# default to stdout logging (for local testing)
ENV LOG_TYPE stdout

ENV HEART_BEAT_INTERVAL 60s
ENV JDOCS_INTERVAL 600s
ENV LOG_COLLECTOR_INTERVAL 1800s
ENV TOKEN_REFRESH_INTERVAL 0s

RUN apt-get update
RUN apt-get --yes install libopus0 ca-certificates

# provision endpoint service URLs
ADD ./server_load_config.json /integrationtest/server_config.json
ADD ./saiconfig /root/.saiconfig

# install keys & certificates
ADD ./certs/device_certs.tgz /
ADD ./certs/trust.* /tmp/anki/gateway/

# install test runner
ADD ./robot_simulator /

CMD ["/robot_simulator"]
