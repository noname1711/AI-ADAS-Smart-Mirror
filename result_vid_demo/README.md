Quy trình tích hợp device 
1. Xác định giao thức sử dụng 
2. driver tương ứng với camera? 
3. device tree có support cho cam chưa? 
4. build và thêm device tree thì làm sao để check xem driver đã thêm thành công vào kernel và đã chạy?  
5. Nếu đã chạy được thì trên userspace đâu là file đại diện <Everything in Linux is File>


How to remove all data in my microSD
```bash
sudo umount /dev/mmcblk0p1 2>/dev/null
sudo umount /dev/mmcblk0p2 2>/dev/null
sudo dd if=/dev/zero of=/dev/mmcblk0 bs=1M count=50 status=progress && sync
```

How to flash image to my microSD
```bash
sudo dd if=~/YOCTO/poky/build/tmp/deploy/images/raspberrypi5/core-image-sato-raspberrypi5.rootfs.wic \
of=/dev/mmcblk0 bs=4M status=progress conv=fsync
sync
```
Run minicom 
```bash
sudo minicom -D /dev/ttyUSB0 -b 115200
```
