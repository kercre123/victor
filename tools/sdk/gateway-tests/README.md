# Gateway Tests

Gateway Tests provide a way of mimicing the communication between the app and gateway.
It uses the protobuf definitions, creates an object from the generated definition, and will send
that data as json to the given endpoints.

All new test files and test functions should start with `test_` to be auto-discovered by pytest.

## Requirements

To run these tests, you'll need the the Python SDK which currently lives under
`tools/sdk/vector-sdk/`. Instructions to use the SDK can be found [on confluence](https://ankiinc.atlassian.net/wiki/spaces/VD/pages/441319496/Python+Vector+SDK+-+Getting+Started).

In addition to properly installing the Python SDK, the Gateway Tests require
pytest, requests and requests_toolbelt. These may be installed via:

```bash
pip3 install pytest requests requests_toolbelt
```

It is also recommended but not required to install `termcolor` (`pip3 install termcolor`) to add colored output to the scripts for added readability.

Lastly, you must set the environment variable `ANKI_ROBOT_SERIAL`. This will fill out all
of the authorization credentials to test against your Vector. Note: you may use `Local` as the serial to communicate with Webots.

## Usage

To run all the tests:

```bash
pytest
```

To show all the output from the tests:

```bash
pytest -s
```

To run a specific test (and see the output):

```bash
pytest -s test_json_rpcs.py::test_protocol_version
```

To test the longevity of the event stream, you may use a special script which will only exit when there is an error:

```bash
pytest -s test_json_streaming.py::test_event_forever
```
