#///LAB3	
#include <inc/mmu.h>

.global _div0_start
_div0_start:
.space 0x20  # pretend there is an a.out header here	
xorl %eax, %eax
divl %eax, %eax
.global _div0_end
_div0_end:

.global _brkpt_start
_brkpt_start:
.space 0x20
int $3
.global _brkpt_end
_brkpt_end:
	
.global _badds_start
_badds_start:
.space 0x20  # pretend there is an a.out header here	
xorl %eax, %eax
movw %ax, %ds
.global _badds_end
_badds_end:

.global _pgfault_rd_nopage_start
_pgfault_rd_nopage_start:
.space 0x20  # pretend there is an a.out header here	
movl 0x1000, %eax
.global _pgfault_rd_nopage_end
_pgfault_rd_nopage_end:

.global _pgfault_rd_noperms_start
_pgfault_rd_noperms_start:
.space 0x20  # pretend there is an a.out header here	
movl 0xffffffff, %eax
.global _pgfault_rd_noperms_end
_pgfault_rd_noperms_end:

.global _pgfault_wr_nopage_start
_pgfault_wr_nopage_start:
.space 0x20  # pretend there is an a.out header here	
movl %eax, 0x1000
.global _pgfault_wr_nopage_end
_pgfault_wr_nopage_end:

.global _pgfault_wr_noperms_start
_pgfault_wr_noperms_start:
.space 0x20  # pretend there is an a.out header here	
movl %eax, 0xffffffff
.global _pgfault_wr_noperms_end
_pgfault_wr_noperms_end:

# test case for ex4 -- 
#   check that registers are saved/restored correctly		

.global _alice_start
_alice_start:
.space 0x20  # pretend there is an a.out header here	
	movl	$1,%edi	
	movl	$2,%esi	
	movl	$3,%ebp
	movl	$4,%ebx
	movl	$5,%edx
	movl	$6,%ecx
	movl	$7,%eax
	movl	$8,%esp
alice_top:	
	cmpl	$1,%edi
	jne	alice_error
	cmpl	$2,%esi	
	jne	alice_error
	cmpl	$3,%ebp
	jne	alice_error
	cmpl	$4,%ebx
	jne	alice_error
	cmpl	$5,%edx
	jne	alice_error
	cmpl	$6,%ecx
	jne	alice_error
	cmpl	$7,%eax
	jne	alice_error
	cmpl	$8,%esp
	jne	alice_error
	
	movw	%cs,%ax
	cmpw	$(GD_UT|3),%ax
	jne	alice_error
	movw	%es,%ax
	cmpw	$(GD_UD|3),%ax
	jne	alice_error
	movw	%ds,%ax
	cmpw	$(GD_UD|3),%ax
	jne	alice_error
	movw	%ss,%ax
	cmpw	$(GD_UD|3),%ax
	jne	alice_error
	movl	$7,%eax

	jmp	alice_top
alice_error:
	xorl %eax, %eax
	divl %eax, %eax
.global _alice_end
_alice_end:

# test case for ex4 -- 
#   check that registers are saved/restored correctly		

.global _bob_start
_bob_start:
.space 0x20  # pretend there is an a.out header here	
	movl	$11,%edi	
	movl	$12,%esi	
	movl	$13,%ebp
	movl	$14,%ebx
	movl	$15,%edx
	movl	$16,%ecx
	movl	$17,%eax
	movl	$18,%esp
bob_top:	
	cmpl	$11,%edi
	jne	bob_error
	cmpl	$12,%esi	
	jne	bob_error
	cmpl	$13,%ebp
	jne	bob_error
	cmpl	$14,%ebx
	jne	bob_error
	cmpl	$15,%edx
	jne	bob_error
	cmpl	$16,%ecx
	jne	bob_error
	cmpl	$17,%eax
	jne	bob_error
	cmpl	$18,%esp
	jne	bob_error

	movw	%cs,%ax
	cmpw	$(GD_UT|3),%ax
	jne	bob_error
	movw	%es,%ax
	cmpw	$(GD_UD|3),%ax
	jne	bob_error
	movw	%ds,%ax
	cmpw	$(GD_UD|3),%ax
	jne	bob_error
	movw	%ss,%ax
	cmpw	$(GD_UD|3),%ax
	jne	bob_error
	movl	$17,%eax

	jmp	bob_top
bob_error:
	xorl %eax, %eax
	divl %eax, %eax
.global _bob_end
_bob_end:
	

.global _spin_start
_spin_start:
.space 0x20  # pretend there is an a.out header here	
1:	jmp 1b		
.global _spin_end
_spin_end:
#///END
