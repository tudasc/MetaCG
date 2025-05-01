function report {
  echo -e "\n[ --------------------------------- ] \n" >>"$logFile"
  echo "Running cgvalidate Integration Test $testNo | $failStr"
}

function failed {
  fails=$(($fails + 1))
  failStr=FAIL
  report
}

function passed {
  failStr=PASS
  report
}

function checkErrorCode {
  if [ $? -ne $1 ]; then
    failed
  else
    passed
  fi
  testNo=$(($testNo + 1))
}
