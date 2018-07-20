"c:\Program Files\qemu\qemu-system-x86_64.exe"^
    -cpu max^
    -kernel sysroot/system/boot/saturn.bin^
    -s -no-reboot -no-shutdown^
    -m 512M^
    -hda "C:\Users\pat\VirtualBox VMs\saturn\saturn_test.vdi"^
    -serial file:saturn.log^
    -smp 2
