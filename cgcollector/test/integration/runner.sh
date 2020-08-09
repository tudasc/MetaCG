#!/bin/bash

fails=0

cd cgverify
bash cgverify_run.sh

fails=$(($fails+$?))

echo Failed tests: $fails
exit $fails

