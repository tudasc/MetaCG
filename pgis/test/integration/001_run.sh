
buildDir=$1
binary=${buildDir}/tool/pgis_pira

echo -e "Static Selection\nRunning $binary"
$binary -o $PWD/out --static ./input/001.ipcg

