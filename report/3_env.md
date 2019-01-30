# Lab 2: User Environments
## E1
...

## E2
+ Allocate envs
+ Copy page directory of Kernel address space as the first step
+ Temporarily use the user page directory by cr3 instruction, then copy binary to the related areas with memcpy.
 
## E3
https://pdos.csail.mit.edu/6.828/2018/readings/i386/c09.htm

## E4
+ `pushal` could save all general purpose registers
+ `.data .text .name .global` directives
+ `extern T a[];` vs `extern T *a;`

## Question
+ different privilege
+ No privilege?

## E5
+ panic for kernel page fault
+ Kill env for user page fault

## E6
## Question 2
+ The privilege level for BRKPT should be 3
+ To prevent the user from executing privileged instructions like page fault
 
## E7
+ SYSCALL has no errorcode
+ How is shift of eip determined?
+ User space vs kernel space syscall functions
+ Caveats for extern function/array definitions. There should NEVER be cast for extern variable
```c
// GOOD
typedef void (*handler_t)();
extern handler_t handler_table[];
extern handler_t syscall_handler;
extern void syscall_handler();
//BAD
extern handler_t handler_table[]; //BAD
extern void (*syscall_handler)(); //BAD
```


## E8
+ a user env starts from libmain, which sets the userid for user space
+ printf and exit calls `sys_puts` and `sys_env_destrop` respectively

## E9 E10
Simply page walk and check existence as well as permission for pages