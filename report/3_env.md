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
+ The privilege level for brkpt should be 3
+ To prevent the user from executing privileged instructions like page fault
 
## E7
