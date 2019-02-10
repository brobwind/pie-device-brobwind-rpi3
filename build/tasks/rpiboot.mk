
ifeq ($(TARGET_PRODUCT),rpi3)

RPIBOOT_OUT_ROOT := $(PRODUCT_OUT)/rpiboot
RPIBOOT_IMAGE    := $(PRODUCT_OUT)/rpiboot.img

RPIBOOT_COPY_FILES := \
    device/brobwind/rpi3/boot/rpiboot/bootcode.bin:bootcode.bin \
    device/brobwind/rpi3/boot/rpiboot/fixup_cd.dat:fixup_cd.dat \
    device/brobwind/rpi3/boot/rpiboot/fixup.dat:fixup.dat \
    device/brobwind/rpi3/boot/rpiboot/fixup_db.dat:fixup_db.dat \
    device/brobwind/rpi3/boot/rpiboot/fixup_x.dat:fixup_x.dat \
    device/brobwind/rpi3/boot/rpiboot/start_cd.elf:start_cd.elf \
    device/brobwind/rpi3/boot/rpiboot/start_db.elf:start_db.elf \
    device/brobwind/rpi3/boot/rpiboot/start.elf:start.elf \
    device/brobwind/rpi3/boot/rpiboot/start_x.elf:start_x.elf \
    device/brobwind/rpi3/boot/rpiboot/issue.txt:issue.txt \
    device/brobwind/rpi3/boot/rpiboot/LICENCE.broadcom:LICENCE.broadcom \
    device/brobwind/rpi3/boot/rpiboot/LICENSE.oracle:LICENSE.oracle \
    device/brobwind/rpi3/boot/rpiboot/SHA1SUM:SHA1SUM \

RPIBOOT_COPY_FILES += \
    device/brobwind/rpi3/boot/rpiboot/cmdline.txt:cmdline.txt \
    device/brobwind/rpi3/boot/rpiboot/config.txt:config.txt \
    device/brobwind/rpi3/boot/rpiboot/u-boot-dtok.bin:u-boot-dtok.bin \
    device/brobwind/rpi3/boot/rpiboot/uboot.env:uboot.env \

RPIBOOT_COPY_FILES += \
    device/brobwind/rpi3/boot/rpiboot/overlays/chosen-serial0.dtbo:overlays/chosen-serial0.dtbo \
    device/brobwind/rpi3/boot/rpiboot/overlays/rpi-uart-skip-init.dtbo:overlays/rpi-uart-skip-init.dtbo

RPIBOOT_COPY_FILES += \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/broadcom/bcm2710-rpi-3-b.dtb:bcm2710-rpi-3-b.dtb \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/broadcom/bcm2710-rpi-3-b-plus.dtb:bcm2710-rpi-3-b-plus.dtb \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/overlays/bcm2710-rpi-3-b-android-fstab.dtbo:overlays/bcm2710-rpi-3-b-android-fstab.dtbo \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/overlays/pwm-2chan.dtbo:overlays/pwm-2chan.dtbo \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/overlays/sdtweak.dtbo:overlays/sdtweak.dtbo \
    device/brobwind/rpi3/boot/kernel-v4.14/dtbs/4.14.77-v8+/overlays/vc4-kms-v3d.dtbo:overlays/vc4-kms-v3d.dtbo \


rpibootimage_intermediates := \
    $(call intermediates-dir-for,PACKAGING,rpibootimage)
BUILT_RPIBOOTIMAGE := $(rpibootimage_intermediates)/rpiboot.img

ALL_INSTALLED_RPIBOOT_FILES :=

unique_rpiboot_copy_files_pairs :=
$(foreach cf,$(RPIBOOT_COPY_FILES), \
    $(if $(filter $(unique_rpiboot_copy_files_pairs),$(cf)),, \
        $(eval unique_rpiboot_copy_files_pairs += $(cf))))

unique_rpiboot_copy_files_destinations :=
unique_rpiboot_copy_files_destinations_dirs :=
$(foreach cf,$(unique_rpiboot_copy_files_pairs), \
    $(eval _src := $(call word-colon,1,$(cf))) \
    $(eval _dest := $(call word-colon,2,$(cf))) \
    $(if $(filter $(unique_rpiboot_copy_files_destinations),$(_dest)), \
        $(warning RPIBOOT: Ignore copy file: $(_src)), \
        $(eval unique_rpiboot_copy_files_destinations += $(_dest)) \
        $(eval _dest_dir := $(dir $(_dest))) \
        $(if $(filter $(unique_rpiboot_copy_files_destinations_dirs),$(_dest_dir)),, \
            $(eval unique_rpiboot_copy_files_destinations_dirs += $(_dest_dir))) \
        $(eval _fulldest := $(call append-path,$(RPIBOOT_OUT_ROOT),$(_dest))) \
        $(eval $(call copy-one-file,$(_src),$(_fulldest))) \
        $(eval ALL_INSTALLED_RPIBOOT_FILES += $(_fulldest))))

unique_rpiboot_copy_files_pairs :=
unique_rpiboot_copy_files_destinations :=
unique_rpiboot_copy_files_destinations_dirs := $(filter-out .,$(patsubst %/,%,$(unique_rpiboot_copy_files_destinations_dirs)))


define build-rpibootimage-target
	mkdir -p $(dir $(1))
	dd if=/dev/zero of=$(1) bs=$$((1024*1024)) count=$(2)
	mkfs.fat -n "rpiboot" $(1)
	for item in $(ALL_INSTALLED_RPIBOOT_FILES); do \
		if [ "`dirname $${item}`" = "$(RPIBOOT_OUT_ROOT)" ] ; then \
			$(FAT16COPY) $(1) $${item} ; \
		fi ; \
	done
	for item in $(unique_rpiboot_copy_files_destinations_dirs); do \
		$(FAT16COPY) $(1) $(RPIBOOT_OUT_ROOT)/$${item} ; \
	done
endef


$(RPIBOOT_IMAGE): $(ALL_INSTALLED_RPIBOOT_FILES) | $(FAT16COPY)
	echo "Target rpiboot fs image: $(BUILT_RPIBOOTIMAGE)"
	$(call build-rpibootimage-target,$(BUILT_RPIBOOTIMAGE),64)
	echo "Install rpiboot fs image: $@"
	$(ACP) $(BUILT_RPIBOOTIMAGE) $@


droidcore rpibootimage: $(RPIBOOT_IMAGE)

.PHONY: rpibootimage

endif
