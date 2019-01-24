# Lab 2: Memory Management

## E1
+ Page table info is in a linkedlist
+ nextfree determined address of the true available page

## E2
Read i386 Manual

## E3
debug with qemu, TBD

## Question
All pointers in C should be virtual address. Thus mystery_t is `uintptr_t`

The two macros are useful in convertion between KV Addr and Physical Addr
```c
KADDR(pa)
PADDR(ka)
```

## E4
...