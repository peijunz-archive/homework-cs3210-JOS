/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>
#include <kern/e1000.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	struct Env *newenv;
	int status = env_alloc(&newenv, curenv->env_id);
	if (status == 0){
		// It is no longer ENV_FREE
		memcpy(newenv, curenv, sizeof(struct Trapframe));
		newenv->env_status = ENV_NOT_RUNNABLE;
		newenv->env_tf.tf_regs.reg_eax = 0;
		return newenv->env_id;
	}
	else{
		return status;
	}
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	struct Env *e;
	if (envid2env(envid, &e, 1) || !e){
		return -E_BAD_ENV;
	}
	if (status == ENV_RUNNABLE || status == ENV_RUNNING){
		e->env_status = status;
		return 0;
	}
	return -E_INVAL;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env* e;
	pte_t *pte;
	if (!(pte = pgdir_walk(curenv->env_pgdir, tf, 0)))
		return -E_INVAL;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	tf->tf_eflags &= ~FL_IOPL_3;
	tf->tf_eflags |= FL_IF;
	tf->tf_ds = GD_UD | 3;
	tf->tf_es = GD_UD | 3;
	tf->tf_ss = GD_UD | 3;
	tf->tf_cs = GD_UT | 3;
	e->env_tf = *tf;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *e;
	if (envid2env(envid, &e, 1) || !e){
		return -E_BAD_ENV;
	}
	e->env_pgfault_upcall = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	struct Env *e;
	struct PageInfo *pp;
	if (va >= (void*)UTOP || (uint32_t)va %PGSIZE)
		return -E_INVAL;
	// if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL))
	// 	return -E_INVAL;
	if ((perm & ~PTE_SYSCALL))
		return -E_INVAL;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	if (!(pp = page_alloc(ALLOC_ZERO)))
		return -E_NO_MEM;
	if (page_insert(e->env_pgdir, pp, va, perm|PTE_P|PTE_U) < 0){
		page_free(pp);
		return -E_NO_MEM;
	}
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.

	struct Env *e1, *e2;
	struct PageInfo *pp;
	pte_t *pte;
	if (srcva >= (void*)UTOP || (uint32_t)srcva %PGSIZE || dstva >= (void*)UTOP || (uint32_t)dstva %PGSIZE)
		return -E_INVAL;
	if ((perm & ~PTE_SYSCALL))
		return -E_INVAL;
	if ((envid2env(srcenvid, &e1, 1) < 0) || (envid2env(dstenvid, &e2, 1) < 0))
		return -E_BAD_ENV;
	if (!(pp = page_lookup(e1->env_pgdir, srcva, &pte)))
		return -E_INVAL;
	if ((perm & PTE_W) && !(*pte & PTE_W))
		return -E_INVAL;
	return page_insert(e2->env_pgdir, pp, dstva, perm|PTE_U|PTE_P);
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	struct Env *e;
	struct PageInfo *pp;
	if (va >= (void*)UTOP || (uint32_t)va %PGSIZE)
		return -E_INVAL;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *dstenv;
	struct PageInfo * pp;
	pte_t *pte;
	if (envid2env(envid, &dstenv, 0)<0)
		return -E_BAD_ENV;
	// cprintf("%p, %d, %d, %d\n", dstenv, envid, dstenv->env_status, dstenv->env_ipc_recving);
	if (dstenv->env_status != ENV_NOT_RUNNABLE || !dstenv->env_ipc_recving)
		return -E_IPC_NOT_RECV;
	if ((uintptr_t)srcva < UTOP && (uintptr_t)dstenv->env_ipc_dstva < UTOP){
		if ((uintptr_t)srcva % PGSIZE)
			return -E_INVAL;
		if ((perm & ~PTE_SYSCALL))
			return -E_INVAL;
		if (!(pp = page_lookup(curenv->env_pgdir, srcva, &pte)))
			return -E_INVAL;
		if ((perm & PTE_W) && !(*pte & PTE_W))
			return -E_INVAL;
		if (page_insert(dstenv->env_pgdir, pp, dstenv->env_ipc_dstva, perm|PTE_U|PTE_P)<0)
			return -E_NO_MEM;
		dstenv->env_ipc_perm = perm;
	}
	dstenv->env_ipc_recving = 0;
	dstenv->env_ipc_from = curenv->env_id;
	dstenv->env_ipc_value = value;
	dstenv->env_status = ENV_RUNNABLE;
	dstenv->env_tf.tf_regs.reg_eax = 0;
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	if ((uintptr_t)dstva % PGSIZE)
		return -E_INVAL;
	curenv->env_ipc_from = 0;
	curenv->env_ipc_perm = 0;
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;

	// cprintf("recv: %p, %d, %d, %d\n", curenv, curenv->env_id, curenv->env_status, curenv->env_ipc_recving);
	sched_yield();
	return 0;
}

// Return the current time.
static int
sys_time_msec(void)
{
	// LAB 6: Your code here.
	return time_msec();
}

// Send net packet
static int
sys_net_send(void *buf, int size){
	if ((uintptr_t)buf+size >= UTOP){
		return -E_INVAL;
	}
	return transmit_packet(buf, size);
}

// Receive net packet
static int
sys_net_recv(void *buf, int size){
	if ((uintptr_t)buf+size >= UTOP){
		return -E_INVAL;
	}
	return receive_packet(buf, size);
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

 	// cprintf("syscall with %d\n", syscallno);
	switch (syscallno) {
	case SYS_cputs:
		// The pagedir is still curenv, as user page dir includes kernel page dir
		// they do not have access though
		user_mem_assert(curenv, (void*)a1, a2, PTE_U|PTE_P);
		sys_cputs((void*)a1, a2);
		return 0;
	case SYS_cgetc:
		return sys_cgetc();
	case SYS_getenvid:
		return sys_getenvid();
	case SYS_env_destroy:
		return sys_env_destroy(a1);
	case SYS_page_alloc:
		return sys_page_alloc(a1, (void*)a2, a3);
	case SYS_page_map:
		return sys_page_map(a1, (void*)a2, a3, (void*)a4, a5);
	case SYS_page_unmap:
		return sys_page_unmap(a1, (void*)a2);
	case SYS_exofork:
		return sys_exofork();
	case SYS_env_set_status:
		return sys_env_set_status(a1, a2);
	case SYS_env_set_trapframe:
		return sys_env_set_trapframe(a1, (void*)a2);
	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void*)a2);
	case SYS_yield:
		sys_yield();
		return 0;
	case SYS_ipc_try_send:
		return sys_ipc_try_send(a1, a2, (void*)a3, a4);
	case SYS_ipc_recv:
		return sys_ipc_recv((void*)a1);
	case SYS_time_msec:
		return sys_time_msec();
	case SYS_net_send:
		return sys_net_send((void*)a1, a2);
	case SYS_net_recv:
		return sys_net_recv((void*)a1, a2);
	// case NSYSCALLS:
	default:
		return -E_INVAL;
	}
}

