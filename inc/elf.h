#ifndef _ELF_H_
#define _ELF_H_ 1

#define ELF_MAGIC 0x464C457F	/* little endian */

struct Elf {
	u_int e_magic;
	u_char e_elf[12];
	u_short e_type;
	u_short e_machine;
	u_int e_version;
	u_int e_entry;
	u_int e_phoff;
	u_int e_shoff;
	u_int e_flags;
	u_short e_ehsize;
	u_short e_phentsize;
	u_short e_phnum;
	u_short e_shentsize;
	u_short e_shnum;
	u_short e_shstrndx;
};

#define ELF_PROG_LOAD 1
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4

struct Proghdr {
	u_int p_type;
	u_int p_offset;
	u_int p_va;
	u_int p_pa;
	u_int p_filesz;
	u_int p_memsz;
	u_int p_flags;
	u_int p_align;
};

#endif
