# grpc_tools

While this will all be added to proper build steps, this folder is used to create everything in the interim to unblock work with grpc.

## Setup

For first time setup you will need to do the following:

1. Set `ANKI_ROBOT_HOST` to point at the ip of your robot
1. Generate python files from the `.proto` file

```bash
> make
```

## Building `vic-gateway`

This is already built into the cmake files, and will build when you run the cmake build.

## Testing `vic-gateway`

You can capture logs off a running robot with:

```bash
> victor_log | grep vic-gateway
```

To test python, run the scripts inside `tools/sdk/vector-sdk/tests`.
Note: They will require the robot name, an ip and the path to the trust.cert file for your robot.
These can all be set via environment variables (run the script to see their names).

Also, not all of them work yet, but they are being converted over as the messages are converted over.

To test the rest api, use `> make test_curl`, or use `> make test_curl_events` to test events coming from the engine.

Soon™, these curl tests will be replaced by the chewie app, or there will be separate python tests that use rest directly.

All the curl tests should work as well using the WEBOTS=1 flag in make

```bash
> make test_ping_pong WEBOTS=1
```

## Running `vic-gateway` on mac (with webots)

`vic-gateway` is built for mac as part of the usual victor build, and launches automatically when a webots sim is started.

1. Do a normal mac build (` ./project/victor/build-victor.sh -p mac -c Debug -f`)
1. Launch a webots sim world (like `victor/simulator/worlds/victorObservingDemo.wbt`)
1. You can run direct curl commands similar to those found in the `Makefile`. See an example with:

``` bash
> make example_curl WEBOTS=1
```
