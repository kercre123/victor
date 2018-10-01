FROM ubuntu:16.04

ENV ENVIRONMENT loadtest
ENV ENABLE_ACCOUNT_CREATION false

ENV TEST_ID 1
ENV TEST_USER_PASSWORD ankisecret

ENV TEST_LOG_FILE /var/log/syslog
ENV URL_CONFIG_FILE integrationtest/server_config.json

RUN apt-get update
RUN apt-get --yes install libopus0 ca-certificates

# provision endpoint service URLs
ADD ./server_load_config.json /integrationtest/server_config.json
ADD ./saiconfig /root/.saiconfig

# install keys & certificates
ADD ./certs/device_certs.tgz /
ADD ./certs/trust.* /tmp/anki/gateway/

# install test runner
ADD ./integrationtest.test /

CMD /integrationtest.test -testify.m TestPrimaryPairingSequence
