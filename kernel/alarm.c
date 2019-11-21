#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_sigalarm(void) {
    int ticks;
    uint64 addr;
    if (argint(0, &ticks) < 0)
        return -1;
    if (argaddr(1, &addr) < 0)
        return -1;
    struct proc* p = myproc();
    if (ticks < 0) return -1;
    p->alarm_interval = ticks;
    p->alarm_handler = addr;
    p->alarm_tick_cnt = 0;
    return 0;
}

uint64 sys_sigreturn(void) {
    struct proc* p = myproc();
    if (p->alarm_tick_cnt != -1) return -1;
    p->alarm_tick_cnt = 0;
    memmove(p->tf, &p->alarm_tf, sizeof(p->alarm_tf));
    return 0;
}
