How to remove all data in my microSD
```bash
sudo umount /media/hungle/boot
sudo umount /media/hungle/root
```
And
```bash
sudo dd if=/dev/zero of=/dev/mmcblk0 bs=1M count=100
sync
```

How to flash image to my microSD
```bash
cd ~/YOCTO/poky/build/tmp/deploy/images/raspberrypi5
bunzip2 core-image-base-raspberrypi5.wic.bz2
sudo dd if=core-image-base-raspberrypi5.wic of=/dev/mmcblk0 bs=4M status=progress conv=fsync
sync
```
Run minicom 
```bash
sudo minicom -D /dev/ttyUSB0 -b 115200
```
