#!/bin/bash

. base.sh

fails=0
testNo=0
logDir=$PWD/logging
logFile=$logDir/cgvalidate.log
inputDir=$PWD/input
buildDir=$PWD/../../../build/tools/
executable=$buildDir/cgvalidate

#cleanup
rm -rf $logDir
mkdir $logDir

# check if cgvalidate is available
if ! type $executable > /dev/null; then
  failed
  exit 1
else
  passed
  testNo=$(($testNo+1))
fi

#general
## wrong input files
$executable $inputDir/general/0001.cubex $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/general/0001.ipcg $inputDir/general/0001.ipcg >> $logFile
checkErrorCode 1

## missing edges
$executable $inputDir/general/0001_missedcallee.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/general/0001_missedparent.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
$executable $inputDir/general/0001_missedboth.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1

## successfull run
$executable $inputDir/general/0001.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 0

#virtual
$executable $inputDir/virtual/exp1.ipcg $inputDir/virtual/exp1.cubex >> $logFile
checkErrorCode 0
$executable $inputDir/virtual/exp2.ipcg $inputDir/virtual/exp2.cubex >> $logFile
checkErrorCode 0
$executable $inputDir/virtual/exp3.ipcg $inputDir/virtual/exp3.cubex >> $logFile
checkErrorCode 0

# finalize
exit $fails

