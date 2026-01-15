# Makefile - Top-level build system for os-dev
#
# =============================================================================
# BUILD SYSTEM (Story 1.3)
# =============================================================================
#
# Targets:
#   all     - Build kernel binary
#   image   - Create bootable disk image
#   qemu    - Run in QEMU
#   debug   - Run in QEMU with GDB stub
#   clean   - Remove build artifacts
#
# Disk Image Layout:
#   Sector 0:      Stage 1 (MBR, 512 bytes)
#   Sectors 1-12:  Stage 2 (6KB, 12 sectors)
#   Sectors 13+:   Kernel
#
# =============================================================================

ROOT := $(CURDIR)
BUILD := $(ROOT)/build

include $(ROOT)/config.mk

# =============================================================================
# Source Files
# =============================================================================

# Kernel C sources
KERNEL_C_SRCS := $(wildcard kernel/init/*.c)
KERNEL_C_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_C_SRCS))

# Kernel assembly sources
KERNEL_ASM_SRCS := $(wildcard kernel/init/*.S)
KERNEL_ASM_OBJS := $(patsubst %.S,$(BUILD)/%.o,$(KERNEL_ASM_SRCS))

# Test sources (only included when TEST_MODE=1)
TEST_C_SRCS := $(wildcard kernel/test/*.c)
TEST_C_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(TEST_C_SRCS))

# All kernel objects (tests added conditionally)
KERNEL_OBJS := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)
ifdef TEST_MODE
KERNEL_OBJS += $(TEST_C_OBJS)
endif

# Bootloader sources
STAGE1_SRC := boot/stage1.S
STAGE2_SRC := boot/stage2.S

# =============================================================================
# Output Files
# =============================================================================

KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
STAGE1_BIN := $(BUILD)/boot/stage1.bin
STAGE2_BIN := $(BUILD)/boot/stage2.bin
DISK_IMG := $(BUILD)/os-dev.img

# =============================================================================
# Disk Image Parameters
# =============================================================================

# 1.44 MB floppy size
DISK_SIZE := 1440

# Stage 2 starts at sector 1 (after MBR at sector 0)
STAGE2_SECTOR := 1

# Kernel starts at sector 13 (LBA), after stage 2
# Stage 2 occupies sectors 1-12 (12 sectors)
KERNEL_SECTOR := 13

# =============================================================================
# Phony Targets
# =============================================================================

.PHONY: all image qemu debug clean dirs test

all: dirs $(KERNEL_BIN)

image: dirs $(DISK_IMG)

# Test build: compile with TEST_MODE and run in QEMU
test:
	@echo "Building kernel with TEST_MODE enabled..."
	$(MAKE) clean
	$(MAKE) image TEST_MODE=1
	@echo "Running tests in QEMU..."
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -serial stdio -display none &
	@sleep 3
	@pkill -f "qemu-system-i386.*$(DISK_IMG)" || true
	@echo "Test run complete (check serial output above)"

# =============================================================================
# Directory Creation
# =============================================================================

dirs:
	@mkdir -p $(BUILD)/kernel/init
	@mkdir -p $(BUILD)/boot

# =============================================================================
# Kernel Build Rules
# =============================================================================

# Compile kernel C sources
# Uses cross-compiler with freestanding flags
$(BUILD)/kernel/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble kernel assembly sources
# Uses cross-compiler in 32-bit mode
$(BUILD)/kernel/%.o: kernel/%.S
	@mkdir -p $(dir $@)
	$(CC) -m32 -c $< -o $@

# Compile kernel test sources (only when TEST_MODE=1)
ifdef TEST_MODE
$(BUILD)/kernel/test/%.o: kernel/test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DTEST_MODE -c $< -o $@

# Also add TEST_MODE to init sources for test builds
$(BUILD)/kernel/init/%.o: kernel/init/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DTEST_MODE -c $< -o $@
endif

# Link kernel
# Entry point is _start, loads at physical address 0x100000
$(KERNEL_ELF): $(KERNEL_OBJS) $(ROOT)/scripts/kernel.ld
	$(LD) $(LDFLAGS) -T $(ROOT)/scripts/kernel.ld -o $@ $(KERNEL_OBJS)

# Extract raw binary from ELF
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Kernel size: $$(stat -c%s $@) bytes ($$(expr $$(stat -c%s $@) / 512 + 1) sectors)"

# =============================================================================
# Bootloader Build Rules
# =============================================================================

# Assemble stage1 bootloader (MBR)
# CRITICAL: stage1.bin MUST be exactly 512 bytes
$(STAGE1_BIN): $(STAGE1_SRC)
	$(CC) -m16 -c -o $(BUILD)/boot/stage1.o $<
	$(LD) --oformat binary -e _start -Ttext 0x7C00 -o $@ $(BUILD)/boot/stage1.o
	@SIZE=$$(stat -c%s $@); \
	if [ "$$SIZE" -ne 512 ]; then \
		echo "ERROR: stage1.bin is $$SIZE bytes, must be exactly 512"; \
		rm -f $@; \
		exit 1; \
	fi

# Assemble stage2 bootloader
# Contains both 16-bit (real mode) and 32-bit (protected mode) code
$(STAGE2_BIN): $(STAGE2_SRC)
	$(CC) -m16 -c -o $(BUILD)/boot/stage2.o $<
	$(LD) --oformat binary -e _start -Ttext 0x7E00 -o $@ $(BUILD)/boot/stage2.o
	@echo "Stage2 size: $$(stat -c%s $@) bytes"

# =============================================================================
# Disk Image Creation
# =============================================================================

# Create bootable disk image
# Layout:
#   Sector 0:     Stage 1 (MBR)
#   Sectors 1-4:  Stage 2
#   Sectors 5+:   Kernel
$(DISK_IMG): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "Creating disk image..."
	@echo "  Stage 1: 512 bytes at sector 0"
	@echo "  Stage 2: $$(stat -c%s $(STAGE2_BIN)) bytes at sector $(STAGE2_SECTOR)"
	@echo "  Kernel:  $$(stat -c%s $(KERNEL_BIN)) bytes at sector $(KERNEL_SECTOR)"
	# Create empty 1.44MB floppy image
	dd if=/dev/zero of=$@ bs=1K count=$(DISK_SIZE) 2>/dev/null
	# Write stage 1 to sector 0 (MBR)
	dd if=$(STAGE1_BIN) of=$@ conv=notrunc 2>/dev/null
	# Write stage 2 starting at sector 1
	dd if=$(STAGE2_BIN) of=$@ bs=512 seek=$(STAGE2_SECTOR) conv=notrunc 2>/dev/null
	# Write kernel starting at sector 13
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=$(KERNEL_SECTOR) conv=notrunc 2>/dev/null
	@echo "Disk image created: $@"

# =============================================================================
# Run Targets
# =============================================================================

# Run in QEMU
# -drive: Use raw disk image
# -serial stdio: Output serial to terminal (for future printk)
qemu: image
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -serial stdio

# Run in QEMU with GDB stub for debugging
# -s: Enable GDB server on port 1234
# -S: Pause execution until GDB connects
debug: image
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -serial stdio -s -S

# =============================================================================
# Clean Target
# =============================================================================

clean:
	rm -rf $(BUILD)
