RPi3 B+ Firmware
================

This directory contains some firmware for the RPi3 to run the kernel image

To run on hardware simply copy the config.txt, cmdline.txt, armstub8.bin and bcm2710-rpi-3-b-plus.dtb files from prebuilt/ onto the raspi3 sdcard
along with kernel8.img

cmdline.txt contains the text that will be added to the end of the chosen/bootargs parameter in the dtb
(which usually has some extra stuff added by the firmware)

The armstub8.bin and bcm2710-rpi-3-b-plus.dtb files in prebuilt/ come from https://github.com/raspberrypi/firmware
You can find custom ones at https://github.com/rems-project/system-litmus-harness/tree/master/firmware/bcm2837 if you want