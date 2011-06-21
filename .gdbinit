set architecture i386:x86-64

echo + target remote localhost:26022\n
target remote localhost:26022

# If this fails, it's probably because your GDB doesn't support ELF.
# Look at the tools page at
#  http://pdos.csail.mit.edu/6.828/2009/tools.html
# for instructions on building GDB with ELF support.
echo + symbol-file obj/kern/kernel\n
symbol-file obj/kern/kernel
