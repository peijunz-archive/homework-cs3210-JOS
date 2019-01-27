# Lab 2: Memory Management

## E1
+ Page table info is in a linkedlist
+ nextfree determined address of the true available page
+ `boot_alloc()` allocates `pages`, which will be used to stores page info for every physical  page
+ `mem_init()` set up the new Virtual Address mapping as plotted in `memlayout.h`
+ `page_init()` initialized pages array, and set up a linkedlist to manage free pages
+ `page_alloc()` allocates a page from free pages list
+ `page_free()` deallocates a page and release it to free pages list
 
## E2
Read i386 Manual...

## E3
debug with qemu/GDB. Inspecting physical memory/virtual memory to ensure the memory mapping might be useful. 

## Question 1
All pointers in C should be virtual address. Thus mystery_t is `uintptr_t`

The two macros are useful in convertion between KV Addr and Physical Addr
```c
KADDR(pa)
PADDR(ka)
```

## E4
+ `pgdir_walk()` is the basic utility to find PTE
+ `boot_map_region()` changes PTE to reflect the mapping. Note it does not allocate any physical page, and only maps memory above UTOP
+ `page_lookup()` calls `pgdir_walk()` and could return the corresponding page info as well as PTE
+ `page_remove()` deallocates a page and kick it from its PTE. It calls `page_deref()` to decrement its refcount, which frees the physical page when ref count is 0
+ `page_insert()` could be used to update permission and allocate physical page for some VA

## E5
Just use `boot_map_region()` to make it!

## Questions 2-5
2. TBD
3. because of the privilege bit `S/U`
4. 4G, the PTD has 1K PDE, every page table has 1K PTE, every page has 4K Bytes
5. Space taken by `pages` and also page directory/table
6. PC relative addressing for simple `jmp` instructions. It is necessary because later function calls will need to use absolute virtual address.

## Challenges
1. `PTE_PS` bit controls whether an entry in PTD is 4M super page or an actual page table
2. TBD
3. TBD
4. Swapping?
5. Buddy Allocator