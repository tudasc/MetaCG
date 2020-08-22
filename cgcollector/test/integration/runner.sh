#!/bin/bash

fails=0

cd cgvalidate
bash cgvalidate-run.sh

fails=$(($fails+$?))

echo Failed tests: $fails
exit $fails

