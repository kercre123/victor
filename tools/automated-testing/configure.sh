#! /bin/bash

PROJECT_DIR=`git rev-parse --show-toplevel`
AUTOTEST_DIR=$PROJECT_DIR/_build/vicos/Release/auto-test/bin
AUTOTEST_TOOL_DIR=$PROJECT_DIR/tools/automated-testing

# install dependencies
npm install $AUTOTEST_TOOL_DIR