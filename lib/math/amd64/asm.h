
#define CNAME(x)	x

#define ENTRY(x)	.text; .p2align 4,0x90; .globl x; \
			.type x,@function; x:

#define END(x)		.size x,.-x

