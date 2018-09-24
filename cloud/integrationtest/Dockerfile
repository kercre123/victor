FROM ubuntu:16.04

RUN apt-get update
RUN apt-get --yes install libopus0 ca-certificates

# provision endpoint service URLs
ADD ./server_config.json /integrationtest/

# install keys & certificates
ADD ./certs/device_certs.tgz /
ADD ./certs/trust.* /tmp/anki/gateway/

# install test runner
ADD ./integrationtest.test /

CMD /integrationtest.test -testify.m TestPrimaryPairingSequence
