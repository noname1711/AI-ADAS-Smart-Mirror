SUMMARY = "Touchscreen Dual Camera Viewer"
DESCRIPTION = "Display one camera at a time with touch-to-switch using OpenCV"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://camera-manager.cpp \
    file://camera-manager.service \
    file://camera-manager.desktop \
"

S = "${WORKDIR}"

DEPENDS = "opencv"

do_compile() {
    ${CXX} ${CXXFLAGS} -I${STAGING_INCDIR}/opencv4 -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs camera-manager.cpp -o camera-manager ${LDFLAGS}
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 camera-manager ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 camera-manager.service ${D}${systemd_system_unitdir}/
    
    install -d ${D}${sysconfdir}/xdg/autostart
    install -m 0644 camera-manager.desktop ${D}${sysconfdir}/xdg/autostart/
}

FILES:${PN} += " \
    ${bindir}/camera-manager \
    ${systemd_system_unitdir}/camera-manager.service \
    ${sysconfdir}/xdg/autostart/camera-manager.desktop \
"

SYSTEMD_SERVICE:${PN} = "camera-manager.service"