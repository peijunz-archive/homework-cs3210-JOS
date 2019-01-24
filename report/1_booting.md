# Lab 1: Booting a PC
Original Link: https://pdos.csail.mit.edu/6.828/2018/labs/lab1/

## E1. ASM
+ CSAPP?
+ `in` and `out` instructions

## E2E3E6.  GDB
+ `si` as step instruction
+ More GDB commands in [lab guide](https://pdos.csail.mit.edu/6.828/2018/labguide.html)
+ Examine with `x/Nx ADDR`

## E4. C
+ Casting
+ Endianness
+ Stack Memory

## E5. objdump
+ BIOS loads boot sector (first 512 bytes sector of a bootable disk) at `0x7c00`, and this boot loader space ends at `0x7e00`
+ `objdump -h` to see a summary of header 

## E6. Boot loader
 The small boot loader loads kernel at `0x100000`
 
## E7. VM
The kernel has linker script `kern/kernel.ld` which specifies link/load addr. The first executed instructions are in `entry.S`

+ Load is at physical memory, as it has not yet entered paging
+ Link of C is at virtual memory. This is the addr that compiler uses
+ if not mapped correctly, call to `i386_init` will fail

## E8. printf
+ Oct
+ CGA
## Challenge on printf
+ Use `%C` to set textcolor, e.g. `%C4` is red, `%C0` is resetting color
+ Use `%B` to set background color, `%B0` is resetting

## E9
+ `entry.S` has specified the size of required memory in the directive `bootstack`
+ `bootstacktop` is the high address of bootstack
+ The kernel sets up stack space and make a procedural call by
```assembly
# Set the stack pointer
movl	$(bootstacktop),%esp
# now to C code
call	i386_init
```

## E10 Calling Conventions
C calling conventions on the x86

## E11
+ Recursively read saved ebp by `movl (%ebp) %ebp` until it is `NULL`
+ The outputs are at `i(%ebp)`, where i = 0,1,2,3,4,5,6

## E12
Use binary search to find the symbol for `%eip`, and the related info is line number. 