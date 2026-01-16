#
# tests/Makefile - Host-side unit test build
#
# Builds and runs tests for pure algorithms using Unity framework.
# These tests run on the development host, not in QEMU.
#
# Usage:
#   make            - Build and run all tests
#   make build      - Build tests without running
#   make test       - Run all tests
#   make clean      - Remove build artifacts
#   make test_NAME  - Build specific test (e.g., make test_bitmap)
#

# Compiler settings - use host compiler, not cross-compiler
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
CFLAGS += -I../kernel/include
CFLAGS += -Ihost
CFLAGS += -DHOST_TEST
CFLAGS += -g  # Debug symbols for development

# Unity framework
UNITY_SRC = host/unity/unity.c

# Test sources - auto-discovered from host/test_*.c
TEST_SRCS = $(wildcard host/test_*.c)
TEST_BINS = $(patsubst host/test_%.c,test_%,$(TEST_SRCS))

# =============================================================================
# Kernel-Linked Tests
# =============================================================================
# Some tests need to link against actual kernel code (not just headers).
# Define the kernel sources each test needs here.
#
# To add a new kernel-linked test:
#   1. Create host/test_NAME.c
#   2. Add: KERNEL_SRCS_NAME = ../kernel/path/to/source.c
#
# The test will automatically link against the specified kernel sources.
# =============================================================================

KERNEL_SRCS_gdt = ../kernel/init/gdt.c

# Colors for output (optional, disable with NO_COLOR=1)
ifndef NO_COLOR
    GREEN = \033[0;32m
    RED = \033[0;31m
    YELLOW = \033[0;33m
    NC = \033[0m
else
    GREEN =
    RED =
    YELLOW =
    NC =
endif

.PHONY: all build test clean help

# Default: build and run all tests
all: build test

# Build all test binaries
build: $(TEST_BINS)
	@echo "$(GREEN)All tests built successfully$(NC)"

# Build individual test binary
# If KERNEL_SRCS_% is defined, link against those kernel sources
test_%: host/test_%.c $(UNITY_SRC)
	@echo "$(YELLOW)Building $@...$(NC)"
	@$(CC) $(CFLAGS) -o $@ $^ $(KERNEL_SRCS_$*)

# Run all tests
test: $(TEST_BINS)
	@echo ""
	@echo "$(YELLOW)========================================$(NC)"
	@echo "$(YELLOW)     Running Host-Side Unit Tests      $(NC)"
	@echo "$(YELLOW)========================================$(NC)"
	@echo ""
	@failed=0; \
	for t in $(TEST_BINS); do \
		echo "$(YELLOW)--- Running $$t ---$(NC)"; \
		./$$t; \
		if [ $$? -ne 0 ]; then \
			failed=$$((failed + 1)); \
		fi; \
		echo ""; \
	done; \
	echo "$(YELLOW)========================================$(NC)"; \
	if [ $$failed -eq 0 ]; then \
		echo "$(GREEN)All test suites passed!$(NC)"; \
	else \
		echo "$(RED)$$failed test suite(s) failed$(NC)"; \
		exit 1; \
	fi

# Clean build artifacts
clean:
	@rm -f $(TEST_BINS)
	@rm -f *.o
	@echo "$(GREEN)Cleaned test artifacts$(NC)"

# Help
help:
	@echo "Host-Side Unit Tests"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build and run all tests (default)"
	@echo "  build      - Build tests without running"
	@echo "  test       - Run all tests"
	@echo "  test_NAME  - Build specific test (e.g., test_bitmap)"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this message"
	@echo ""
	@echo "Available tests:"
	@for t in $(TEST_BINS); do echo "  $$t"; done
	@echo ""
	@echo "Adding new tests:"
	@echo "  1. Create host/test_NAME.c"
	@echo "  2. Include unity/unity.h"
	@echo "  3. Implement setUp(), tearDown(), and test functions"
	@echo "  4. Run 'make' to build and execute"
