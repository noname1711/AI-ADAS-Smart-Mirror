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
Check kĩ hơn 
```bash
ls tmp/work/raspberrypi5*/linux-raspberrypi*/ | grep 6.6
```
-> 6.6.63+git

-> Yocto chọn kernel 6.6.63 làm nguồn chính cho board RPi5
Thư mục source kernel thu được bằng lệnh 
```bash
find meta-raspberrypi -type f -name "linux-raspberrypi*.bb*"
```

Unpack để quan sát các file .c, .dts, .config của kernel
```bash
bitbake linux-raspberrypi -c cleanall
bitbake linux-raspberrypi -c unpack
```
Tuy vậy, lúc này check kernel source thì vẫn không thấy gì, khi này ta buộc phải clone lại kernel source
```bash
cd tmp/work-shared/raspberrypi5/
git clone --depth=1 -b rpi-6.6.y https://github.com/raspberrypi/linux kernel-source
```
Khi này quan sát trong kernel source thu được
```bash
cd kernel-source/
ls
```
những folder này
```bash
arch     CREDITS        fs        ipc      lib          mm         rust      sound
block    crypto         include   Kbuild   LICENSES     net        samples   tools
certs    Documentation  init      Kconfig  MAINTAINERS  README     scripts   usr
COPYING  drivers        io_uring  kernel   Makefile     README.md  security  virt
```
Kiểm tra driver cho IMX708 và UVC camera
```bash
ls drivers/media/i2c | grep imx
ls drivers/media/platform/bcm2835/ | grep unicam
ls drivers/media/usb | grep uvc
```
Thu được kết quả là driver của chúng lần lượt là:
```text
imx208.c
imx214.c
imx219.c
imx258.c
imx274.c
imx290.c
imx296.c
imx319.c
imx334.c
imx335.c
imx355.c
imx412.c
imx415.c
imx477.c
imx500.c
imx519.c
imx708.c
```
```text
bcm2835-unicam.c
vc4-regs-unicam.h
```
```text
uvc
```
Kiểm tra xem kernel config đã bật các driver đó chưa
```bash
grep -i imx708 arch/arm64/configs/bcm2712_defconfig
grep -i unicam arch/arm64/configs/bcm2712_defconfig
grep -i uvc arch/arm64/configs/bcm2712_defconfig
```
Lúc này kết quả thu được
```bash
CONFIG_VIDEO_IMX708=m
CONFIG_VIDEO_BCM2835_UNICAM=m
CONFIG_USB_CONFIGFS_F_UVC=y
```
Kết quả đã ổn, bây giờ, nhiệm vụ là cần check xem device tree đã có support chưa
```bash
ls arch/arm/boot/dts/overlays/
```
Quan sát thấy có overlay có sẵn
```bash
arch/arm/boot/dts/overlays/imx708-overlay.dts
arch/arm/boot/dts/overlays/imx708.dtsi
```
Còn OV3660 USB OVC camera là loại camera USB không cần overlay riêng. Tiến hành kiểm tra xem device tree có support cho cam chưa
```bash
grep -A 30 -B 5 imx708 arch/arm/boot/dts/overlays/imx708-overlay.dts
```
Thu được kết quả rất khả quan 
```dts
__overlay__ {
			#address-cells = <1>;
			#size-cells = <0>;
			status = "okay";

			#include "imx708.dtsi"
		};
	};

	csi_frag: fragment@101 {
		target = <&csi1>;
		csi: __overlay__ {
			status = "okay";
			brcm,media-controller;

			port {
				csi_ep: endpoint {
					remote-endpoint = <&cam_endpoint>;
					clock-lanes = <0>;
					data-lanes = <1 2>;
					clock-noncontinuous;
				};
			};
		};
	};

	__overrides__ {
		rotation = <&cam_node>,"rotation:0";
		orientation = <&cam_node>,"orientation:0";
		media-controller = <&csi>,"brcm,media-controller?";
		cam0 = <&i2c_frag>, "target:0=",<&i2c_csi_dsi0>,
		       <&csi_frag>, "target:0=",<&csi0>,
		       <&clk_frag>, "target:0=",<&cam0_clk>,
		       <&reg_frag>, "target:0=",<&cam0_reg>,
		       <&cam_node>, "clocks:0=",<&cam0_clk>,
		       <&cam_node>, "vana1-supply:0=",<&cam0_reg>,
```
Điều này nghĩa là trong config.txt thêm dòng
```bash
dtoverlay=imx708,cam1
```
là Pi sẽ enable full pipeline: I²C + CSI + driver + Unicam. Cẩn thận hơn có thể check sensor node IMX708 cụ thể
```bash
grep -A 40 imx708 arch/arm/boot/dts/overlays/imx708.dtsi
```
Và kết quả
```bash
cam_node: imx708@1a {
	compatible = "sony,imx708";
	reg = <0x1a>;
	status = "disabled";

	clocks = <&cam1_clk>;
	clock-names = "inclk";

	vana1-supply = <&cam1_reg>;	/* 2.8v */
	vana2-supply = <&cam_dummy_reg>;/* 1.8v */
	vdig-supply = <&cam_dummy_reg>;	/* 1.1v */
	vddl-supply = <&cam_dummy_reg>;	/* 1.8v */

	rotation = <180>;
	orientation = <2>;

	port {
		cam_endpoint: endpoint {
			clock-lanes = <0>;
			data-lanes = <1 2>;
			clock-noncontinuous;
			link-frequencies =
				/bits/ 64 <450000000>;
		};
	};
};

vcm_node: dw9817@c {
	compatible = "dongwoon,dw9817-vcm";
	reg = <0x0c>;
	status = "disabled";
	VDD-supply = <&cam1_reg>;
};
```
Tiến hành build sato 
```bash
bitbake core-image-sato
```
Check xem kernel Yocto đã thực sự build thành công 2 module cần cho full camera pipeline chưa
```bash
find tmp/work/raspberrypi5*/linux-raspberrypi*/ -type f -name "*imx708*.ko"
find tmp/work/raspberrypi5*/linux-raspberrypi*/ -type f -name "*unicam*.ko"
```
thu được 
```bash
tmp/work/raspberrypi5-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi5-standard-build/drivers/media/i2c/imx708.ko
tmp/work/raspberrypi5-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi5-standard-build/drivers/media/platform/bcm2835/bcm2835-unicam.ko
```
Đồng nghĩa với việc kernel Yocto đã build được thành công
Trên RPI:
```bash
root@raspberrypi5:~# uname -r                                                   
6.6.63-v8-16k
```
Cop file .ko 
```bash
sudo mkdir -p /media/hungle/root/lib/modules/6.6.63-v8-16k/kernel/drivers/media/i2c
sudo mkdir -p /media/hungle/root/lib/modules/6.6.63-v8-16k/kernel/drivers/media/platform/bcm2835
```
Trong build Yocto
```bash
sudo cp \
tmp/work/raspberrypi5-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi5-standard-build/drivers/media/i2c/imx708.ko \
/media/hungle/root/lib/modules/6.6.63-v8-16k/kernel/drivers/media/i2c/

sudo cp \
tmp/work/raspberrypi5-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi5-standard-build/drivers/media/platform/bcm2835/bcm2835-unicam.ko \
/media/hungle/root/lib/modules/6.6.63-v8-16k/kernel/drivers/media/platform/bcm2835/
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

I2C
```bash
echo imx708 0x1a | sudo tee /sys/bus/i2c/devices/i2c-13/new_device
```

Nếu bị lỗi khi flash image thành root1 
```bash
lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
loop0         7:0    0   1.3G  1 loop /snap/android-studio/191
loop1         7:1    0  63.8M  1 loop /snap/core20/2599
loop2         7:2    0 164.8M  1 loop /snap/gnome-3-28-1804/198
loop3         7:3    0   1.2G  1 loop /snap/intellij-idea-community/661
loop4         7:4    0    41M  1 loop /snap/node/10653
loop5         7:5    0 905.9M  1 loop /snap/pycharm-community/480
mmcblk0     179:0    0  59.5G  0 disk 
├─mmcblk0p1 179:1    0   130M  0 part /media/hungle/boot
└─mmcblk0p2 179:2    0   1.5G  0 part /media/hungle/root1
nvme0n1     259:0    0 931.5G  0 disk 
├─nvme0n1p1 259:1    0  70.9G  0 part /boot/efi
├─nvme0n1p2 259:2    0    16M  0 part 
├─nvme0n1p3 259:3    0 280.5G  0 part 
├─nvme0n1p4 259:4    0  46.6G  0 part /
└─nvme0n1p5 259:5    0 533.6G  0 part /home
```
Thì sửa như sau:
```bash
sudo umount /dev/mmcblk0p1 /dev/mmcblk0p2
sudo rm -rf /media/hungle/boot /media/hungle/root /media/hungle/root1
```
Check lại thành 
```bash
sudo blkid /dev/mmcblk0p2
```
Thành thế này là được 
```bash
/dev/mmcblk0p2: LABEL="root" UUID="92723ebe-78ae-4ce5-9f65-de475212e17a" BLOCK_SIZE="4096" TYPE="ext4" PARTUUID="076c4a2a-02"
```
Và 
```bash
lsblk
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
loop0         7:0    0   1.3G  1 loop /snap/android-studio/191
loop1         7:1    0  63.8M  1 loop /snap/core20/2599
loop2         7:2    0 164.8M  1 loop /snap/gnome-3-28-1804/198
loop3         7:3    0   1.2G  1 loop /snap/intellij-idea-community/661
loop4         7:4    0    41M  1 loop /snap/node/10653
loop5         7:5    0 905.9M  1 loop /snap/pycharm-community/480
mmcblk0     179:0    0  59.5G  0 disk 
├─mmcblk0p1 179:1    0   130M  0 part /media/hungle/boot
└─mmcblk0p2 179:2    0   1.5G  0 part /media/hungle/root
nvme0n1     259:0    0 931.5G  0 disk 
├─nvme0n1p1 259:1    0  70.9G  0 part /boot/efi
├─nvme0n1p2 259:2    0    16M  0 part 
├─nvme0n1p3 259:3    0 280.5G  0 part 
├─nvme0n1p4 259:4    0  46.6G  0 part /
└─nvme0n1p5 259:5    0 533.6G  0 part /home
```
Sử dụng dtc để chuyển dtbo ra dts mà check node 
```bash
/media/hungle/boot/overlays$ dtc -I dtb -O dts -o imx708_rev.dts imx708.dtbo
```
dts ngược ra dtbo
```bash
dtc -@ -I dts -O dtb -o imx708.dtbo imx708.dts
```
