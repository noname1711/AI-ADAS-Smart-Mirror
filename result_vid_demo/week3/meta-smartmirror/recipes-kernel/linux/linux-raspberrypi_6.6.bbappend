FILESEXTRAPATHS:prepend := "${THISDIR}/linux-raspberrypi/features/camera-enable:"

SRC_URI += " \
    file://camera-enable.scc \
    file://camera-enable.cfg \
    file://overlays/imx708.dtbo \
"

KERNEL_FEATURES:append = " camera-enable.scc"

# Ensure imx708 overlay is installed in /boot/overlays
KERNEL_DEVICETREE:append = " overlays/imx708.dtbo"

do_deploy:append() {
    install -d ${DEPLOYDIR}/overlays
    install -m 0644 ${WORKDIR}/overlays/imx708.dtbo ${DEPLOYDIR}/overlays/
}
