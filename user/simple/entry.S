#if LAB >= 4
#include <inc/asm.h>
#include <inc/mmu.h>

.text
.globl ___start
___start:
	cmpl $USTACKTOP, %esp
	jne args_exist
	pushl $0
	pushl $0
args_exist:
	call ___main
	call _umain
	call _exit
1:      jmp 1b
	
#if SOL >= 4

// On entry there a trap frame is on the stack: 
//   [ 5 spare words ]
//   trap time eip
//   trap time eflags
//   trap time esp
//   trap time errcode
//   trap time va  <-- %esp 
.globl _asm_pgfault_handler
_asm_pgfault_handler:
// caller saves
	movl %eax,20(%esp)
	movl %edx,24(%esp)
	movl %ecx,28(%esp)
	call _pgfault_handler
// copy trap time eip and eflags to trap time stack
	
	movl 8(%esp),%eax    // EAX <- trap time ESP
	movl 16(%esp),%ecx   // ECX <- trap time EIP
	movl %ecx, -4(%eax)  // *(trap time esp - 4) = trap time eip
	movl 12(%esp),%ecx   // EBX <- trap time EFLAGS
	movl %ecx, -8(%eax)  // *(trap time esp - 8) = trap time eflags
// restore caller saves

	movl 20(%esp),%eax
	movl 24(%esp),%edx
	movl 28(%esp),%ecx
// load esp with trap time stack - 8

	subl $8,8(%esp)
	movl 8(%esp),%esp
// eflags and eip are waiting us there
	popf
	ret
#endif /* SOL >= 4 */

#endif /* LAB >= 4 */
