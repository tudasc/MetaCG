# s.beta30.pofi.r2
echo "Usage: source-file param-name [dflt: X] seq-start [dflt: 2]  seq-inc [dflt: 2]  seq-stop [dflt: 10]"

sourceFileArg=$1
paramNameArg=$2
seqStartArg=$3
seqIncArg=$4
seqStopArg=$5

if [[ -z "${sourceFileArg}" ]]; then
  echo "need to provide source file"
  exit 1
fi

paramName="${paramNameArg:-X}"
seqStart="${seqStartArg:-2}"
seqInc="${seqIncArg:-2}"
seqStop="${seqStopArg:-10}"

for s in $(seq ${seqStart} ${seqInc} ${seqStop}); do
  echo ${s}
  scorep gcc -D${paramName}=${s} ${sourceFileArg}
  #scorep gcc -D${paramName}=${s} ${sourceFileArg} -o a.${paramName}${s}.out ;
  for i in $(seq 1 5); do
    ./a.out
    mkdir t.${paramName}${s}.postfix.r${i}
    mv scorep-*/profile.cubex ./t.${paramName}${s}.postfix.r${i}/profile.cubex
    rm -r scorep-*
  done
  mv ./a.out ./a.${paramName}${s}.out
done
