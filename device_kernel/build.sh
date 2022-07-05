#ARCH=arm make udoo_quad_dual_defconfig
#ARCH=arm make menuconfig
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make zImage -j4
