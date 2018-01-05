#!/usr/bin/env bash
curTime="$(date +"%s")"
adb shell date @$curTime
