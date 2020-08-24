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
echo "Invocation: $executable $inputDir/general/0001.cubex $inputDir/general/0001.cubex" >> $logFile
$executable $inputDir/general/0001.cubex $inputDir/general/0001.cubex >> $logFile
checkErrorCode 2
echo "Invocation: $executable $inputDir/general/0001.ipcg $inputDir/general/0001.ipcg" >> $logFile
$executable $inputDir/general/0001.ipcg $inputDir/general/0001.ipcg >> $logFile
checkErrorCode 3

## missing edges
echo "Invocation: $executable $inputDir/general/0001_missedcallee.ipcg $inputDir/general/0001.cubex" >> $logFile
$executable $inputDir/general/0001_missedcallee.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
echo "Invocation: $executable $inputDir/general/0001_missedparent.ipcg $inputDir/general/0001.cubex" >> $logFile
$executable $inputDir/general/0001_missedparent.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
echo "Invocation: $executable $inputDir/general/0001_missedboth.ipcg $inputDir/general/0001.cubex" >> $logFile
$executable $inputDir/general/0001_missedboth.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1

## successfull run
echo "Invocation $executable $inputDir/general/0001.ipcg $inputDir/general/0001.cubex" >> $logFile
$executable $inputDir/general/0001.ipcg $inputDir/general/0001.cubex >> $logFile
checkErrorCode 0

#virtual
echo "Invocation: $executable $inputDir/virtual/exp1.ipcg $inputDir/virtual/exp1.cubex" >> $logFile
$executable $inputDir/virtual/exp1.ipcg $inputDir/virtual/exp1.cubex >> $logFile
checkErrorCode 0
echo "Invocation: $executable $inputDir/virtual/exp2.ipcg $inputDir/virtual/exp2.cubex" >> $logFile
$executable $inputDir/virtual/exp2.ipcg $inputDir/virtual/exp2.cubex >> $logFile
checkErrorCode 0
echo "Invocation: $executable $inputDir/virtual/exp3.ipcg $inputDir/virtual/exp3.cubex" >> $logFile
$executable $inputDir/virtual/exp3.ipcg $inputDir/virtual/exp3.cubex >> $logFile
checkErrorCode 0

# finalize
exit $fails

