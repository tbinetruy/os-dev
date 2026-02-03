# Epic 2 Retrospective: Interrupt Handling & Device I/O

**Date:** 2026-02-03
**Facilitator:** Bob (Scrum Master)
**Participants:** Alice (PO), Charlie (Senior Dev), Dana (QA), Elena (Junior Dev), Thomas (Project Lead)

---

## Epic Summary

| Metric | Value |
|--------|-------|
| Epic | 2: Interrupt Handling & Device I/O |
| Stories Completed | 3/3 (100%) |
| Status | Done |
| Tests | 65 → 370 (5.7x growth) |
| Code Review Issues | 7 total (all fixed) |

### Stories Delivered

| Story | Title | Status | Tests Added |
|-------|-------|--------|-------------|
| 2.1 | IDT Setup & Exception Handlers | Done | 11 |
| 2.2 | PIC & Timer Driver | Done | 32 |
| 2.3 | Keyboard Driver | Done | 273 |

### Delivery Highlights

- IDT with 256 entries, exception handlers 0-31 with diagnostic output
- Page fault handler reads CR2 and parses error code (NFR10 satisfied)
- PIC remapped: IRQ 0-7 → INT 32-39, IRQ 8-15 → INT 40-47
- Timer running at 100Hz (10ms tick interval)
- Keyboard driver with circular buffer and scancode set 1 translation
- Keyboard echo loop - OS is now interactive

### Requirements Satisfied

**Functional:** FR6, FR31-33, FR36
**Non-Functional:** NFR10 (page fault diagnostics)

---

## What Went Well

1. **Code reviews caught real bugs**
   - `keyboard_getchar()` interrupt safety issue (could enable interrupts unexpectedly)
   - Breakpoint exception needed trap gate, not interrupt gate
   - Missing cascade line (IRQ 2) unmask for slave PIC
   - Extended scancode handling for arrow keys

2. **Testability-first design paid off**
   - Scancode translation table extracted as pure logic
   - 273 keyboard tests possible because of clean separation
   - Epic 1 lesson applied successfully

3. **Story dev notes accelerated implementation**
   - PIC ICW1-4 initialization sequence fully documented
   - Intel SDM references included
   - ISR stub patterns with assembly examples

4. **All Epic 1 action items completed**
   - VGA test failure fixed before Epic 2 started
   - Learnings applied to story preparation
   - Testability design included in dev notes

5. **OS is now interactive**
   - Keyboard input works end-to-end
   - Foundation for shell (Epic 8) is ready

---

## What Could Be Improved

1. **Kernel size caused silent boot failure**
   - Kernel grew past 16KB, bootloader couldn't load it
   - No error message - just didn't boot
   - Had to bump KERNEL_SECTORS from 32 to 64
   - Currently at ~24KB, limit is 32KB

2. **Extended scancodes caught only in manual testing**
   - Arrow keys produced numpad characters
   - Automated tests didn't cover this path
   - Manual QEMU testing was essential

3. **Test count inflation**
   - 273 keyboard tests mostly from scancode table (128 entries × 2)
   - Not a sustainable growth rate
   - Should focus on test quality, not quantity

---

## Key Learnings

| # | Learning | Evidence | Impact on Epic 3 |
|---|----------|----------|------------------|
| 1 | Code reviews catch real bugs | 7 issues found and fixed across 3 stories | Maintain review discipline |
| 2 | Test quality > test quantity | Scancode tests inflated numbers | Focus on edge cases and failure modes |
| 3 | Silent failures need visibility | Bootloader limit had no warning | Monitor kernel size proactively |
| 4 | Manual testing still essential | Extended scancode bug only caught in QEMU | Include manual verification steps |
| 5 | Detailed dev notes accelerate work | PIC/IDT patterns were copy-paste ready | Continue for page tables, memory maps |

---

## Epic 1 Retrospective Follow-Through

| Action Item | Status | Evidence |
|-------------|--------|----------|
| Fix VGA test failure | ✅ Done | Fixed before Epic 2 started |
| Apply learnings to story prep | ✅ Done | All 3 stories had detailed dev notes |
| Include testability design | ✅ Done | Scancode table extracted, 273 tests |

**Result:** 3/3 action items completed (100%)

---

## Action Items

### Process Improvements

1. **Focus on test quality over quantity**
   - Owner: Dana (QA) + Dev team
   - Success criteria: Tests target edge cases and real failure modes

2. **Continue code review discipline**
   - Owner: All
   - Success criteria: Every story gets adversarial review

### Technical Monitoring

3. **Monitor kernel size during Epic 3**
   - Owner: Charlie (Senior Dev)
   - Trigger: If build approaches 30KB, bump KERNEL_SECTORS
   - Success criteria: No surprise boot failures

### Documentation

4. **Maintain detailed dev notes with diagrams**
   - Owner: SM (story prep)
   - Success criteria: Page table layouts, memory maps in story files

---

## Team Agreements

- Test *meaningfulness* matters more than test *count*
- Silent failures need explicit warnings or checks where possible
- Manual testing catches bugs automated tests miss
- Monitor and react on kernel size (don't over-engineer preemptively)

---

## Next Epic

**Epic 3: Memory Management**

| Story | Title | Dependencies on Epic 2 |
|-------|-------|------------------------|
| 3.1 | Physical Memory Manager | Page fault handler (2.1) for testing |
| 3.2 | Paging & Virtual Memory | Exception handling (2.1) |
| 3.3 | Page Fault Handler | IDT infrastructure (2.1) |
| 3.4 | Kernel Heap Allocator | PMM (3.1), Paging (3.2) |

**Preparation Notes:**
- Monitor kernel size (currently ~24KB, limit 32KB)
- Include page table layout diagrams in dev notes
- Focus on edge case testing for allocators

---

## Readiness Assessment

| Area | Status |
|------|--------|
| Testing & Quality | ✅ 370 tests pass |
| Code Reviews | ✅ All stories reviewed |
| Technical Health | ✅ No known bugs |
| Manual Verification | ✅ Keyboard echo works in QEMU |

**Epic 2 Status:** Complete and ready for Epic 3

---

## Sign-off

Retrospective completed and findings documented.

- Sprint status updated: `epic-2: done`, `epic-2-retrospective: done`
- Ready to proceed with Epic 3: Memory Management
