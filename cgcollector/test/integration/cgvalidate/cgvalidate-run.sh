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
echo "Invocation: $executable -i $inputDir/general/0001.cubex -c $inputDir/general/0001.cubex" >> $logFile
$executable -i $inputDir/general/0001.cubex -c $inputDir/general/0001.cubex >> $logFile
checkErrorCode 2
echo "Invocation: $executable -i $inputDir/general/0001.ipcg -c $inputDir/general/0001.ipcg" >> $logFile
$executable -i $inputDir/general/0001.ipcg -c $inputDir/general/0001.ipcg >> $logFile
checkErrorCode 3

## missing edges
echo "Invocation: $executable -i $inputDir/general/0001_missedcallee.ipcg -c $inputDir/general/0001.cubex" >> $logFile
$executable -i $inputDir/general/0001_missedcallee.ipcg -c $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
echo "Invocation: $executable -i $inputDir/general/0001_missedparent.ipcg -c $inputDir/general/0001.cubex" >> $logFile
$executable -i $inputDir/general/0001_missedparent.ipcg -c $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1
echo "Invocation: $executable -i $inputDir/general/0001_missedboth.ipcg -c $inputDir/general/0001.cubex" >> $logFile
$executable -i $inputDir/general/0001_missedboth.ipcg -c $inputDir/general/0001.cubex >> $logFile
checkErrorCode 1

## successfull run
echo "Invocation $executable -i $inputDir/general/0001.ipcg -c $inputDir/general/0001.cubex" >> $logFile
$executable -i $inputDir/general/0001.ipcg -c $inputDir/general/0001.cubex >> $logFile
checkErrorCode 0

#virtual
echo "Invocation: $executable -i $inputDir/virtual/exp1.ipcg -c $inputDir/virtual/exp1.cubex" >> $logFile
$executable -i $inputDir/virtual/exp1.ipcg -c $inputDir/virtual/exp1.cubex >> $logFile
checkErrorCode 0
echo "Invocation: $executable -i $inputDir/virtual/exp2.ipcg -c $inputDir/virtual/exp2.cubex" >> $logFile
$executable -i $inputDir/virtual/exp2.ipcg -c $inputDir/virtual/exp2.cubex >> $logFile
checkErrorCode 0
echo "Invocation: $executable -i $inputDir/virtual/exp3.ipcg -c $inputDir/virtual/exp3.cubex" >> $logFile
$executable -i $inputDir/virtual/exp3.ipcg -c $inputDir/virtual/exp3.cubex >> $logFile
checkErrorCode 0

#fix
echo "Invocation: $executable -i $inputDir/fix/miss.ipcg -c $inputDir/fix/0001.cubex -f" >> $logFile
$executable -i $inputDir/fix/miss.ipcg -c $inputDir/fix/0001.cubex -f >> $logFile
$executable -i $inputDir/fix/miss.ipcg.fix -c $inputDir/fix/0001.cubex >> $logFile
checkErrorCode 0
echo "Invocation: $executable -i $inputDir/fixvirtual/miss.ipcg -c $inputDir/fixvirtual/exp1.cubex -f" >> $logFile
$executable -i $inputDir/fixvirtual/miss.ipcg -c $inputDir/fixvirtual/exp1.cubex -f >> $logFile
$executable -i $inputDir/fixvirtual/miss.ipcg.fix -c $inputDir/fixvirtual/exp1.cubex >> $logFile
checkErrorCode 0


# finalize
exit $fails

