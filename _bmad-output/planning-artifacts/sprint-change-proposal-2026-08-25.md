---
status: approved
workflow: correct-course
date: '2026-08-25'
project_name: 'os-dev'
user_name: 'Thomas'
triggering_story: 'Story 4.1: Task Structure & Thread Creation'
scope_classification: moderate
approval_status: approved
approved_by: Thomas
approved_at: '2026-08-25'
---

# Sprint Change Proposal: Kernel Virtual Memory Layout

## 1. Issue Summary

During review of the in-progress Story 4.1 implementation, kernel thread
stack allocation exposed a missing architectural contract in Epic 3.
Thread stacks were being designed around `P2V(pmm_alloc_frame())`, while
the kernel heap currently grows upward from `_kernel_end`. Both approaches
can select virtual addresses in the same higher-half area without any
shared ownership rule.

The current paging setup directly maps only the first 16MiB of physical
memory at `0xC0000000-0xC0FFFFFF`. That makes `P2V()` safe only for that
direct-map window. It is not a general "make this PMM frame usable as a
kernel virtual address" operation. Once PMM returns frames outside the low
direct map, using `P2V()` to access those frames can fault or corrupt an
unrelated virtual-memory region.

The same design gap affects page-table management. New page tables are
currently accessed with `P2V(pt_phys)`, which only works while page-table
frames happen to come from low direct-mapped physical memory.

## 2. Impact Analysis

Epic impact:

- Epic 3 remains the correct home for the fix because the issue is a
  virtual-memory ownership problem, not a process-management problem.
- Epic 3 needs a new Story 3.5 after heap allocation and before kernel
  threads.
- Epic 4 must depend on Story 3.5, because thread creation needs a
  reliable kernel stack allocator and a clear answer for PID 0's bootstrap
  stack.

Story impact:

- Story 3.2 remains historically valid as the first paging milestone, but
  it was incomplete as a durable layout contract.
- Story 3.4 remains valid as a heap allocator story, but its heap start and
  growth rules need explicit bounds.
- Story 4.1 should not proceed until kernel stacks have a dedicated virtual
  region and guard-page behavior.

Artifact impact:

- Architecture document: updated with explicit kernel virtual address
  regions and recursive page-table mapping.
- Project context: updated with critical implementation rules for future
  agents.
- Epics document: updated with Story 3.5 and a dependency from Story 4.1.
- PRD: no MVP change required. Existing FR7, FR11, FR12, FR14, and FR16
  already imply the needed capability.

Technical impact:

- Introduces constants for kernel virtual regions.
- Moves heap start to a fixed region, with upper-bound checks.
- Adds recursive page-directory mapping at PDE 1023.
- Changes VMM internals so page tables are accessed through the recursive
  window rather than through arbitrary direct-map conversion.
- Makes accidental remapping a failure unless the caller uses an explicit
  replacement path.
- Adds a kernel-stack virtual slot allocator with guard pages.
- Documents PID 0 as using the inherited boot stack at physical `0x90000`,
  virtual `0xC0090000`, unless a later story explicitly migrates it.

## 3. Recommended Approach

Recommended path: Direct adjustment within the current epic plan.

Add Story 3.5: Kernel Virtual Address Space Layout & Dynamic Mapping Regions
between Story 3.4 and Story 4.1. This keeps the project sequence coherent:
paging, page faults, heap, then a corrected virtual-memory contract, then
threads and scheduling.

Rollback is not recommended. Existing Epic 3 work is useful and forms the
base for the fix. The in-progress Story 4.1 work can stay parked on its
checkpoint branch until Story 3.5 lands, then be rebased or adjusted.

MVP review is not required. This is a foundational correctness fix within
the original learning goal, not a scope expansion beyond the OS roadmap.

Effort estimate: Medium.

Risk level: Medium. The riskiest parts are page-table self-mapping and
heap relocation because mistakes show up as early boot failures or page
faults. The risk is acceptable if implemented with narrow tests and QEMU
boot verification before resuming Story 4.1.

## 4. Detailed Change Proposals

Architecture update:

- Add explicit kernel virtual regions:
  `0xC0000000-0xC0FFFFFF` low direct map,
  `0xC1000000-0xDFFFFFFF` kernel heap,
  `0xE0000000-0xEFFFFFFF` dynamic kernel mappings,
  `0xF0000000-0xFEFFFFFF` reserved expansion space,
  `0xFF000000-0xFFBFFFFF` kernel stack slots, and
  `0xFFC00000-0xFFFFFFFF` recursive page tables.
- Define `P2V()`/`V2P()` as bounded direct-map helpers, not arbitrary
  physical/virtual translation.
- Document PID 0's inherited boot stack separately from normal kernel
  thread stacks.
- Require dynamic mapping operations to roll back virtual slots, physical
  frames, and page mappings on failure.

Project context update:

- Add the region map to the critical Memory Rules section.
- Warn future agents that direct-map conversion only applies to the
  bounded low physical mapping.
- Require recursive page-table access after Story 3.5.

Epics update:

- Add Story 3.5 under Epic 3.
- Require acceptance criteria for region ownership, heap bounds, direct-map
  guardrails, recursive page-directory mapping, kernel stack slots with
  guard pages, explicit remap behavior, PID 0 bootstrap stack handling, and
  tests.
- Add a Story 4.1 dependency stating thread stacks must come from the
  dedicated stack region, not from `P2V(pmm_alloc_frame())` or heap memory.

## 5. Implementation Handoff

Scope classification: Moderate.

Handoff recommendation:

- Architect/PM role: approve this corrective Story 3.5 insertion and the
  updated artifact wording.
- Developer role: implement Story 3.5 before resuming Story 4.1.
- Reviewer/test role: review Story 3.5 against the new layout contract
  before process/thread work continues.

Success criteria:

- No kernel virtual regions overlap.
- Heap growth cannot enter stack, dynamic mapping, or recursive page-table
  regions.
- Kernel stacks are allocated in dedicated virtual slots with guard pages.
- PID 0's bootstrap stack ownership is explicit.
- Page-table access no longer depends on new page-table frames living
  inside the low direct map.
- Existing mappings are not silently replaced by normal map calls.
- In-kernel and host tests cover boundaries, guard pages, recursive
  mapping addresses, and failure rollback.

## Checklist Summary

1. Trigger and context: Done.
2. Epic impact: Done.
3. Artifact conflict analysis: Done.
4. Path forward: Done.
5. Sprint change proposal: Done as draft.
6. Final approval and route: Approved by Thomas on 2026-08-25; route to
   Story 3.5 creation and independent quality review.
