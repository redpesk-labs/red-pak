#!/bin/bash

cd $(dirname $0)

#set -x

redroot=/tmp/rpak-jointest
rm -rf $redroot
mkdir -p $redroot/main/join

cc -static -o ${redroot}/prog prog.c

sed s:SHARING:Disabled: ./conf-normal > ${redroot}/main-normal
redconf create --alias main --config ${redroot}/main-normal --config-adm ./conf-admin ${redroot}/main

redwrap --redpath ${redroot}/main -- /prog > ${redroot}/output-main &
sleep 1
read ready pidm < ${redroot}/output-main
nsmain=/proc/$pidm/ns/net
vmain=$(readlink $nsmain)

sed "s,SHARING,$nsmain," ./conf-normal > ${redroot}/join-normal
redconf create --alias join --config ${redroot}/join-normal --config-adm ./conf-admin ${redroot}/join

redwrap --redpath ${redroot}/join -- /prog > ${redroot}/output-join &
sleep 1
read ready pidj < ${redroot}/output-join
nsjoin=/proc/$pidj/ns/net
vjoin=$(readlink $nsjoin)

kill $pidj
kill $pidm

echo "TAP version 14"
if [[ "$vjoin" = "$vmain" ]]
then
   echo "ok 1 - join"
else
   echo "not ok 1 - join"
   echo "   vmain = $vmain"
   echo "   vjoin = $vjoin"
fi
echo "1..1"
echo

