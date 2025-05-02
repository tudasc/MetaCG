#!/bin/bash

fails=0

cd cgvalidate
bash cgvalidate-run.sh -b "$1"

fails=$(($fails + $?))

echo Failed tests: $fails
exit $fails
