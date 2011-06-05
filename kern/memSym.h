#ifndef MIK_MEM_H
#define MIK_MEM_H


// An x86-64 address is split into six fields:
//	- the sign extension area in bits 63-48.
//	- four 9-bit page table index fields in bits 47-12.
//	- the page offset field in bits 11-0.
// The following macros aid in manipulating these addresses.

// Bit-shifts for each field
#define P4SHIFT		39
#define P3SHIFT		30
#define P2SHIFT		21
#define P1SHIFT		12

// Cover sizes for each mapping level
#define P4SIZE		(1L << P4SHIFT)
#define P3SIZE		(1L << P3SHIFT)
#define P2SIZE		(1L << P2SHIFT)
#define P1SIZE		(1L << P1SHIFT)

// Page table index extractors
#define P4X(va)		((((u64_t)(va)) >> P4SHIFT) & 0x1ffL)
#define P3X(va)		((((u64_t)(va)) >> P3SHIFT) & 0x1ffL)
#define P2X(va)		((((u64_t)(va)) >> P2SHIFT) & 0x1ffL)
#define P1X(va)		((((u64_t)(va)) >> P1SHIFT) & 0x1ffL)

// Offset extractors
#define P4OFF(va)	(((u64_t)(va)) & (P4SIZE - 1))
#define P3OFF(va)	(((u64_t)(va)) & (P3SIZE - 1))
#define P2OFF(va)	(((u64_t)(va)) & (P2SIZE - 1))
#define P1OFF(va)	(((u64_t)(va)) & (P1SIZE - 1))

// Truncate (downward) and round (upward) macros
#define P1TRUNC(va)	((u64_t)(va) & ~(P1SIZE - 1))
#define P2TRUNC(va)	((u64_t)(va) & ~(P2SIZE - 1))
#define P3TRUNC(va)	((u64_t)(va) & ~(P3SIZE - 1))
#define P4TRUNC(va)	((u64_t)(va) & ~(P4SIZE - 1))

#define P1ROUND(va)	P1TRUNC((u64_t)(va) + (P1SIZE - 1))
#define P2ROUND(va)	P2TRUNC((u64_t)(va) + (P2SIZE - 1))
#define P3ROUND(va)	P3TRUNC((u64_t)(va) + (P3SIZE - 1))
#define P4ROUND(va)	P4TRUNC((u64_t)(va) + (P4SIZE - 1))


// Page Table Entry (PTE) flags
#define PTE_P		0x0000000000000001	// Present
#define PTE_W		0x0000000000000002	// Writeable
#define PTE_U		0x0000000000000004	// User
#define PTE_PWT		0x0000000000000008	// Write-Through
#define PTE_PCD		0x0000000000000010	// Cache-Disable
#define PTE_A		0x0000000000000020	// Accessed
#define PTE_D		0x0000000000000040	// Dirty
#define PTE_PS		0x0000000000000080	// Page Size
#define PTE_G		0x0000000000000100	// Global
#define PTE_AVL		0x0000000000000e00	// Available for software use
#define PTE_NX		0x8000000000000000	// No Execute

// Physical address portion of page table entry
#define PTE_ADDR(pte)	((u64_t)(pte) & 0x000ffffffffff000)


#endif // MIK_MEM_H
