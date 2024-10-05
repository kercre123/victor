#!/bin/bash

cd $HOME/snowboy

docker run -it -v "$1":/snowboy-master/examples/Python/model snowboy-pmdl