FROM ubuntu:16.04

# install requried packages
RUN apt-get update
RUN apt-get --yes install libopus0 ca-certificates

# install keys & certificates
ADD ./certs/device_certs.tgz /

ENV ENVIRONMENT loadtest

ENV ENABLE_ACCOUNT_CREATION false
ENV ENABLE_DISTRIBUTED_CONTROL false

ENV ROBOTS_PER_PROCESS 1
ENV TASKS_PER_CLUSTER 1

ENV TEST_USER_PASSWORD ankisecret

ENV TEST_LOG_FILE /var/log/dmesg
ENV URL_CONFIG_FILE /integrationtest/server_config.json

# default to stdout logging (for local testing)
ENV LOG_TYPE stdout

ENV RAMP_UP_DURATION 0s
ENV RAMP_DOWN_DURATION 0s

ENV HEART_BEAT_INTERVAL 60s
ENV HEART_BEAT_STDDEV 3s
ENV JDOCS_INTERVAL 600s
ENV JDOCS_STDDEV 0s
ENV LOG_COLLECTOR_INTERVAL 1800s
ENV LOG_COLLECTOR_STDDEV 0s
ENV TOKEN_REFRESH_INTERVAL 0s
ENV TOKEN_REFRESH_STDDEV 0s
ENV CONNECTION_CHECK_INTERVAL 5m
ENV CONNECTION_CHECK_STDDEV 0s

# provision configuration files
ADD ./server_load_config.json /integrationtest/server_config.json
ADD ./saiconfig /root/.saiconfig
ADD ./certs/trust.* /tmp/anki/gateway/

# install test runner
ADD ./robot_simulator /

CMD ["/robot_simulator"]
