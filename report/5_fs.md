# Lab 5: File System

There is error of mmap happened when making `clean-fs.img`, so Makefrag is modified to simply copy the image from project directory

## E1 Q1
add the flags by

```c
e->env_tf.tf_eflags |= FL_IOPL_3;
```

Nothing else needed because eflags is resumed/saved on context switch

## E2
+ use alloc page and map page to support demand paging by pgfault handler

## E3
Simple bit manipulations

## E4
+ file_block_walk could probably create indirect block
+ file_get_block should allocate block if the block entry is current ZERO

## E5 E6
+ Read should use pageref to prevent stale file descriptor

## E7
Remember to set CPL/~IOPL/Interrupt

## E8
I have found the problem of old fork from lab4, and updated it to a more complete copy
of address space. But it is still fast as we have taken advantage of 2 level page-table
to avoid unnecessary walking

## E9
Keyboard

## E10
stdin redir of shell