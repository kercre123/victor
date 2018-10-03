# Anki Vector SDK Scripts

## configure_local.py

Called by `mac_setup.sh` to update the sdk configuration file located at `~/.anki_vector/sdk_config.ini`.

## download_cert.sh

Downloads a robot's certificate from the cloud. Useful for investigating certificate issues.

## mac_setup.sh

Creates key/cert pair for webots sdk, and adds the relevant details to the sdk configuration file located at `~/.anki_vector/sdk_config.ini`.
This lets a user communicate with webots via the SDK, and is required for vic-gateway to start up correctly.
This script is called automatically as part of the mac build process.
If this needs to be called manually that is probably an error, and should be mentioned in the `#vic-coz-sdk` slack channel.

## sdk_lint.sh

Auto-format (`autopep8`) and lint (`pylint`) the python sdk code.

## update_proto.sh

Copies `.proto` files into the python sdk, makes minor modifications to their import paths, and builds the generated pb2 files.

## victor_curl.py

The `victor_curl` script is intended to bridge the ease of use of invoking curl with the credentials created
through the configure.py script. It will use the serial number of a robot that has been configured via the `configure.py`
script in `vector-sdk`, and then construct a relevant curl command to communicate with the robot.

### Configuration

To get set up with access to the robot, you will need to do the following steps:

1. Set up your robot via the vector app (or `mac-client` for advanced users)

1. Configure your laptop to have access to Vector by running

    ```bash
    ../vector-sdk/configure.py
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
