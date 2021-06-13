#!/bin/bash


set -e
set -u

OUTDIR=/tmp/ecen5013
#DIR is the path where writer executable, finder.sh and tester.sh are stored.
#Refer: https://www.electrictoolbox.com/bash-script-directory/
DIR="$( cd "$(dirname "$0")" ; pwd -P )"


if [ $# -lt 1 ]
then
	echo -e "\nUSING DEFAULT DIRECTORY ${OUTDIR} TO STORE FILES\n"
else
	OUTDIR=$1
	echo -e "\n USING ${OUTDIR} DIRECTORY TO STORE FILES\n"
fi


sudo cp "${DIR}/assignment-3-test-init.sh" "${OUTDIR}/rootfs/home"
sudo cp "${DIR}/assignment-1-test.sh" "${OUTDIR}/rootfs/home"
sudo cp "${DIR}/assignment-1-test-iteration.sh" "${OUTDIR}/rootfs/home"
sudo cp "${DIR}/script-helpers" "${OUTDIR}/rootfs/home"
sudo cp "${DIR}/assignment-timeout" "${OUTDIR}/rootfs/home"

#Device nodes required for test script
#See https://unix.stackexchange.com/a/327233
sudo mknod -m 0666 "${OUTDIR}/rootfs/dev/random" c 1 8
sudo mknod -m 0666 "${OUTDIR}/rootfs/dev/urandom" c 1 9

echo -e "\nCREATING STANDALONE INITRAMFS .CPIO FILE\n"
find "${OUTDIR}/rootfs" | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs_testing.cpio
pushd "${OUTDIR}"
#-f is written below in order to force it to overwrite if initramfs_testing.cpio already exists.
gzip -f initramfs_testing.cpio
popd
