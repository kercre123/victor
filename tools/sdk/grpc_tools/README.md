# grpc_tools

While this will all be added to proper build steps, this folder is used to create everything in the interim to unblock work with grpc.

## Setup

For first time setup you will need to do the following:
1. Set `ANKI_ROBOT_HOST` to point at the ip of your robot
2. Generate files from the `.proto` file

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
Use Ctrl+C to exit

## Testing `vic-gateway`

To test python, currently use `> python3 grpc_test.py`

To test the rest api, use `> make test_curl`

Soon™, these will be replaced by the vector-sdk and the chewie app respectively.
