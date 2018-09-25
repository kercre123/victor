# Anki Vector SDK

The Python SDK currently lives under `tools/sdk/vector-sdk/`. Instructions to
use the SDK can be found [on confluence](https://ankiinc.atlassian.net/wiki/spaces/VD/pages/441319496/Python+Vector+SDK+-+Getting+Started).

## victor_curl.py

The `victor_curl` script is intended to bridge the ease of use of invoking curl with the credentials created
through the configure.py script. It will use the serial number of a robot that has been configured via the `configure.py`
script in `vector-sdk`, and then construct a relevant curl command to communicate with the robot.

### Configuration

To get set up with access to the robot, you will need to do the following steps:

1. Set up your robot via the vector app (or `mac-client` for advanced users)

1. Configure your laptop to have access to Vector by running

    ```bash
    vector-sdk/configure.py
    ```

    You should see "SUCCESS!" if everything worked properly. If not, please contact your friendly, neighborhood sdk team members via the `#vic-coz-sdk` slack channel.

1. Remember to re-run these steps anytime you clear user data.

### Usage

The script has a parameter for the serial number of your robot `-s`, `--serial`, or it will attempt
to use the environment variable `ANKI_ROBOT_SERIAL` if none is provided. This value is used to index
the configuration details necessary to connect to Vector.

To directly invoke the curl command:

```bash
eval $(./victor_curl.py)/v1/protocol_version -d '{}'
```

To get a curl command you can do what you want to:

```bash
./victor_curl.py
```

## gateway-tests

The `gateway-tests` directory contains a few tests scripts which may be executed with pytest.
Further details may be found [in grpc_tools's README](gateway-tests/README.md)

## grpc_tools

The `grpc_tools` directory is mainly useful for the Makefile contained within.
Everything in there is intended to be a temporary band aid until better processes
exist. Further details may be found [in grpc_tools's README](grpc_tools/README.md)
