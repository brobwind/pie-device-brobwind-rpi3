#! /bin/bash
#set -e -x

cat << EOF 2>&1 > /dev/null
Disk iot_rpi3.img: 16777216 sectors, 8.0 GiB
Logical sector size: 512 bytes
Disk identifier (GUID): C06D97C6-B03D-4504-B474-73ED60F71CC3
Partition table holds up to 128 entries
First usable sector is 34, last usable sector is 16777182
Partitions will be aligned on 8-sector boundaries
Total free space is 13 sectors (6.5 KiB)

Number  Start (sector)    End (sector)  Size       Code  Name
   1              40          131111   64.0 MiB    FFFF  rpiboot
   2          131112          133159   1024.0 KiB  FFFF  uboot_a
   3          133160          135207   1024.0 KiB  FFFF  uboot_b
   4          135208          200743   32.0 MiB    FFFF  boot_a
   5          200744          266279   32.0 MiB    FFFF  boot_b
   6          266280         1314855   512.0 MiB   FFFF  system_a
   7         1314856         2363431   512.0 MiB   FFFF  system_b
   8         2363432         2363559   64.0 KiB    FFFF  vbmeta_a
   9         2363560         2363687   64.0 KiB    FFFF  vbmeta_b
  10         2363688         2365735   1024.0 KiB  FFFF  misc
  11         2365736         2496807   64.0 MiB    FFFF  vendor_a
  12         2496808         2627879   64.0 MiB    FFFF  vendor_b
  13         2627880         2636071   4.0 MiB     FFFF  oem_bootloader_a
  14         2636072         2644263   4.0 MiB     FFFF  oem_bootloader_b
  15         3791144         4839719   4.0 MiB     FFFF  frp
  16         4839720        16777175   5.7 GiB     FFFF  userdata
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A rpiboot
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A uboot_a
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A uboot_b
PART: BB499290-B57E-49F6-BF41-190386693794 boot_a
PART: BB499290-B57E-49F6-BF41-190386693794 boot_b
PART: 0F2778C4-5CC1-4300-8670-6C88B7E57ED6 system_a
PART: 0F2778C4-5CC1-4300-8670-6C88B7E57ED6 system_b
PART: B598858A-5FE3-418E-B8C4-824B41F4ADFC vbmeta_a
PART: B598858A-5FE3-418E-B8C4-824B41F4ADFC vbmeta_b
PART: 6B2378B0-0FBC-4AA9-A4F6-4D6E17281C47 misc
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A vendor_a
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A vendor_b
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 oem_bootloader_a
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 oem_bootloader_b
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 frp
PART: 0BB7E6ED-4424-49C0-9372-7FBAB465AB4C userdata
EOF

# -----------------------------------------------------------------------------------
SGDISK=sgdisk
GDISK=gdisk
DD=dd
DN=`dirname $0`

if [ "${1}x" = "x" ] ; then
	echo "Please specify target file name"
	exit 1
fi

target=${1}

# -----------------------------------------------------------------------------------
SIZE_RPIBOOT=64M
SIZE_UBOOT=1024K
SIZE_BOOT=32M
SIZE_SYSTEM=650M
SIZE_VBMETA=64K
SIZE_MISC=1024K
SIZE_VENDOR=256M
SIZE_OEM_BOOTLOADER=4M
SIZE_FRP=2M
SIZE_SWAP=384M
SIZE_USERDATA=0

# -----------------------------------------------------------------------------------
SECTOR_SIZE=512
SECTOR_START=40
PART_INDEX=1
SG_PARAM="--set-alignment=1"

SG_PARAM="${SG_PARAM} $(cat << EOF | while read line
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A rpiboot ${SIZE_RPIBOOT}
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A uboot_a ${SIZE_UBOOT}
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A uboot_b ${SIZE_UBOOT}
PART: BB499290-B57E-49F6-BF41-190386693794 boot_a ${SIZE_BOOT}
PART: BB499290-B57E-49F6-BF41-190386693794 boot_b ${SIZE_BOOT}
PART: 0F2778C4-5CC1-4300-8670-6C88B7E57ED6 system_a ${SIZE_SYSTEM}
PART: 0F2778C4-5CC1-4300-8670-6C88B7E57ED6 system_b ${SIZE_SYSTEM}
PART: B598858A-5FE3-418E-B8C4-824B41F4ADFC vbmeta_a ${SIZE_VBMETA}
PART: B598858A-5FE3-418E-B8C4-824B41F4ADFC vbmeta_b ${SIZE_VBMETA}
PART: 6B2378B0-0FBC-4AA9-A4F6-4D6E17281C47 misc ${SIZE_MISC}
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A vendor_a ${SIZE_VENDOR}
PART: 314F99D5-B2BF-4883-8D03-E2F2CE507D6A vendor_b ${SIZE_VENDOR}
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 oem_bootloader_a ${SIZE_OEM_BOOTLOADER}
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 oem_bootloader_b ${SIZE_OEM_BOOTLOADER}
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 frp ${SIZE_FRP}
PART: AA3434B2-DDC3-4065-8B1A-18E99EA15CB7 swap ${SIZE_SWAP}
PART: 0BB7E6ED-4424-49C0-9372-7FBAB465AB4C userdata ${SIZE_USERDATA}
EOF
do
	ARRAY=(${line})
	uuid=${ARRAY[1]}
	label=${ARRAY[2]}
	size=${ARRAY[3]}

	if [ "${size}x" = "x" ] ; then
		echo "ERROR size @ ${line}"
		exit 1
	elif [[ ${size} =~ [0-9]+G$ ]] ; then
		real_size=$((`echo ${size} | sed 's/G$//'` * 1024 * 1024 * 1024))
	elif [[ ${size} =~ [0-9]+M$ ]] ; then
		real_size=$((`echo ${size} | sed 's/M$//'` * 1024 * 1024))
	elif [[ ${size} =~ [0-9]+K$ ]] ; then
		real_size=$((`echo ${size} | sed 's/K$//'` * 1024))
	elif [[ ${size} =~ [0-9]+$ ]] ; then
		real_size=$((${size}))
	else
		echo "ERROR @ ${line}"
		exit 2
	fi

	sector_end=$((${SECTOR_START} + ${real_size} / ${SECTOR_SIZE}))
	if [ ${real_size} -eq 0 ] ; then
		sector_end=1
	fi

	echo " --new=${PART_INDEX}:${SECTOR_START}:$((${sector_end} - 1))"
	echo " --typecode=${PART_INDEX}:${uuid}"
	echo " --change-name=${PART_INDEX}:${label}"

	PART_INDEX=$((${PART_INDEX} + 1))
	SECTOR_START=${sector_end}
done)"

echo " => Destroy partition table ..."

${SGDISK} --zap-all ${target}

echo " => Install GPT partition table ..."

${SGDISK} ${SG_PARAM} ${target} 2>&1 > /dev/null

echo " => Install hybrid MBR partition table ..."

${GDISK} << EOF ${target} 2>&1 > /dev/null
r
h
1
N
06
N
N
w
Y
EOF

# --------------------------------------------------------------------------------------------------

echo " => Install images ...."

DINFO="`${SGDISK} --print ${target}`"

echo "${DINFO}" | tail -n +10 | while read line
do
	ARRAY=(${line})
	start=${ARRAY[1]}
	end=${ARRAY[2]}
	label=${ARRAY[6]}
	file=${OUT}/`echo ${label} | sed 's/_a//g'`.img

	if [ ! -f ${file} ] ; then
		file=${DN}/images/${label}.img
	fi

	# Erase userdata partition when system bootup
	if [ "${label}x" = "userdatax" ] ; then
		file=${DN}/images/zero_4k.bin
	fi

	if [ -f ${file} ] ; then
		echo "     => Install: ${label}(${file}) image ..."
		sector_count=$((${end} - ${start} + 1))
		part_size=$((${sector_count} * ${SECTOR_SIZE}))
		file_size=`stat -c %s ${file}`
		if [ ${file_size} -gt ${part_size} ] ; then
			echo "Can not install image: ${label}, file size: ${file_size}, partition size: ${part_size}"
			continue
		fi
		${DD} if=${file} of=${target} conv=notrunc bs=${SECTOR_SIZE} count=$((${end} - ${start} + 1)) seek=${start}
	fi
done

# --------------------------------------------------------------------------------------------------

echo -e " => Dump partition table ...."

echo "${DINFO}"

echo "${DINFO}" | tail -n +10 | while read line
do
	ARRAY=(${line})
	num=${ARRAY[0]}
	start=${ARRAY[1]}
	end=${ARRAY[2]}
	label=${ARRAY[6]}
	guid=`${SGDISK} --info ${num} ${target} | head -n 1 | sed 's/.*: \([0-9A-Z-]\+\).*/\1/g'`
	uuid=`${SGDISK} --info ${num} ${target} | head -n 2 | tail -n 1 | sed 's/.*: \([0-9A-Z-]\+\).*/\1/g'`
	echo "PART: ${guid} ${uuid} ${label}"
done
