#!/bin/bash

. base.sh

fails=0
testNo=0
logDir=$PWD/logging
logFile=$logDir/cgverify.log
inputDir=$PWD/input
buildDir=$PWD/../../../build/tools/
executable=$buildDir/cgverify

#cleanup
rm -rf $logDir
mkdir $logDir

# check if cgverify is available
if ! type $executable > /dev/null; then
  failed
  exit 1
else
  passed
  testNo=$(($testNo+1))
fi

# wrong input files
$executable $inputDir/0001.cubex $inputDir/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/0001.ipcg $inputDir/0001.ipcg >> $logFile
checkErrorCode 1

# missing edges
$executable $inputDir/0001_missedcallee.ipcg $inputDir/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/0001_missedparent.ipcg $inputDir/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/0001_missedboth.ipcg $inputDir/0001.cubex >> $logFile
checkErrorCode 1

# successfull run
$executable $inputDir/0001.ipcg $inputDir/0001.cubex >> $logFile
checkErrorCode 0

# finalize
exit $fails

