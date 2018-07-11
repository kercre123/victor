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


## Running `vic-gateway` on mac (with webots)

vic-gateway builds on mac and can be used with webots, but isn't automatically launched. To use it with a
simulated robot in webots (on mac), do the following:

1. Do a normal mac build (` ./project/victor/build-victor.sh -p mac -c Debug -f`)
2. `> make create_certs_mac`
3. Launch a webots sim world (like `victor/simulator/worlds/victorObservingDemo.wbt`)
4. From the victor root directory, in another terminal, run `_build/mac/Debug/bin/vic-gateway`

You may have to click "allow" on an OS popup to allow listening. After this is done, you can run direct curl commands similar to those found in the `Makefile`, e.g.:

``` bash
> curl --cacert trust.cert -X POST https://localhost:8443/v1/event_stream -d '{}'
```

All the curl tests should work as well using the WEBOTS=1 flag in make

```bash
> make test_ping_pong WEBOTS=1
```

Note that you need to use localhost as the ip and 8443 as the port (OS X gets angry if you try to listen on 443)
