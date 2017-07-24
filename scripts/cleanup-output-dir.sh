#!/bin/bash

o=${1}
test -d ${o} || { echo 'missing dir'; exit -1 ; } ;
test -f ${o}/machine.json || { echo 'not a translator dir'; exit -1 ; } ;
test -f ${o}/bcs.zip && { echo 'already archived'; exit -1 ; } ;


inpath=$(realpath ${o})
cwd=$(pwd)
wd=$(mktemp -d)

echo Doing: ${inpath}

cd ${wd}
cp ${inpath}/machine.json .
cp ${inpath}/final.bc .
cp ${inpath}/*.bin .
cp ${inpath}/args .
shopt -s extglob
mkdir bcs
cd bcs
cp ${inpath}/funcs-+([0-9]).bc .
cp ${inpath}/funcs-indirect-+([0-9]).bc .
cd ..
zip -r bcs.zip bcs
rm -rf bcs
echo ${wd}

rm -rf ${inpath}
mv ${wd} ${inpath}
#rm -rf ${d}
#cp ${inpath}*
#rm -rf ${o}/already-explored-*.json
#rm -rf ${o}/arm-thumb-funcs-pcs-*.json
#rm -rf ${o}/arm-thumb-funcs-unmerged-*.json
#rm -rf ${o}/funcs-indirect*
#rm -rf ${o}/s2e-out-*
#rm -rf ${o}/s2e-last
#rm -rf ${o}/remaining-*.bc
#rm -rf ${o}/new-targets-*.json
#rm -rf ${o}/qemu-*.log
#rm -rf ${o}/is-thumb-*.json
#rm -rf ${o}/funcs-*.bc
#rm -rf ${o}/mem-ops.ll.bc
#rm -rf ${o}/final.bcno-opt.bc
#rm -rf ${o}/final.bcpost_passes.bc
#rm -rf ${o}/final-linked.bc
#rm -rf ${o}/final-linked.bc.fcnt
#rm -rf ${o}/final.ll
#rm -rf ${o}/translator.json

#0x0000_0000-0x0008_0000.bin
#final.bc
#final.bcno-opt.bc
#final.bcpost_passes.bc
#final-linked.bc
#final-linked.bc.fcnt
#final.ll
#machine.json
#mem-ops.ll.bc
#translator.json

#tar -cvf /tmp/o.tar \
#	${o}/machine.json \
#	${o}/final.bc \
#	${o}/*.bin \

