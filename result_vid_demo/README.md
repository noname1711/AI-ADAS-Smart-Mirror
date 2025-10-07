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
sudo dd if=tmp/deploy/images/raspberrypi5/core-image-base-raspberrypi5.wic.bz2 of=/dev/sdX bs=4M status=progress && sync
sync
```
