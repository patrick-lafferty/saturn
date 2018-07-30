#!/usr/bin/env bash

bochs="bochs"
kernel=$(uname -r)

if [[ $kernel = *"Microsoft"* ]]; 
then
    bochs="$bochs.exe"
    where=$(which $bochs)
    bochsDir=$(dirname $(which $bochs))
    bochsDirName=$(basename $bochsDir)

    if [[ $bochsDirName = *"bochs"* ]];
    then
        biosPath=$bochsDir/bios
    else
        biosPath=$(dirname $bochsDir)/bios
    fi

    BXSHARE=$biosPath
    export BXSHARE
    export WSLENV=$WSLENV:BXSHARE/p 
fi

$bochs -f meta/bochsrc -q