#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;

  // modify here for lazy allocate heap
  if (n > 0) {
      myproc()->sz = addr + n;
  } else { // handle for decrease bytes, should occur immediately, because more mem available
      if(growproc(n) < 0)
        return -1;
  }

  /*
  if(growproc(n) < 0)
    return -1;
  */
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_date(void) 
{
	struct rtcdate *rtc;
	if(argptr(0, (void*)&rtc, sizeof(*rtc)) < 0)
		return -1;
	cmostime(rtc);
	return 0;
}

int
sys_alarm(void) {
    int ticks;
    void (*handler)();

    if(argint(0, &ticks) < 0)
        return -1;
    if(argptr(1, (char**)&handler, 1) < 0)
        return -1;
    myproc()->alarmticks = ticks;
    myproc()->alarmhandler = handler;

    acquire(&tickslock);
    myproc()->lastticks = ticks;
    release(&tickslock);
    return 0;
}

// howto:
// check if tick from last time elapsed >= alarmticks
// append user stack addr(handler), set eip to this addr(now return to handler, after handler return will return to old code
//      Note: remember ebp for old stack's store !!
// check dup:
//      if in this code, but eip is between handler and end_of_handler
/*
 * 1 first save caller saved registers onto stack, then old eip, now set to new handler as new eip, set return code
 *   as sys_sigreturn
 *   sys_sigreturn:
 *      restore caller registers into trapframe, set eip to old eip, return
 */
void handle_alarm(struct trapframe *tf) {
    // how to handle alarm here
    int ticks0; 
    acquire(&tickslock);
    ticks0 = ticks;
    release(&tickslock);

    // no need to call yet
    if (ticks0 - myproc()->alarmticks < myproc()->lastticks || myproc()->sighandling) {
        // todo:
        //      also check if handler is doing
        // still doing handler or not yet time to handle
        return;
    }
    myproc()->lastticks = ticks0;

    // save caller saved user registers and old return eip
    tf->esp -= 4;
    *(uint*)tf->esp = tf->eax;
    tf->esp -= 4;
    *(uint*)tf->esp = tf->ecx;
    tf->esp -= 4;
    *(uint*)tf->esp = tf->edx;
    tf->esp -= 4;
    *(uint*)tf->esp = tf->eip;

    // we can use asm volatile here! like x86.h
    tf->esp -= 4;
    *(uint*)tf->esp = 0xc340cd00; // int 0x40
    tf->esp -= 4;
    *(uint*)tf->esp = 0x18b8; // mov 0x18, eax
    uint tmp = tf->esp;
    tf->esp -= 4;
    *(uint*)tf->esp = tmp; // jump back to sig return syscall
//    cprintf("return esp is %x *esp %x\n", tf->esp, *(uint*)tf->esp);
    tf->eip = (uint)myproc()->alarmhandler; // set new eip
    myproc()->sighandling = 1; // no need lock here?
    return;
}

int sys_sigreturn(void) {
    struct trapframe *tf = myproc()->tf;
    tf->esp += 8;  // drop executed code 8byte on stack
    tf->eip = *(uint*)tf->esp;
    tf->esp += 4;
    tf->edx = *(uint*)tf->esp;
    tf->esp += 4;
    tf->ecx = *(uint*)tf->esp;
    tf->esp += 4;
    tf->eax = *(uint*)tf->esp; // restore and return
    tf->esp += 4;
    myproc()->sighandling = 0;
    return 0;
}
