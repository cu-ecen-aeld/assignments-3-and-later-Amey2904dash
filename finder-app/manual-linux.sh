#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
# Updated by:  Amey Sharad Dashaputre
# Note: Changes were made in this file as per Assignment 3 - Part 2 requirements
#		


set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

#---Amey Modifications Starts---#
if [ ! -d ${OUTDIR} ]	#if directory is not created successfully, error
then
	echo "Error creating the Directory." 
	exit 1
fi
#---Amey Modifications Ends---#

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

#---Amey Modifications Starts---#

    # TODO: Add your kernel build steps here
    echo "Starting kernel build steps"
    echo "Deep Cleaning the build"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    echo "Building defconfig to configure virt arm dev board to simulate "
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    echo "Building a kernel image for booting with QEMU"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    
    echo "Building Modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    
    echo "Building the Device Tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
#---Amey Modifications Ends---#

fi 


echo "Adding the Image in outdir"
#---Amey Modifications Starts---#
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/
#---Amey Modifications Ends---#

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
#---Amey Modifications Starts---#
echo "Creating the necessary base directories"
mkdir -p ${OUTDIR}/rootfs

if ! [ -d "${OUTDIR}/rootfs" ]
then
	echo "Error!: ${OUTDIR}/rootfs could not be created"
	exit 1
fi

cd ${OUTDIR}/rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log
#---Amey Modifications Ends---#

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # TODO:  Configure busybox
#---Amey Modifications Starts---#
    echo "Configuring Busybox"
    make distclean
    make defconfig
#---Amey Modifications Ends---#
else
    cd busybox
fi

# TODO: Make and install busybox
#---Amey Modifications Starts---#
echo "Installing Busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Returning to rootfs"
cd ${OUTDIR}/rootfs
#---Amey Modifications Ends---#

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
#---Amey Modifications Starts---#
echo "Adding lib dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -f $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -f $SYSROOT/lib64/libm.so.6 lib64
cp -f $SYSROOT/lib64/libresolv.so.2 lib64
cp -f $SYSROOT/lib64/libc.so.6 lib64
#---Amey Modifications Ends---#

# TODO: Make device nodes
#---Amey Modifications Starts---#
echo "Creating device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
#---Amey Modifications Ends---#

# TODO: Clean and build the writer utility
#---Amey Modifications Starts---#
cd $FINDER_APP_DIR
make clean 
make CROSS_COMPILE=${CROSS_COMPILE}
#---Amey Modifications Ends---#

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

#---Amey Modifications Starts---#
cp -f $FINDER_APP_DIR/autorun-qemu.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/conf/ -r ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/finder.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/finder-test.sh ${OUTDIR}/rootfs/home
cp -f $FINDER_APP_DIR/writer ${OUTDIR}/rootfs/home
#---Amey Modifications Ends---#

# TODO: Chown the root directory
#---Amey Modifications Starts---#
echo "Chowning the rootfs directory"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
#---Amey Modifications Ends---#

# TODO: Create initramfs.cpio.gz
#---Amey Modifications Starts---#
echo "Creating initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio
#---Amey Modifications Ends---#

##EOF
