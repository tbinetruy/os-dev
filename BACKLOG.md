# Backlog

## Physical memory manager

### Align E820 available regions to complete physical frames

- **Status:** Todo
- **Location:** `kernel/mm/pmm.c`, `pmm_init()`
- **Problem:** `PHYS_TO_FRAME(base)` rounds an unaligned E820 region start
  down, which can mark a partially available frame as completely free. The
  region end also needs to be rounded down so only frames fully contained in
  the half-open interval `[base, base + length)` are made allocatable.
- **Fix:** Compute the boundaries while they are still 64-bit, align the start
  up and the exclusive end down, skip the region when the aligned start is not
  below the aligned end, and only then convert both addresses to frame indexes.
- **Also fix:** Do not cast an exclusive end of exactly 4 GiB to `uint32_t`
  before shifting; that wraps the address to zero. Shift the 64-bit value first
  so the exclusive frame index remains `1048576`.
- **Acceptance cases:**
  - An available region `[5 KiB, 9 KiB)` frees no frames.
  - An available region `[5 KiB, 12 KiB)` frees only frame 2 (`[8 KiB, 12 KiB)`).
  - Page-aligned available regions preserve their current behavior.
  - A region ending exactly at 4 GiB produces the correct exclusive end-frame
    index rather than zero.

## Process and boot stacks

### Migrate PID 0 from the unbounded bootstrap stack

- **Status:** Todo
- **Location:** `boot/stage2.S`, `kernel/init/entry.S`, `scripts/kernel.ld`,
  and future PID 0 initialization in `kernel/proc/`
- **Problem:** The bootloader initializes physical ESP to `0x90000`, which
  becomes virtual `0xC0090000` after the higher-half transition. This is an
  exclusive stack top, not the base of the documented `0x90000-0x9FFFF`
  region. Because i386 stacks grow downward, the bootstrap stack has no
  enforced size or guard page and can silently overwrite lower memory.
- **Fix:** After Story 3.5 provides guarded stack allocation, migrate PID 0
  either to a normal guarded kernel-stack slot or to an explicitly sized,
  linker-reserved initial stack. Keep the special stack non-freeable if it is
  static; otherwise record normal `struct kstack` ownership for teardown.
- **Acceptance cases:**
  - PID 0 has a named base, exclusive top, and compile-time-defined size.
  - The initial ESP is the exclusive top of that declared range.
  - A downward overflow reaches an unmapped guard page or another explicit
    detection mechanism before unrelated memory.
  - Bootloader, entry code, linker symbols, and process metadata agree on the
    same addresses and ownership.
  - The obsolete claim that ESP `0x90000` uses `0x90000-0x9FFFF` is removed.
