REM "c:\Program Files\qemu\qemu-system-x86_64.exe"
"c:\Program Files\qemu\qemu-system-i386.exe"^
    -cpu max^
    -cdrom sysroot/system/boot/saturn.iso^
    -s -no-reboot -no-shutdown^
    -m 512M^
    -serial file:saturn.log^
    -smp 2
