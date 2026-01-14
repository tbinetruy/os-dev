# config.mk - Build configuration for os-dev
# Cross-compiler toolchain settings

# Cross-compiler prefix
CROSS := i686-elf-

# Toolchain
CC := $(CROSS)gcc
AS := $(CROSS)as
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

# C compiler flags
CFLAGS := -m32 -std=gnu99 -ffreestanding -nostdlib
CFLAGS += -fno-builtin -fno-stack-protector -fno-pic
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g -O0

# Include paths
CFLAGS += -I$(ROOT)/kernel/include

# Assembler flags
ASFLAGS := --32

# Linker flags
LDFLAGS := -m elf_i386 -nostdlib
