# Docker

## Install
The following explains how to setup a Mac to build victor in docker. 
First, you'll need to install Docker for Mac.  Installation is pretty easy,
just follow the instructions on the [official Docker
Website](https://docs.docker.com/v17.12/docker-for-mac/install/).

## Tuning
In the Docker preferences got to "Advanced" and give Docker access to all CPUs
and enough RAM: ![Image of Docker settings](Mac_Docker_Settings.png)

## Adding packages
If a package is required for building victor it should be added to the
Dockerfile. If a package is only used during development it should
be added to the Dockerfile.dev specification.

## Using
Get a prompt in the victor-${USER} container
run.sh /bin/bash
