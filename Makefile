# Makefile - Top-level build system for os-dev
#
# Targets:
#   all     - Build kernel binary
#   image   - Create bootable disk image
#   qemu    - Run in QEMU
#   debug   - Run in QEMU with GDB stub
#   clean   - Remove build artifacts

ROOT := $(CURDIR)
BUILD := $(ROOT)/build

include $(ROOT)/config.mk

# Source files
KERNEL_SRCS := $(wildcard kernel/init/*.c)
KERNEL_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_SRCS))

BOOT_SRC := boot/stage1.S

# Output files
KERNEL_BIN := $(BUILD)/kernel.bin
STAGE1_BIN := $(BUILD)/stage1.bin
DISK_IMG := $(BUILD)/os-dev.img

# Image parameters
DISK_SIZE := 1440  # 1.44 MB floppy size in KB

.PHONY: all image qemu debug clean dirs

all: dirs $(KERNEL_BIN)

image: dirs $(DISK_IMG)

# Create build directories
dirs:
	@mkdir -p $(BUILD)/kernel/init
	@mkdir -p $(BUILD)/boot

# Compile kernel C sources
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -T $(ROOT)/scripts/kernel.ld -o $(BUILD)/kernel.elf $(KERNEL_OBJS)
	$(OBJCOPY) -O binary $(BUILD)/kernel.elf $@

# Assemble stage1 bootloader
$(STAGE1_BIN): $(BOOT_SRC)
	$(CC) -m16 -c -o $(BUILD)/stage1.o $<
	$(LD) --oformat binary -e _start -Ttext 0x7C00 -o $@ $(BUILD)/stage1.o

# Create disk image
$(DISK_IMG): $(STAGE1_BIN) $(KERNEL_BIN)
	@echo "Creating disk image..."
	dd if=/dev/zero of=$@ bs=1K count=$(DISK_SIZE) 2>/dev/null
	dd if=$(STAGE1_BIN) of=$@ conv=notrunc 2>/dev/null
	@echo "Disk image created: $@"

# Run in QEMU
qemu: image
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -serial stdio

# Run in QEMU with GDB stub
debug: image
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -serial stdio -s -S

# Clean build artifacts
clean:
	rm -rf $(BUILD)
