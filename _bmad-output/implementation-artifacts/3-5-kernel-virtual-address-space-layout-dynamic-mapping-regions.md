# Story 3.5: Kernel Virtual Address Space Layout & Dynamic Mapping Regions

Status: ready-for-dev

<!-- Approved corrective story. Implement and review before Story 4.1 resumes. -->

## Story

As a developer,
I want kernel virtual memory divided into explicit owned regions,
so that heap, direct-map, page-table, and kernel-stack mappings cannot
silently collide as the kernel grows.

## Acceptance Criteria

1. **AC1: One canonical, non-overlapping kernel virtual layout**
   - Given the i386 4 GiB virtual address space
   - When the VMM-owned layout constants are inspected
   - Then they define page-aligned, half-open ranges with these exact bounds:
     - user space: `[0x00000000, 0xC0000000)`
     - low physical direct map: `[0xC0000000, 0xC1000000)` mapping physical
       `[0x00000000, 0x01000000)`
     - kernel heap: `[0xC1000000, 0xE0000000)`
     - dynamic kernel mappings: `[0xE0000000, 0xF0000000)`
     - reserved expansion: `[0xF0000000, 0xFF000000)`
     - kernel stack slots: `[0xFF000000, 0xFFC00000)`
     - recursive page tables: `[0xFFC00000, 0x100000000)` (the final
       4 MiB, represented without overflowing a 32-bit end constant)
   - And inclusive end aliases, if exposed for diagnostics, match the approved
     last addresses (`0xBFFFFFFF`, `0xC0FFFFFF`, `0xDFFFFFFF`,
     `0xEFFFFFFF`, `0xFEFFFFFF`, `0xFFBFFFFF`, `0xFFFFFFFF`)
   - And compile-time assertions or host tests prove adjacency,
     page-alignment, and no overlap
   - And constants live in one VMM-owned header; other subsystems do not copy
     raw region addresses or derive virtual ownership from PMM frame numbers.

2. **AC2: Direct-map conversions are bounded and cannot masquerade as
   arbitrary translation**
   - Given physical `phys` or virtual `virt`
   - When a direct-map conversion is requested
   - Then physical input is accepted only below `0x01000000` and virtual input
     only in `[0xC0000000, 0xC1000000)`
   - And the public APIs are exactly
     `int vmm_direct_phys_to_virt(uint32_t phys, uint32_t *virt_out)` and
     `int vmm_direct_virt_to_phys(uint32_t virt, uint32_t *phys_out)`
   - And they return `-EINVAL` for a null output pointer or out-of-range input,
     without writing the output, instead of wrapping or producing a plausible
     address in another owned region
   - And all retained low-memory users (E820 map, VGA, kernel/boot objects) use
     the bounded contract
   - And no page-table, heap, or stack path uses direct-map conversion on an
     arbitrary result from `pmm_alloc_frame()`.

3. **AC3: Normal mapping preserves existing ownership**
   - Given a destination PTE is already present
   - When `vmm_map_page()` targets that page
   - Then it returns `-EEXIST` and leaves the original frame,
     flags, PDE/PTE state, and TLB-visible mapping unchanged
   - And `vmm_map_page()` rejects an unaligned virtual or physical address with
     `-EINVAL` rather than silently aligning either input
   - And `EEXIST` is added to `kernel/include/errno.h`
   - And any intentional replacement uses a separate, explicit API whose
     caller owns the old mapping and whose TLB behavior is tested
   - And `vmm_unmap_page()` never frees the mapped PMM frame implicitly
   - And ordinary map/unmap calls reject the reserved expansion range
     `[0xF0000000, 0xFF000000)` and recursive range
     `[0xFFC00000, 4GiB)` with `-EINVAL`; only VMM-internal recursive self-map
     setup may write PDE 1023
   - And `vmm_unmap_page()` returns `int` so invalid reserved addresses can be
     reported without mutation.

4. **AC4: Heap ownership is fixed and bounded**
   - Given VMM initialization is complete
   - When `heap_init()` runs
   - Then the heap starts at `0xC1000000`, not after `_kernel_end`
   - And `scripts/kernel.ld` exports `__kernel_heap_start = 0xC1000000`, asserts
     `_kernel_end <= __kernel_heap_start`, and fails linking with a clear message
   - And runtime initialization verifies the linker symbol address equals the
     production C constant `KERNEL_HEAP_START` before mapping heap pages
   - And initial heap state is not published until every required page is
     mapped; a boot-time failure panics immediately with a precise diagnostic
     and does not require resource rollback because initialization cannot
     continue
   - And later expansion checks size arithmetic for overflow, never maps at or
     above `0xE0000000`, and returns `NULL`/`-ENOMEM` with heap metadata,
     mappings, and free-frame count restored
   - And existing first-fit, 8-byte alignment, split, free, and coalescing
     behavior remains unchanged.

5. **AC5: Recursive page-directory mapping is the page-table access path**
   - Given the active boot page directory
   - When paging/VMM initialization completes
   - Then supervisor-only PDE 1023 points to the physical address loaded in
     CR3, with writable/present flags and no `PAGE_USER`
   - And `vmm_init()` installs PDE 1023 through the still-direct-mapped boot
     page directory, reloads CR3, and only then switches VMM internals to the
     recursive aliases
   - And the active page directory is visible at `0xFFFFF000`
   - And page table for PDE index `n` is visible at
     `0xFFC00000 + (n * PAGE_SIZE)`
   - And VMM create/inspect/map/unmap/translate operations use these recursive
     aliases rather than `P2V(page_table_phys)`
   - And a newly allocated page-table frame is installed, the recursive alias
     is invalidated, and the full page is zeroed through that alias before any
     target PTE is read or written
   - And page-table creation failure restores the prior PDE and frees the new
     PMM frame; no partial mapping becomes visible
   - And future address spaces are explicitly required to install their own
     PDE 1023 self-reference while preserving the shared kernel-region map.

6. **AC6: Kernel stacks use dedicated guarded slots**
   - Given the stack region `[0xFF000000, 0xFFC00000)`
   - When the stack allocator initializes
   - Then `KSTACK_GUARD_PAGES` is 1 and `KSTACK_PAGES` is 1 for the current
     kernel-thread milestone
   - And it owns deterministic slots containing an unmapped lower guard page
     followed by one mapped 4 KiB stack page
   - And slot size and slot count are derived from the region size, page size,
     guard-page count, and stack-page count rather than independently hard-coded
   - And the region provides exactly 1536 non-overlapping slots without
     entering the recursive window
   - When a stack is allocated
   - Then one free slot and one PMM frame are acquired transactionally, the
     stack page is mapped supervisor writable, the guard remains non-present,
     and initial ESP is the exclusive top of the mapped page
   - And the public interface is
     `struct kstack { uint32_t guard_base; uint32_t stack_base; uint32_t top; };`,
     `int kstack_alloc(struct kstack *stack)`, and
     `int kstack_free(struct kstack *stack)`
   - And `kstack_alloc()` returns `-EINVAL` for null output, zeroes all output
     fields on every allocation failure, and returns 0 only for a complete stack
   - And `kstack_free()` returns `-EINVAL` for null/invalid/non-owned input and
     clears all fields after a successful free
   - When allocation or mapping fails
   - Then the frame and slot are returned and pre-existing mappings are
     unchanged
   - When a valid normal stack is freed
   - Then the mapped physical frame is captured, the page is unmapped, the PMM
     frame is freed, and the slot becomes reusable
   - And invalid, duplicate, misaligned, guard-page, or out-of-region frees are
     rejected without changing allocator state
   - And exhaustion returns `-ENOMEM` without crossing `0xFFC00000`.

7. **AC7: PID 0 bootstrap stack is explicitly exceptional**
   - Given entry from the bootloader
   - When paging is enabled and PID 0 is later represented
   - Then physical `0x00090000`, reached through the direct map at virtual
     `0xC0090000`, is documented as its exclusive initial stack top, not the
     base of a 4 KiB stack
   - And because i386 stacks grow downward, the first push writes below that
     address
   - And the bootstrap stack has no enforced lower bound or guard page; its
     underlying low physical memory is PMM-reserved, but it is not a normal
     `struct kstack` allocation
   - And it is never passed to `kstack_free()`
   - And migration of PID 0 to an explicitly bounded static or guarded stack is
     deferred to the documented backlog item.

8. **AC8: Verification proves boundaries, rollback, and boot compatibility**
   - Given host and in-kernel test targets
   - When the Story 3.5 suite runs
   - Then it covers every exact region boundary, recursive address formula,
     direct-map first/last valid and first invalid inputs, non-overwriting map
     behavior, heap start/end, stack first/last slot, guard non-presence,
     allocation/free reuse, exhaustion, invalid/double free, and injected PMM
     and VMM failures
   - And recoverable rollback tests compare PMM free counts, mappings, and slot
     state before and after runtime heap-expansion, page-table, and stack
     allocation failures
   - And ownership rejection is tested at `0xF0000000`, `0xFFC00000`, and
     `0xFFFFF000`, proving no PDE/PTE/TLB mutation
   - And tests exercise a page-table frame above the 16 MiB direct-map limit by
     forcing PMM to allocate a real available frame, reserved by the test
     fixture, above 16 MiB; no fabricated or invalid physical address is used
   - And `make`, `make host-test`, and `make test` pass with no regressions
   - And the build fails clearly before image creation if `kernel.bin` exceeds
     the current effective 65536-byte stage-2 copy capacity
   - And a normal QEMU boot reaches `Boot complete` with VGA and serial output
     still functional and no unexpected page fault.

## Tasks / Subtasks

- [ ] **Task 1: Establish the canonical layout contract** (AC: #1, #2, #7)
  - [ ] 1.1 Extend `kernel/include/vmm.h` (or a VMM-owned header included by
    it) with named starts, exclusive ends/sizes, PDE indices, recursive
    formulas, direct-map physical limit, and stack-slot geometry.
  - [ ] 1.2 Use half-open ranges internally; avoid a 32-bit
    `RECURSIVE_END_EXCLUSIVE` value that wraps to zero.
  - [ ] 1.3 Replace the obsolete flat-layout diagram and document owner,
    allowed allocator, and mapping policy for every region.
  - [ ] 1.4 Replace raw conversion macros with
    `vmm_direct_phys_to_virt()` / `vmm_direct_virt_to_phys()` using the exact
    AC2 signatures and `-EINVAL` behavior; update only legitimate bounded users.
  - [ ] 1.5 Define `KSTACK_GUARD_PAGES = 1`, `KSTACK_PAGES = 1`, and derive
    `KSTACK_SLOT_PAGES`, `KSTACK_SLOT_SIZE`, and `KSTACK_SLOT_COUNT`; add
    compile-time or host-testable assertions that the current policy yields
    1536 slots and that stacks use PDE indices 1020-1022 while PDE 1023 remains
    recursive.

- [ ] **Task 2: Install and adopt recursive paging** (AC: #3, #5)
  - [ ] 2.1 In `vmm_init()`, use the still-direct-mapped `boot_page_directory`
    to install PDE 1023 against the active CR3 physical frame, reload CR3, and
    only then adopt recursive aliases. No `entry.S` bootstrap implementation is
    required beyond keeping its comments consistent.
  - [ ] 2.2 Add documented helpers for current page directory and current page
    table aliases using `0xFFFFF000` and `0xFFC00000 + index * 4096`.
  - [ ] 2.3 Refactor `kernel/mm/vmm.c` so existing and newly created tables are
    accessed only through recursive aliases after initialization.
  - [ ] 2.4 For a missing PDE: allocate a frame, publish the supervisor PDE,
    invalidate the recursive-table alias, zero all 4096 bytes through that
    alias, then operate on the target PTE.
  - [ ] 2.5 Make `vmm_map_page()` return `-EINVAL` for unaligned addresses and
    `-EEXIST` for a present PTE, without mutation; add `EEXIST` to `errno.h`.
  - [ ] 2.6 Change `vmm_unmap_page()` to return `int`, preserve its unmap-only
    ownership and precise TLB invalidation, and retain
    `vmm_get_physaddr()` offset behavior.
  - [ ] 2.7 Reject ordinary map/unmap requests in reserved expansion and
    recursive ranges with `-EINVAL`; keep PDE 1023 mutation private to recursive
    self-map setup and test `0xF0000000`, `0xFFC00000`, `0xFFFFF000` boundaries.

- [ ] **Task 3: Move and bound the heap** (AC: #1, #4)
  - [ ] 3.1 Change `heap_init()` to start exactly at `KERNEL_HEAP_START`
    (`0xC1000000`) and verify the destination pages are initially unmapped.
  - [ ] 3.2 Export `__kernel_heap_start = 0xC1000000` and add a linker `ASSERT`
    proving `_kernel_virt_end <= __kernel_heap_start`; at runtime verify the
    linker symbol equals the production C `KERNEL_HEAP_START` constant.
  - [ ] 3.3 Preflight expansion page count and address arithmetic so no request
    wraps or reaches `KERNEL_HEAP_END_EXCLUSIVE`.
  - [ ] 3.4 Keep `free_list` and the initialized state unpublished until all
    initial mappings succeed; on failure, log the operation/address and panic
    without adding rollback work. Preserve the transactional later-expansion
    cleanup because those failures return to a running kernel.
  - [ ] 3.5 Update heap diagnostics/tests to assert the fixed region while
    retaining first-fit, alignment, splitting, coalescing, and `kmalloc(0)`
    semantics.

- [ ] **Task 4: Add the kernel-stack region allocator** (AC: #6, #7)
  - [ ] 4.1 Add `struct kstack` and the exact `kstack_alloc()` / `kstack_free()`
    signatures from AC6 in `kernel/include/kstack.h`, with implementation in
    `kernel/mm/kstack.c`; keep process policy out of this module.
  - [ ] 4.2 Track the derived fixed-size slots with a bounded bitmap or
    equivalent static metadata; size it from `KSTACK_SLOT_COUNT`, not a
    duplicate `1536` literal, and do not allocate the metadata from the heap.
  - [ ] 4.3 Allocate guard/stack virtual addresses from slot index, confirm both
    are non-present, allocate one PMM frame, map only the upper page, and return
    top-of-stack plus any handle/address required for teardown.
  - [ ] 4.4 On every failure edge, unwind in reverse order; never mark a slot
    allocated until the mapping transaction can be committed, and zero the
    caller's three output fields on allocation failure.
  - [ ] 4.5 On free, validate exact slot ownership before mutation, capture the
    physical frame before unmapping, then unmap/free/release exactly once and
    clear the caller's three fields after successful free.
  - [ ] 4.6 Document that `0xC0090000` is PID 0's exclusive initial stack top,
    not a stack base; state that it grows downward without an enforced bound or
    guard and is never accepted by `kstack_free()`.

- [ ] **Task 5: Update boot and integration assumptions** (AC: #2, #5, #7)
  - [ ] 5.1 Keep the existing 16 MiB low physical mapping in `entry.S`; install
    the recursive PDE in `vmm_init()` without broadening direct-map semantics.
  - [ ] 5.2 Reconcile comments in `kernel/init/entry.S`, `kernel/mm/vmm.c`,
    `kernel/include/vmm.h`, `kernel/mm/heap.c`, and `kernel/init/main.c` with
    the new ownership contract.
  - [ ] 5.3 Initialize VMM/recursive access before heap and stack allocators;
    stack allocation need not be consumed by process code until Story 4.1.
  - [ ] 5.4 Audit all repository `P2V`/`V2P` call sites. Preserve VGA, E820,
    boot page-directory, and linker-symbol uses only where their addresses are
    provably inside the direct map.
  - [ ] 5.5 Add a clear build-time size check that rejects Story 3.5
    `kernel.bin` above the bootloader's current effective 64 KiB copy capacity.
    Do not change loader constants or claim support above 64 KiB; robust
    larger-kernel/CHS loading is a separate corrective boot story.

- [ ] **Task 6: Add focused host-side verification** (AC: #1-#6, #8)
  - [ ] 6.1 Add pure layout/slot-index tests under `tests/host/` and register any
    kernel-linked source/flags in `tests/Makefile`.
  - [ ] 6.2 Add a `HOST_TEST`-only backing-base seam so heap algorithms
    dereference a real host buffer while production always uses the immutable
    `KERNEL_HEAP_START`; extend heap mocks for upper-bound rejection, an
    initial-init failure that panics before publishing usable heap state, map
    collision, PMM failure, and integer overflow/oversize requests. A host test
    that catches panic must reset its own mock state; production cleanup is not
    part of the boot-failure contract.
  - [ ] 6.3 Test stack first/last slot, full exhaustion, reuse, guard presence,
    invalid/double free, PMM failure, map failure, and exact rollback counts.
  - [ ] 6.4 Test recursive index/address formulas and bounded conversion helpers
    without dereferencing kernel virtual addresses on the host; keep separate
    pure layout tests asserting production `KERNEL_HEAP_START`.

- [ ] **Task 7: Add in-kernel VMM and stack integration tests** (AC: #2-#8)
  - [ ] 7.1 Extend `kernel/test/test_vmm.c` for PDE 1023, page directory alias,
    page-table aliases, supervisor flags, collision preservation, and recursive
    map/unmap/translation.
  - [ ] 7.2 Replace both current heap-owned `0xD0000000` ad hoc addresses:
    `test_vmm.c` uses dynamic test page `0xE0000000`, and `test_fault.c` uses
    distinct page `0xE0001000`; each test unmaps and frees its page/frame.
  - [ ] 7.3 Add `kernel/test/test_kstack.c` and register it in
    `kernel/test/test_runner.c`; verify guard-page non-presence without causing
    an unrecoverable fault unless using the existing recoverable PF test hook.
  - [ ] 7.4 Add deterministic failure injection under `TEST_MODE` where needed
    to force PMM to return a real available frame reserved by the fixture above
    16 MiB and each rollback edge; never fabricate a physical address or
    destructively exhaust the live PMM.
  - [ ] 7.5 Re-run existing PMM, page-fault, heap, VGA, boot, and host suites to
    prove no ownership or direct-map regression.

- [ ] **Task 8: Build and runtime verification** (AC: #4, #5, #8)
  - [ ] 8.1 Run `make clean && make` and confirm linker boundary checks pass.
  - [ ] 8.2 Confirm the build rejects a `kernel.bin` larger than 65536 bytes with
    a clear message before producing a misleading boot image.
  - [ ] 8.3 Run `make host-test` and record suite totals.
  - [ ] 8.4 Run `make test`; confirm all suites finish with zero failures and
    no leaked frames/slots after injected failures.
  - [ ] 8.5 Run normal `make qemu`; confirm serial/VGA initialization, heap at
    `0xC1000000`, recursive VMM initialization, and `Boot complete`.
  - [ ] 8.6 Inspect the linked ELF/map (`readelf`/`nm`/`objdump` as appropriate)
    to prove kernel end, page-aligned paging objects, and region symbols.

## Dev Notes

### Why This Corrective Story Exists

Story 3.2 established a useful 16 MiB higher-half direct map, but its original
general `P2V()` wording and overwrite-capable mapper are not durable contracts.
Story 3.4 then made the heap grow upward from `_kernel_end`. A future thread
stack based on `P2V(pmm_alloc_frame())` can therefore fault above 16 MiB or
collide with heap/page-table ownership. Story 3.5 corrects those assumptions
before process work resumes; it does not discard the existing PMM, VMM, page
fault, or first-fit heap implementations.

### Required Region Ownership

| Range (half-open) | PDEs | Owner / allowed use |
|---|---:|---|
| `0x00000000-0xC0000000` | 0-767 | Future per-process user mappings |
| `0xC0000000-0xC1000000` | 768-771 | Fixed low-physical direct map only |
| `0xC1000000-0xE0000000` | 772-895 | Kernel heap allocator only |
| `0xE0000000-0xF0000000` | 896-959 | Explicit dynamic/test/device mappings |
| `0xF0000000-0xFF000000` | 960-1019 | Reserved; no allocator in this story |
| `0xFF000000-0xFFC00000` | 1020-1022 | Kernel-stack slot allocator only |
| `0xFFC00000-4GiB` | 1023 | Recursive page-table machinery only |

Do not place general allocations in the dynamic mapping range as a shortcut.
This story reserves and documents that ownership; a general vmalloc/ioremap
allocator is not required unless an approved caller is introduced.
Ordinary map/unmap APIs remain callable in direct-map, heap, dynamic-test, and
stack regions by their owning subsystems. They reject reserved expansion and
recursive addresses; only private VMM setup may establish PDE 1023.

### Recursive Mapping Formulas and Ordering

- Current page directory virtual address: `0xFFFFF000`.
- Page table for PDE `n`: `0xFFC00000 + n * 0x1000`.
- PTE address for virtual `v`: recursive table base for `PDE_INDEX(v)` plus
  `PTE_INDEX(v) * sizeof(uint32_t)`.
- PDE 1023 must contain `(read_cr3() & PAGE_FRAME_MASK) | PAGE_PRESENT |
  PAGE_WRITABLE`; it must not contain `PAGE_USER`.
- `vmm_init()` writes PDE 1023 through the still-direct-mapped boot page
  directory, reloads CR3, and switches to recursive aliases only afterward.
- When creating a missing page table, writing its PDE changes whether its
  recursive alias exists. Invalidate that alias (or reload CR3) before zeroing
  the frame through the alias. Zero before inspecting or publishing any target
  PTE.
- Normal mapping checks `PAGE_PRESENT` before write. A collision is a caller
  ownership error returned as `-EEXIST`, not success and not an implicit remap.
- Normal mapping returns `-EINVAL` for either unaligned address. Normal map and
  integer-returning unmap reject `[0xF0000000, 0xFF000000)` and
  `[0xFFC00000, 4GiB)` without changing paging state.

### Transaction and Failure Rules

- Ownership is acquired in the order: validate/preflight virtual range → claim
  virtual slot (if applicable) → allocate PMM frame → install mapping → commit
  allocator metadata.
- Rollback is reverse order: remove only mappings created by this operation →
  free only frames acquired by it → release virtual slot → restore counters and
  bounds.
- Never use a failed normal map call as evidence that the destination is safe;
  compare the prior mapping and flags explicitly in tests.
- Required boot infrastructure may panic immediately after a precise log once
  it has avoided publishing partially initialized state. Cleanup before an
  unconditional boot panic is not required and should not risk hiding the
  original failure. Runtime heap/stack exhaustion is recoverable (`NULL` or
  negative errno) and therefore requires complete rollback.
- Page-table pages may remain allocated while their PDE is in use. Reclaiming
  empty page tables is not required here and must not be added casually because
  recursive aliases and future shared kernel mappings depend on them.

### Existing Code to Extend, Not Reimplement

- `kernel/mm/vmm.c` already owns map, unmap, and translation but currently
  converts page-table frames with `P2V()` and overwrites present PTEs.
- `kernel/mm/heap.c` already has first-fit, splitting, coalescing, later-growth
  rollback, and debug counters. Preserve those algorithms while changing the
  start and bounds and delaying publication of initial heap state until setup
  succeeds.
- `kernel/init/entry.S` already maps physical 0-16 MiB at
  `0xC0000000-0xC0FFFFFF`, removes the identity aliases, and moves ESP from
  physical `0x90000` to virtual `0xC0090000`. That value is the exclusive
  initial top of a downward-growing bootstrap stack; the current code does not
  enforce a lower bound or guard page.
- The page-fault test hook from Story 3.3 can verify a guard page if needed;
  prefer presence inspection where it proves the contract without faulting.
- The build already auto-discovers `kernel/mm/*.c` and `kernel/test/*.c`; only
  test registration and host-test linkage require explicit updates.

### Testing Expectations

- Host tests are the primary place for exhaustive boundary/failure injection;
  in-kernel tests prove real MMU behavior.
- A high-frame recursive test must reserve and force allocation of a real PMM
  frame above 16 MiB. It must not depend on first-fit naturally reaching the
  boundary and must never fabricate a physical address absent from usable RAM.
- Capture baseline PTE/physical mapping, PMM free count, and stack-slot free
  count before each failure test; assert exact restoration afterward.
- Do not treat `kfree()` as returning heap backing pages to PMM; Story 3.4's
  allocator retains mapped heap pages. Leak tests concern failed expansion and
  stack/mapping teardown, not ordinary freed heap blocks.
- The current `make test` uses a timed QEMU run; verify the serial summary, not
  only the make command exit status.
- In test builds, stacks may be filled with a known pattern so later work can
  measure a high-water mark. This is diagnostic support, not a requirement for
  allocation correctness in this story.
- Host heap tests use a `HOST_TEST`-only backing-base seam to dereference a real
  host buffer. Production `KERNEL_HEAP_START` is immutable and is verified by
  separate pure layout tests; do not alias `_kernel_end` to a host buffer.
- `test_vmm.c` owns `0xE0000000` and `test_fault.c` owns `0xE0001000` during
  their tests. They are distinct dynamic-test pages and each must clean up.

### Non-Goals

- Per-process address-space creation, CR3 switching, or copying kernel mappings
  (Story 5.2), beyond documenting the recursive PDE requirement.
- General-purpose `vmalloc`, `ioremap`, device-MMIO, or temporary-map allocator.
- Multiple mapped pages per kernel stack, automatic stack growth, PID 0 stack
  migration (tracked in `BACKLOG.md`), scheduler/task integration, or context
  switching (Epic 4).
- User stacks, demand paging, copy-on-write, page-table-page reclamation, SMP
  locking, or heap algorithm replacement.
- Enlarging the low physical direct map beyond 16 MiB.
- Changing the stage-2 kernel loader or claiming kernel support above its
  current effective 64 KiB copy capacity. A robust larger-kernel/CHS loader is
  a separate corrective boot story.

### Source Tree Notes

Expected primary changes during implementation (not performed by this story
artifact):

- `kernel/include/vmm.h`, `kernel/mm/vmm.c`: canonical layout, bounded direct
  conversion, recursive access, collision-safe mapping.
- `kernel/include/heap.h`, `kernel/mm/heap.c`: fixed heap region and bounds.
- `kernel/include/errno.h`: add `EEXIST`; retain `EINVAL` and `ENOMEM`.
- `kernel/include/kstack.h`, `kernel/mm/kstack.c` (recommended): stack-region
  allocator isolated from future `proc/` policy.
- `kernel/init/entry.S`, `kernel/init/main.c`, `scripts/kernel.ld`: recursive
  initialization ordering/comments, linker heap symbol, and link assertion.
- `kernel/test/test_vmm.c`, `kernel/test/test_heap.c`,
  `kernel/test/test_kstack.c`, `kernel/test/test_runner.c`: real MMU integration.
- `tests/host/test_heap.c`, new focused host tests, `tests/Makefile`: replace the
  obsolete `_kernel_end` alias arrangement with the `HOST_TEST` backing-base
  seam; test production layout constants separately; cover deterministic
  boundaries and rollback.
- `Makefile`: fail clearly if `kernel.bin` exceeds 65536 bytes; do not modify
  `boot/stage2.S` in this story.

No `kernel/proc/` source should be created or changed in Story 3.5.

### Previous Story and Git Intelligence

- Story 3.1 established `pmm_alloc_frame()` returning physical addresses and
  zero on failure; `pmm_free_frame()` is caller-driven.
- Story 3.2 expanded the direct map to 16 MiB specifically so early page tables
  happened to be reachable through `P2V()`. This story removes that accidental
  dependency while keeping low VGA/E820/kernel access valid.
- Story 3.3 established recoverable page-fault testing and requires CR2 capture
  before other memory work.
- Story 3.4's review already fixed partial heap-expansion rollback. Preserve
  that recoverable cleanup pattern, keep boot-time initialization unpublished
  until complete, and add the hard upper bound.
- Relevant commits are `1777f87` (PMM), `e345118` (paging), `63d295c` (page
  faults), `0155e00` (heap), and `82f7aed` (Epic 3 retrospective). Recent story
  commits use `feat[story X.Y]: ...`.

### Final API and Design Decisions

- Each normal kernel stack currently uses one mapped 4 KiB page plus one lower
  4 KiB guard. This is intentionally sufficient for the simple Epic 4 kernel
  threads, not a permanent architectural limit. Kernel code must avoid
  recursion and large stack-local arrays. Slot geometry is derived from
  `KSTACK_GUARD_PAGES` and `KSTACK_PAGES`, yielding 1536 slots under the current
  policy. `struct kstack` carries `guard_base`, `stack_base`, and `top`;
  allocation/free use the exact integer-returning AC6 signatures.
- Region constants use exclusive-end semantics internally to prevent overflow
  and simplify adjacency checks.
- Direct-map conversion uses the exact checked AC2 functions and returns
  `-EINVAL` for null output or out-of-range input.
- `vmm_map_page()` returns `-EINVAL` for unaligned inputs and `-EEXIST` for an
  occupied destination. `vmm_unmap_page()` returns `int` and never frees frames.
- `vmm_init()` installs recursive PDE 1023 through the boot page directory,
  reloads CR3, and only then adopts recursive aliases.
- The dynamic region is reserved for explicit owned mappings and test pages;
  no general virtual-slot allocator is introduced in Story 3.5.

### References

- [Source: _bmad-output/planning-artifacts/sprint-change-proposal-2026-08-25.md#Issue-Summary]
- [Source: _bmad-output/planning-artifacts/sprint-change-proposal-2026-08-25.md#Detailed-Change-Proposals]
- [Source: _bmad-output/planning-artifacts/architecture.md#Kernel-Virtual-Address-Layout]
- [Source: _bmad-output/planning-artifacts/architecture.md#Testing-Strategy]
- [Source: _bmad-output/planning-artifacts/epics.md#Story-3.5-Kernel-Virtual-Address-Space-Layout--Dynamic-Mapping-Regions]
- [Source: _bmad-output/project-context.md#Memory-Rules]
- [Source: _bmad-output/project-context.md#Critical-Donts]
- [Source: _bmad-output/implementation-artifacts/3-2-paging-virtual-memory.md#Completion-Notes-List]
- [Source: _bmad-output/implementation-artifacts/3-3-page-fault-handler.md#Completion-Notes-List]
- [Source: _bmad-output/implementation-artifacts/3-4-kernel-heap-allocator.md#Completion-Notes-List]
- [Source: kernel/include/vmm.h#Address-Conversion-Macros]
- [Source: kernel/mm/vmm.c#get_or_create_page_table]
- [Source: kernel/mm/vmm.c#vmm_map_page]
- [Source: kernel/mm/heap.c#heap_expand]
- [Source: kernel/mm/heap.c#heap_init]
- [Source: kernel/init/entry.S#SET-UP-PAGING]
- [Source: boot/stage2.S#PM_STACK]
- [Source: scripts/kernel.ld#Address-constants]
- [Source: kernel/test/test_vmm.c#test_vmm]
- [Source: tests/host/test_heap.c#OOM-Tests]
- [Source: tests/Makefile#Kernel-Linked-Tests]

## Dev Agent Record

### Agent Model Used

{{agent_model_name_version}}

### Debug Log References

### Completion Notes List

- Ultimate context engine analysis completed - comprehensive developer guide
  created.

### File List
