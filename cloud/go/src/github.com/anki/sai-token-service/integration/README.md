# Testing with Local Services

The `docker-compose.yml` and `env` files provide an environment to run
the token service talking to local implementations of the AWS
services, rather than talking to AWS itself.

This has several benefits - it's faster, safer, and also allows for
offline testing.

## Launching the Local Services

Simple run `docker-compose up` in this directory.

```
cd integration
docker-compose up
```

## Configuring the Services

Configure the environment for connecting to the local services by
sourcing the `env` file in your shell. The `integration/setup.sh`
script will need to be run once to create the S3 bucket needed.

```
source integration/env
./integration/setup.sh
```

The `env` file will use your docker-machine IP, if it detects you have it
installed.  If not, it will use 127.0.0.1.
