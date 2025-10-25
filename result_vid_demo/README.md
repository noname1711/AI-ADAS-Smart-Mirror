Quy trình tích hợp device 
1. Xác định giao thức sử dụng (RPI camera module 3 - I2C-controlled, OV3660 Camera Module - USB)
2. driver tương ứng với camera? 
3. device tree có support cho cam chưa? 
4. build và thêm device tree thì làm sao để check xem driver đã thêm thành công vào kernel và đã chạy?  
5. Nếu đã chạy được thì trên userspace đâu là file đại diện <Everything in Linux is File>

```text
Build Configuration:
BB_VERSION           = "2.8.1"
BUILD_SYS            = "x86_64-linux"
NATIVELSBSTRING      = "universal"
TARGET_SYS           = "aarch64-poky-linux"
MACHINE              = "raspberrypi5"
DISTRO               = "poky"
DISTRO_VERSION       = "5.0.12"
TUNE_FEATURES        = "aarch64 crypto cortexa76"
TARGET_FPU           = ""
meta                 
meta-poky            
meta-yocto-bsp       = "scarthgap:fbc7beca684d81410f5ad5c146b87e4cb080c98a"
meta-raspberrypi     = "scarthgap:aaf976a665daa7e520545908adef8a0e9410b57f"
meta-qt6             = "6.9:9d7693a4e0cdb1592e8e479e7c8a45e264274be3"
meta-oe              
meta-python          
meta-multimedia      = "scarthgap:e621da947048842109db1b4fd3917a02e0501aa2"
meta-smartmirror     = "scarthgap:fbc7beca684d81410f5ad5c146b87e4cb080c98a"
```

Check kernel version
```bash
bitbake -e virtual/kernel | grep ^PREFERRED_VERSION
```
Thu được 
```text
PREFERRED_VERSION_linux-raspberrypi="6.6.%"
```
-> Yocto chọn kernel linux-raspberrypi_6.6.bb làm nguồn chính cho board RPi5
Thư mục source kernel thu được bằng lệnh 
```bash
find meta-raspberrypi -type f -name "linux-raspberrypi*.bb*"
```

Unpack để quan sát các file .c, .dts, .config của kernel
```bash
bitbake linux-raspberrypi -c cleanall
bitbake linux-raspberrypi -c unpack
```

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
