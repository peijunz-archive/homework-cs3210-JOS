# Lab 4: Preemptive multitasking

## E1
Allocate virtual memory space for MMIO starting from MMIOBASE. The LAPIC lives in a high area of paddr, which makforce us to map it manually to low addr. (if < 2G then default mapping works fine)

## E2
Just skip a page to make sure APs safely runs on `MPENTRY_PADDR`
```c
if (i*PGSIZE == MPENTRY_PADDR) continue;
```

## Q1
+ in boot.S we have configured linker so we could make load/link addr different. 
+ in mpentry.S, the MACRO works for the similar purpose

## E3 E4
+ Stack space mapping
+ `trap_init_percpu()` is the code that every cpu set its trap handlers and TSS, and later load them with lidt and ltr respectively
+ task state segment (TSS) are configuring which stack to use when interrupt happens

## E5 E6 Q2
+ Big kernel lock
+ race condition happens
+ RR scheduling by scanning through all envs...
+ yield just calls scheduler...

## Q3 Q4
+ Address space of user env is super set of kernel addr
+ User envs just do not have the privilege to access e while kernel has
+ The privilege level is in mode after an interrupt. By using iret in env_pop_tf, kernel is switched to user mode
+ There are trap frames stored in user envs. It is done at `trap()`
```c
curenv->env_tf = *tf;
```

## E7 E8 E9
+ more syscalls
+ pgfault handler
+ user exception stack

## E10 E11
+ set user level pgfault hadler
+ Write some asm to pop trap frames after its calling user level page fault handlers to resume envs to previous trap or normal execution

## E12
+ COW
+ take advantage of UVPT
+ dup process of a page simply overwrites the PTE of both user pages of parent/child by using sys_page_map
+ reduce # of copies by ref count?
+ sfork TODO, there is still bug when running forktree with sfork

## E13 E14 E15
+ Set the timer interrupt (IRQ)
+ its hadler calls scheduler
+ ipc send and receive




