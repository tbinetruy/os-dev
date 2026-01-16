# Epic 1 Retrospective: Boot & Build Foundation

**Date:** 2026-01-16
**Facilitator:** Bob (Scrum Master)
**Participants:** Alice (PO), Charlie (Senior Dev), Dana (QA), Elena (Junior Dev), Thomas (Project Lead)

---

## Epic Summary

| Metric | Value |
|--------|-------|
| Epic | 1: Boot & Build Foundation |
| Stories Completed | 6/6 (100%) |
| Status | Done |
| Technical Debt | Resolved |

### Stories Delivered

| Story | Title | Status |
|-------|-------|--------|
| 1.1 | Project Structure & Build System | Done |
| 1.2 | Stage 1 Bootloader (MBR) | Done |
| 1.3 | Stage 2 Bootloader & Protected Mode | Done |
| 1.4 | Kernel Entry & GDT Setup | Done |
| 1.5 | VGA Text Mode Driver | Done |
| 1.6 | Serial Debug & Panic Infrastructure | Done |

### Delivery Highlights

- Two-stage bootloader (MBR → Protected Mode → Kernel)
- A20 line enablement with verification
- GDT with kernel/user segment placeholders and TSS entry
- VGA text mode driver with scrolling and hardware cursor
- Serial debug output at 38400 baud
- printk() with LOG_ERROR/WARN/INFO/DEBUG levels
- Panic handler with full register dump
- GDB debugging infrastructure
- In-kernel test framework + host-side unit tests

### Requirements Satisfied

**Functional:** FR1-5, FR8, FR34, FR62-68
**Non-Functional:** NFR7, NFR8, NFR9, NFR11

---

## What Went Well

1. **Detailed story dev notes accelerated implementation**
   - Each story had implementation patterns, Intel SDM references, code examples
   - Story 1.4's GDT documentation (access byte breakdown) saved hours of debugging
   - Copy-paste ready code patterns reduced errors

2. **Test infrastructure evolved appropriately**
   - Started with in-kernel tests
   - Added host-side unit tests by Story 1.6
   - format.c extraction for testability was a smart refactor during review

3. **Incremental integration reduced risk**
   - Each story built on the previous one
   - Stage 1 → Stage 2 → Kernel → GDT → VGA → Serial
   - No big-bang integration moments
   - Every story left the system bootable

4. **All acceptance criteria met**
   - PRD requirements FR1-5, FR8, FR34, FR62-68 satisfied
   - Debug infrastructure NFRs (7, 8, 9, 11) covered
   - Clean traceability from requirements to implementation

5. **Code reviews caught real issues**
   - INT32_MIN edge case in printk
   - Inconsistent newline handling (serial vs VGA)
   - Typo "Interrup" in serial.h
   - Review process is working as intended

---

## What Could Be Improved

1. **Initial test coverage was smoke-test quality**
   - Tests verified functions didn't crash, not output correctness
   - Had to refactor in Story 1.6 review to extract pure functions
   - Should design for testability from the start

2. **VGA test failure carried forward (now resolved)**
   - Pre-existing test failure mentioned in Story 1.6 review
   - Fixed by Thomas after retrospective started
   - Lesson: Don't carry known failures into next epic

3. **Code duplication crept in**
   - `print_num()` existed in both main.c and test_runner.c
   - Documented as intentional, but still a code smell
   - printk() eventually replaced it

4. **Story 1.3 complexity underestimated**
   - A20 + protected mode + kernel loading in one story
   - Spent longer in 'review' status than other stories
   - Consider splitting complex stories earlier

5. **No automated CI**
   - Tests run manually with `make test` and `make host-test`
   - Easy to forget to run tests
   - Future consideration for automation

---

## Key Learnings

| # | Learning | Evidence | Impact on Epic 2 |
|---|----------|----------|------------------|
| 1 | Detailed story dev notes accelerate implementation | GDT/VGA/Serial patterns were copy-paste ready | Continue pattern for IDT, PIC, keyboard stories |
| 2 | Extract pure functions for testability | format.c refactor enabled 18 host-side tests | Plan test extraction upfront for interrupt handlers |
| 3 | Code review catches real bugs | 5 issues found and fixed in Story 1.6 alone | Maintain review discipline |
| 4 | Assembly documentation is critical | gdt_flush.S, entry.S comments prevented confusion | IDT stubs and context save will need same rigor |
| 5 | Incremental integration reduces risk | No big-bang moments, each story bootable | Timer before keyboard, exceptions before IRQs |

---

## Recommendations for Epic 2

### 1. Fix known issues before starting (DONE)
- ~~VGA test failure should be resolved first~~
- Completed by Thomas before retrospective closed
- Exception handlers will use VGA output

### 2. Design for testability from the start
- PIC/PIT register values can be tested on host
- Scancode translation table is pure logic - extract and test it
- Don't wait for code review to refactor for testability

### 3. Story 2.1 (IDT) should include exception test triggers
- Intentional divide-by-zero, invalid opcode tests
- Verify handlers fire and output correctly
- Test both serial and VGA output paths

### 4. Document interrupt stack frame layout explicitly
- Story dev notes should diagram: error code, EIP, CS, EFLAGS
- Critical for context save/restore correctness
- Reference Intel SDM Vol 3, Section 6.12

### 5. Consider splitting Story 2.2 if PIC + Timer is too much
- Story 1.3 showed complexity creep risk
- PIC init could be separate from timer setup
- Monitor during story creation

---

## Action Items

| Action | Owner | Status |
|--------|-------|--------|
| Fix VGA test failure | Thomas | Done |
| Apply learnings to Epic 2 story preparation | SM | Pending |
| Include testability design in story dev notes | SM/Dev | Pending |

---

## Next Epic

**Epic 2: Interrupt Handling & Device I/O**

| Story | Title | Dependencies on Epic 1 |
|-------|-------|------------------------|
| 2.1 | IDT Setup & Exception Handlers | GDT (1.4), VGA (1.5), Serial/Panic (1.6) |
| 2.2 | PIC & Timer Driver | IDT (2.1), Serial (1.6) |
| 2.3 | Keyboard Driver | PIC (2.2), VGA (1.5) |

---

## Sign-off

Retrospective completed and findings documented.

- Sprint status updated: `epic-1-retrospective: done`
- Ready to proceed with Epic 2
