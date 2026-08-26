# Epic 3 Retrospective: Memory Management

**Date:** 2026-02-10
**Facilitator:** Bob (Scrum Master)
**Participants:** Alice (PO), Charlie (Senior Dev), Dana (QA), Elena (Junior Dev), Thomas (Project Lead)

---

## Epic Summary

| Metric | Value |
|--------|-------|
| Epic | 3: Memory Management |
| Stories Completed | 4/4 (100%) |
| Status | Done |
| Tests | 382 → 559 (177 new, 46% growth) |
| Code Review Issues | Found in every story (all fixed) |
| Blockers | 2 (boot page table size, kernel binary size) |
| Technical Debt Items | 3 (spinlock, interrupt safety, user-mode faults) |

### Stories Delivered

| Story | Title | Status | Tests Added | Agent Model |
|-------|-------|--------|-------------|-------------|
| 3.1 | Physical Memory Manager | Done | 11 (382 total) | Opus 4.5 |
| 3.2 | Paging & Virtual Memory | Done | 12 (394 total) | Opus 4.5 |
| 3.3 | Page Fault Handler | Done | 14 (408 total) | Opus 4.6 |
| 3.4 | Kernel Heap Allocator | Done | 151 (559 total) | Opus 4.6 |

### Delivery Highlights

- Bitmap-based physical memory manager parsing E820 memory map
- Higher-half kernel at 0xC0000000 with two-level i386 paging
- Page fault handler with diagnostic output (CR2 capture, error code parsing)
- Kernel heap allocator with first-fit, block splitting, and coalescing
- kmalloc/kfree available for all future kernel subsystems

### Requirements Satisfied

**Functional:** FR7, FR9-14
**Non-Functional:** NFR10 (page fault diagnostics), NFR18 (no memory leaks, verifiable)

---

## What Went Well

1. **Code reviews caught real bugs in every story**
   - Story 3.1: Missing exhaustion test, reserved frame protection test
   - Story 3.3: Register dump restoration, header declarations, write fault test
   - Story 3.4: heap_expand partial failure cleanup, try_alloc helper extraction

2. **Testing innovations**
   - Bitmap ops extracted to `kernel/lib/bitmap.c` for host testing (Story 3.1)
   - Recoverable page fault test hook `pf_set_test_hook()` (Story 3.3)
   - Host tests for heap OOM/rollback behaviors (Story 3.4)

3. **Dev notes quality was excellent**
   - Memory layout diagrams in every story
   - Page table architecture (two-level, PDE/PTE format) fully documented
   - Block header layouts and coalescing algorithms diagrammed
   - Intel SDM references included

4. **Epic 2 retro follow-through: 3/4 completed**
   - Test quality focus applied ✅
   - Code review discipline maintained ✅
   - Detailed dev notes with diagrams ✅
   - Kernel size monitoring: partial ⏳ (reactive, not proactive)

5. **Higher-half kernel transition succeeded**
   - Complete rewrite of entry.S with 16MB paging
   - 13 files touched in Story 3.2, all 394 tests passing after
   - Kernel crossed a significant complexity threshold

---

## What Could Be Improved

1. **Bootloader size limit hit again (second time)**
   - Kernel exceeded 32KB in Story 3.3
   - KERNEL_SECTORS bumped 64→128 (previously 32→64 in Epic 2)
   - Same symptom: silent boot failure, no error message
   - Monitoring commitment from Epic 2 retro was not proactive

2. **Story 3.2 blast radius underestimated**
   - Dev notes planned ~5 files, actual was 13 files modified
   - Boot page tables planned for 4MB, needed 16MB
   - PAGE_MASK redefinition conflict between vmm.h and pmm.h
   - VGA, PMM, and test updates discovered mid-implementation

3. **Host tests introduced at review time for Story 3.4**
   - OOM/rollback host tests were necessary but came during review
   - Boilerplate cost is real but pays off quickly
   - Should have been part of the development plan from the start

---

## Key Learnings

| # | Learning | Evidence | Impact on Epic 4 |
|---|----------|----------|-------------------|
| 1 | Host tests should be planned during dev, not review | Story 3.4 host tests added at review time | Include host test tasks in story task lists |
| 2 | Big-bang stories need full file impact analysis | Story 3.2 planned 5 files, touched 13 | Story 4.2 (context switch) prep must list ALL affected files |
| 3 | Kernel size needs permanent visibility | Hit size limit twice across two epics | Add size to build output, bump KERNEL_SECTORS generously |
| 4 | Code review catches real bugs every time | Bugs found in all 4 stories | Continue adversarial review discipline |
| 5 | Testing innovations compound | Bitmap extraction, fault hooks, host tests | Apply same thinking to scheduler and task state testing |

---

## Epic 2 Retrospective Follow-Through

| Action Item | Status | Evidence |
|-------------|--------|----------|
| Focus on test quality over quantity | ✅ Done | Tests target edge cases: exhaustion, double-free, coalescing, fault capture |
| Continue code review discipline | ✅ Done | All 4 stories reviewed, real bugs found in every review |
| Monitor kernel size during Epic 3 | ⏳ Partial | Hit 32KB limit in Story 3.3, reacted by bumping 64→128. Reactive, not proactive. |
| Maintain detailed dev notes with diagrams | ✅ Done | Page table layouts, block headers, memory maps in every story |

**Result:** 3/4 action items completed, 1 partially applied (100%)

---

## Action Items

### Process Improvements

1. **Include host tests in development plan, not as review afterthought**
   - Owner: Bob (SM — story preparation)
   - Success criteria: Each story explicitly identifies host-testable logic and includes host test tasks

2. **Map full file impact for high-blast-radius stories**
   - Owner: Bob (SM — story preparation)
   - Success criteria: Stories with all-or-nothing changes list ALL files expected to change

### Technical Monitoring

3. **Bump KERNEL_SECTORS to 512 and add kernel size to build output**
   - Owner: Charlie (Senior Dev)
   - Success criteria: `make` prints kernel binary size; KERNEL_SECTORS high enough for all remaining epics

### Code Review

4. **Continue adversarial code review discipline**
   - Owner: All
   - Success criteria: Every story reviewed; every review finds something worth discussing

---

## Team Agreements

- Host tests are first-class citizens, not afterthoughts
- Big-bang stories need proportionally bigger preparation
- Kernel size is visible, not manually monitored
- Code review discipline is non-negotiable

---

## Next Epic

**Epic 4: Kernel Threads & Scheduling**

| Story | Title | Dependencies on Epic 3 |
|-------|-------|------------------------|
| 4.1 | Task Structure & Thread Creation | kmalloc for task_struct and kernel stacks |
| 4.2 | Context Switch | Stable paging, assembly switch.S |
| 4.3 | Round-Robin Scheduler | Timer interrupt (Epic 2) + context switch |

**Preparation Tasks:**

- [ ] Bump KERNEL_SECTORS to 512 in `boot/stage2.S`
- [ ] Add kernel size print to Makefile build output
- [ ] Story 4.1: include spinlock/interrupt-disable primitive design (cli/sti with flags save)
- [ ] Story 4.2: full stack layout diagrams, register save order, all affected files listed
- [ ] Story 4.3: identify all critical sections needing lock protection (heap, scheduler queue)
- [ ] Identify host-testable logic for each Epic 4 story

**Key Decision:** Spinlock primitives (cli/sti wrappers) defined in Story 4.1, applied to heap and scheduler in Story 4.3.

---

## Readiness Assessment

| Area | Status |
|------|--------|
| Testing & Quality | ✅ 559 tests pass |
| Code Reviews | ✅ All stories reviewed |
| Technical Health | ✅ No known bugs |
| Codebase Stability | ✅ Solid foundation for Epic 4 |
| Technical Debt | ⚠️ 3 TODOs (spinlock, interrupt safety, user-mode faults) — addressed in Epic 4 plan |

**Epic 3 Status:** Complete and ready for Epic 4

---

## Sign-off

Retrospective completed and findings documented.

- Sprint status updated: `epic-3: done`, `epic-3-retrospective: done`
- Ready to proceed with Epic 4: Kernel Threads & Scheduling

---

## Corrective Addendum — 2026-08-26

This addendum preserves the original retrospective as an accurate record of
what was understood on 2026-02-10. Epic 3 was subsequently reopened after the
Story 4.1 review exposed a missing virtual-memory ownership design.

### Trigger and Evidence

The in-progress thread-creation work derived a stack virtual address with
`P2V(pmm_alloc_frame())`. That is unsafe because the current direct map covers
only physical `0-16 MiB`, while PMM may return frames above it. It also gives
kernel stacks virtual addresses in the same upward-growing area previously used
by the heap, so later heap expansion can replace a live stack mapping.

The review also established that:

- kernel virtual space had no explicit regions or owners;
- the heap had no fixed virtual range or upper bound;
- page-table access depended on new PMM frames happening to be direct-mapped;
- ordinary page mapping could silently replace an existing mapping; and
- the PID 0 bootstrap stack has an initial top at virtual `0xC0090000`, but no
  enforced lower bound or guard page.

### Corrective Decision

Add Story 3.5, **Kernel Virtual Address Space Layout & Dynamic Mapping
Regions**, to Epic 3 and complete it before resuming Story 4.1. Story 3.5 owns:

- explicit, non-overlapping kernel virtual regions;
- bounded direct-map conversion helpers;
- recursive current-page-table access;
- a fixed and bounded kernel heap;
- non-overwriting map semantics;
- dedicated guarded kernel-stack slots; and
- boundary, failure, rollback, and boot-compatibility tests.

Story 4.1 remains valid but depends on the guarded stack allocator instead of
choosing virtual stack addresses itself. Migration of PID 0 from its unbounded
bootstrap stack is tracked separately in `BACKLOG.md`.

### Correction to Earlier Action Items

The earlier recommendation to raise `KERNEL_SECTORS` directly to 512 was
incomplete. Stage 2 copies only 64 KiB and its current disk-reading assumptions
do not establish support for a 512-sector kernel. Story 3.5 therefore adds a
clear 64 KiB build-time limit; robust larger-kernel loading requires a separate
bootloader correction rather than another constant-only increase.

### Updated Readiness

| Area | Updated Status |
|------|----------------|
| Stories 3.1-3.4 | Done; historical implementations retained |
| Story 3.5 | Ready for development |
| Epic 3 | Reopened and in progress |
| Story 4.1 | Paused in a preserved WIP branch until Story 3.5 is done |
| Epic 3 retrospective | Original retrospective done; this corrective addendum records the reopened scope |

**Revised conclusion:** Epic 3 is not ready to hand off to scheduling until
Story 3.5 is implemented, tested, and independently reviewed.
