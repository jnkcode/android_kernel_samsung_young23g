KERNEL_OUT ?= $(OUT)/obj/KERNEL_OBJ
KERNEL_DEFCONFIG ?= cyanogen_vivalto3gvn_defconfig
KERNEL_CONFIG := $(KERNEL_OUT)/.config

TARGET_KERNEL_SOURCE ?= $(PWD)

CCACHE ?= $(TARGET_KERNEL_SOURCE)/../../../prebuilts/misc/linux-x86/ccache/ccache

ifeq ($(USES_UNCOMPRESSED_KERNEL),true)
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/Image
else
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage
endif

all: $(TARGET_PREBUILT_KERNEL)

kernelheader: $(KERNEL_OUT)
	$(MAKE) -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_OUT) ARCH=arm CROSS_COMPILE="$(CCACHE) arm-eabi-" headers_install

$(KERNEL_OUT):
	mkdir -p $(KERNEL_OUT)

$(KERNEL_CONFIG): $(TARGET_KERNEL_SOURCE)/arch/arm/configs/$(KERNEL_DEFCONFIG)
	$(MAKE) -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_OUT) ARCH=arm CROSS_COMPILE="$(CCACHE) arm-eabi-" $(KERNEL_DEFCONFIG)

$(TARGET_PREBUILT_KERNEL): kernelheader $(KERNEL_OUT) $(KERNEL_CONFIG)
	$(MAKE) -C $(TARGET_KERNEL_SOURCE) O=$(KERNEL_OUT) ARCH=arm CROSS_COMPILE="$(CCACHE) arm-eabi-" zImage -j4
