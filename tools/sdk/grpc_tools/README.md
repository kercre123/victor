# grpc_tools

While this will all be added to proper build steps, this folder is used to create everything in the interim to unblock work with grpc.

## Setup

For first time setup you will need to do the following:
1. Set `ANKI_ROBOT_HOST` to point at the ip of your robot
2. Generate python files from the `.proto` file

```bash
> make
```
3. Make and install keys on your robot for your ip

```bash
> make create_certs
```

## Building `vic-gateway`

This is already built into the cmake files, and will build when you run the cmake build.

## Running `vic-gateway`

In a free terminal, run
```bash
> make gateway
```
And, in another terminal, capture the logs with
```bash
> victor_log | grep vic-gateway
```
Use Ctrl+C to exit

## Testing `vic-gateway`

To test python, run the scripts inside `tools/sdk/vector-sdk/tests`.
Note: They will require an ip and the path to the trust.cert file for your robot.
Also, not all of them work yet, but they are being converted over as the messages are converted over.

To test the rest api, use `> make test_curl`, or use `> make test_curl_events` to test events coming from the engine.

Soon™, these curl tests will be replaced by the chewie app.
