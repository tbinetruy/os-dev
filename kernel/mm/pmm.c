/*
 * pmm.c - Physical Memory Manager
 *
 * Implements a bitmap-based physical frame allocator. Each bit in the
 * bitmap represents one 4KB page frame:
 *   - bit = 0: frame is free
 *   - bit = 1: frame is allocated
 *
 * The bitmap is stored in the kernel BSS section and supports up to
 * 4GB of physical memory (requires 128KB of bitmap space).
 *
 * Initialization parses the BIOS E820 memory map to determine which
 * regions are available for allocation. The first 1MB and kernel
 * memory are always marked as reserved.
 *
 * TODO(multicore): pmm_alloc_frame() and pmm_free_frame() are not
 * thread-safe. When adding SMP/multi-threading support, protect
 * bitmap operations with a spinlock and disable interrupts around
 * critical sections to prevent race conditions between threads and
 * interrupt handlers that may allocate frames.
 */

#include <pmm.h>
#include <vmm.h>
#include <mmap.h>
#include <bitmap.h>
#include <string.h>
#include <printk.h>

/*
 * Bitmap Configuration
 *
 * Support up to 4GB of physical memory. At 4KB per frame:
 *   - 4GB / 4KB = 1M frames
 *   - 1M frames / 8 bits per byte = 128KB bitmap
 */
#define MAX_MEMORY      (4ULL * 1024 * 1024 * 1024)  /* 4GB */
#define MAX_FRAMES      (MAX_MEMORY / PAGE_SIZE)     /* 1M frames */
#define BITMAP_SIZE     (MAX_FRAMES / 8)             /* 128KB */

/*
 * First allocatable frame (skip first 1MB)
 * Frames 0-255 cover addresses 0x00000 - 0xFFFFF (first 1MB)
 */
#define FIRST_ALLOC_FRAME   256

/*
 * Static bitmap stored in kernel BSS
 * Each bit represents one 4KB frame: 0=free, 1=allocated
 */
static uint8_t frame_bitmap[BITMAP_SIZE];

/*
 * Memory statistics
 */
static uint32_t total_frame_count;  /* Total frames in system */
static uint32_t free_frame_count;   /* Currently free frames */

#ifdef TEST_MODE
static uint32_t forced_test_frame;

int pmm_test_force_next_frame(uint32_t phys_addr)
{
    uint32_t frame = PHYS_TO_FRAME(phys_addr);

    if ((phys_addr & (PAGE_SIZE - 1)) != 0 ||
        phys_addr < DIRECT_MAP_PHYS_LIMIT || frame >= total_frame_count ||
        bitmap_test(frame_bitmap, frame)) {
        return -1;
    }
    forced_test_frame = phys_addr;
    return 0;
}
#endif

/* Bitmap operations provided by kernel/lib/bitmap.c */

/*
 * pmm_init - Initialize the physical memory manager
 *
 * Parses the E820 memory map and initializes the frame bitmap.
 * Steps:
 *   1. Mark all frames as allocated (safe default)
 *   2. Parse memory map, mark available regions as free
 *   3. Re-mark first 1MB as reserved
 *   4. Re-mark kernel frames as allocated
 */
void pmm_init(void)
{
    struct mmap_entry *mmap;
    uint32_t count;
    uint32_t i;
    uint32_t f;
    uint32_t mmap_virt;

    /* 1. Mark all frames as allocated initially (0xFF = all bits set) */
    memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));
    free_frame_count = 0;
    total_frame_count = 0;

    /*
     * Get memory map from bootloader
     *
     * boot_mmap_ptr contains the PHYSICAL address of the memory map
     * (at 0x504, set by stage2 bootloader). After paging is enabled,
     * we use the checked low-physical direct-map conversion.
     */
    if (vmm_direct_phys_to_virt(boot_mmap_ptr, &mmap_virt) != 0) {
        printk(LOG_ERROR, "PMM: memory map outside direct map\n");
        return;
    }
    mmap = (struct mmap_entry *)mmap_virt;
    count = boot_mmap_count;

    if (count == 0) {
        printk(LOG_ERROR, "PMM: No memory map available\n");
        return;
    }

    printk(LOG_INFO, "PMM: Parsing %d memory map entries\n", count);

    /* 2. Parse memory map, mark available regions as free */
    for (i = 0; i < count; i++) {
        uint64_t base = mmap[i].base;
        uint64_t length = mmap[i].length;
        uint32_t type = mmap[i].type;

        /* Skip regions above 4GB (we only support 32-bit addresses) */
        if (base >= MAX_MEMORY) {
            continue;
        }

        /* Truncate regions that extend beyond 4GB */
        if (base + length > MAX_MEMORY) {
            length = MAX_MEMORY - base;
        }

        /* Only mark available (type 1) regions as free */
        if (type == MMAP_TYPE_AVAILABLE) {
            uint32_t start_frame = PHYS_TO_FRAME((uint32_t)base);
            uint32_t end_frame = PHYS_TO_FRAME((uint32_t)(base + length));

            /* Track highest frame for total count */
            if (end_frame > total_frame_count) {
                total_frame_count = end_frame;
            }

            /* Mark each frame in this region as free */
            for (f = start_frame; f < end_frame; f++) {
                bitmap_clear(frame_bitmap, f);
                free_frame_count++;
            }

            printk(LOG_DEBUG, "PMM: Available 0x%x - 0x%x (%d frames)\n",
                   (uint32_t)base, (uint32_t)(base + length),
                   end_frame - start_frame);
        }
    }

    /* 3. Mark first 1MB as reserved (frames 0-255) unconditionally */
    for (f = 0; f < FIRST_ALLOC_FRAME; f++) {
        if (!bitmap_test(frame_bitmap, f)) {
            bitmap_set(frame_bitmap, f);
            free_frame_count--;
        }
    }

    /* 4. Mark kernel frames as allocated */
    uint32_t kernel_start_frame = PHYS_TO_FRAME(KERNEL_PHYS_START);
    uint32_t kernel_end_frame = PHYS_TO_FRAME(PAGE_ALIGN_UP(KERNEL_PHYS_END));

    for (f = kernel_start_frame; f < kernel_end_frame; f++) {
        if (!bitmap_test(frame_bitmap, f)) {
            bitmap_set(frame_bitmap, f);
            free_frame_count--;
        }
    }

    printk(LOG_INFO, "PMM: Kernel 0x%x - 0x%x (%d frames)\n",
           KERNEL_PHYS_START, KERNEL_PHYS_END,
           kernel_end_frame - kernel_start_frame);

    printk(LOG_INFO, "PMM: %d frames free (%d MB), %d frames total\n",
           free_frame_count,
           (free_frame_count * PAGE_SIZE) / (1024 * 1024),
           total_frame_count);
}

/*
 * pmm_alloc_frame - Allocate a physical page frame
 *
 * Scans the bitmap for the first free frame starting after 1MB,
 * marks it as allocated, and returns its physical address.
 *
 * Returns: Physical address on success, 0 if no frames available
 */
uint32_t pmm_alloc_frame(void)
{
    uint32_t f;

#ifdef TEST_MODE
    if (forced_test_frame != 0) {
        f = PHYS_TO_FRAME(forced_test_frame);
        bitmap_set(frame_bitmap, f);
        free_frame_count--;
        forced_test_frame = 0;
        return FRAME_TO_PHYS(f);
    }
#endif

    /* Start search after 1MB to skip reserved memory */
    for (f = FIRST_ALLOC_FRAME; f < total_frame_count; f++) {
        if (!bitmap_test(frame_bitmap, f)) {
            bitmap_set(frame_bitmap, f);
            free_frame_count--;
            return FRAME_TO_PHYS(f);
        }
    }

    /* No free frames available */
    printk(LOG_WARN, "PMM: Out of memory\n");
    return 0;
}

/*
 * pmm_free_frame - Free a physical page frame
 *
 * Validates the address and marks the frame as free.
 *
 * @phys_addr: Physical address of frame to free (must be 4KB-aligned)
 */
void pmm_free_frame(uint32_t phys_addr)
{
    uint32_t frame;

    /* Validate page alignment */
    if (phys_addr & (PAGE_SIZE - 1)) {
        printk(LOG_WARN, "PMM: free_frame unaligned addr 0x%x\n", phys_addr);
        return;
    }

    frame = PHYS_TO_FRAME(phys_addr);

    /* Validate frame range */
    if (frame >= total_frame_count) {
        printk(LOG_WARN, "PMM: free_frame invalid addr 0x%x\n", phys_addr);
        return;
    }

    /* Prevent freeing reserved frames (first 1MB) */
    if (frame < FIRST_ALLOC_FRAME) {
        printk(LOG_WARN, "PMM: free_frame in reserved region 0x%x\n", phys_addr);
        return;
    }

    /* Check for double-free */
    if (!bitmap_test(frame_bitmap, frame)) {
        printk(LOG_WARN, "PMM: double-free frame %d (0x%x)\n", frame, phys_addr);
        return;
    }

    bitmap_clear(frame_bitmap, frame);
    free_frame_count++;
}

/*
 * pmm_get_free_count - Get number of free frames
 */
uint32_t pmm_get_free_count(void)
{
    return free_frame_count;
}

/*
 * pmm_get_total_count - Get total number of frames
 */
uint32_t pmm_get_total_count(void)
{
    return total_frame_count;
}
