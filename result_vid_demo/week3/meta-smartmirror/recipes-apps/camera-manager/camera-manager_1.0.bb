SUMMARY = "Touchscreen Dual Camera Viewer"
DESCRIPTION = "Display one camera at a time with touch-to-switch using OpenCV"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://camera-manager.cpp \
    file://CMakeLists.txt \
    file://camera-manager.service \
"

S = "${WORKDIR}"

inherit cmake systemd pkgconfig

DEPENDS += "opencv"

EXTRA_OECMAKE += " \
    -DOpenCV_DIR=${STAGING_DIR_TARGET}/usr/lib/cmake/opencv4 \
    -DCMAKE_PREFIX_PATH=${STAGING_DIR_TARGET}/usr/lib/cmake/opencv4 \
"

SYSTEMD_SERVICE:${PN} = "camera-manager.service"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/camera-manager ${D}${bindir}/camera-manager

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/camera-manager.service ${D}${systemd_system_unitdir}/camera-manager.service
}

FILES:${PN} += "${systemd_system_unitdir}/camera-manager.service"
