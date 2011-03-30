qemu -gdb tcp:localhost:10000 -device ers_panel8 -device ers_punch_press -icount 9 -m 128 -boot a -fda boot.img -hda fat:./hda --no-reboot
