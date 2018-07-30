#!/usr/bin/env bash

vbox="VBoxManage"
kernel=$(uname -r)

if [[ $kernel = *"Microsoft"* ]]; 
then
    vbox="$vbox.exe"
fi

vms=$($vbox list vms)

declare -a vms
mapfile -t vms < <($vbox list vms)

foundVM=false

for vm in "${vms[@]}"; do

    if [[ $vm == \"Saturn\"* ]]; 
    then
        foundVM=true
    fi

done

if [ "$foundVM" = false ];
then
    echo "Creating new Saturn VM for VirtualBox"
    $vbox createvm --name Saturn --ostype "Other_64" --register
    $vbox modifyvm Saturn --memory 256 --cpus 2 
    $vbox storagectl Saturn --name "IDE" --add ide --bootable on
    $vbox storageattach Saturn --storagectl "IDE" --medium sysroot/system/boot/saturn.iso --type dvddrive --port 0 --device 0
fi

$vbox startvm Saturn