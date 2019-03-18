#!/usr/bin/env bash

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

    $bochs -f meta/bochsrc -q

else
    #expect bochs to be built and stored adjacent to saturn dir
    BXSHARE=../bochs/bios ../bochs/bochs -f meta/bochsrc -q
fi
