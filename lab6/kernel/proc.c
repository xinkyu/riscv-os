// kernel/proc.c
#include "defs.h"

struct proc proc[NPROC];
struct cpu cpus[1]; // 确保定义在这里
struct proc *initproc;
static int nextpid = 1;

// [修复] 强制单核
struct cpu* mycpu(void) {
    return &cpus[0];
}

struct proc* myproc(void) {
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

// [新增] 专门给 test.c 用的调试接口，绕过头文件可能存在的问题
// 直接返回 CPU 0 的当前进程
struct proc* get_current_proc_debug(void) {
    return cpus[0].proc;
}

static void freeproc(struct proc *p) {
    if (p->trapframe) kfree((void*)p->trapframe);
    p->trapframe = 0;
    if (p->kstack) kfree((void*)p->kstack);
    p->kstack = 0;
    p->pagetable = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
}

// 简单的入口点
void proc_entry(void) {
    struct proc *p = myproc();
    release(&p->lock);
    if (p->entry) p->entry();
    exit(0);
}

static struct proc* allocproc(void) {
    struct proc *p;
    for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == UNUSED) {
            goto found;
        } else {
            release(&p->lock);
        }
    }
    return 0;

found:
    p->pid = nextpid++;
    p->state = USED;

    // 分配 trapframe
    if((p->trapframe = (struct trapframe *)kalloc()) == 0){
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    // 清零 trapframe
    char *tf = (char*)p->trapframe;
    for(int i=0; i<4096; i++) tf[i] = 0;

    if((p->kstack = (uint64)kalloc()) == 0){
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    p->context.sp = p->kstack + PGSIZE;
    p->context.ra = (uint64)proc_entry;

    release(&p->lock);
    return p;
}

void procinit(void) {
    for (int i = 0; i < NPROC; i++) {
        spinlock_init(&proc[i].lock, "proc");
        proc[i].state = UNUSED;
    }
    printf("procinit: complete\n");
}

int create_process(void (*entry)(void)) {
    struct proc *p = allocproc();
    if (p == 0) return -1;
    p->entry = entry;
    p->parent = 0; 
    acquire(&p->lock);
    p->state = RUNNABLE;
    release(&p->lock);
    return p->pid;
}

// fork 实现
int fork(void) {
    int i;
    struct proc *np;
    struct proc *p = myproc();

    if ((np = allocproc()) == 0) return -1;

    *(np->trapframe) = *(p->trapframe);
    np->trapframe->a0 = 0;

    for(i = 0; i < 16; i++) np->name[i] = p->name[i];
    np->parent = p;

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

    return np->pid;
}

void scheduler(void) {
    struct cpu *c = mycpu();
    c->proc = 0;
    printf("scheduler: starting on cpu 0\n");
    while(1) {
        intr_on();
        for (struct proc *p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p; // 设置当前进程
                
                // [调试] 确认设置成功
                // printf("sched: switch to %d\n", p->pid);

                swtch(&c->context, &p->context);
                
                c->proc = 0; // 清除
            }
            release(&p->lock);
        }
    }
}

void sched(void) {
    int intena;
    struct proc *p = myproc();
    if (!p->lock.locked) { printf("sched: lock not held\n"); while(1); }
    if (p->state == RUNNING) { printf("sched: state is RUNNING\n"); while(1); }
    intena = mycpu()->intena;
    swtch(&p->context, &mycpu()->context);
    mycpu()->intena = intena;
}

void yield(void) {
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

void exit(int status) {
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = ZOMBIE;
    p->xstate = status;
    if (p->parent) wakeup(p->parent);
    sched();
    while(1);
}

int wait(int *status) {
    struct proc *p = myproc();
    int havekids, pid;
    acquire(&p->lock);
    while(1) {
        havekids = 0;
        for (struct proc *cp = proc; cp < &proc[NPROC]; cp++) {
            if (cp->parent == p) {
                if (cp->state == ZOMBIE) {
                    havekids = 1;
                    pid = cp->pid;
                    if (status) *status = cp->xstate;
                    freeproc(cp);
                    release(&p->lock);
                    return pid;
                }
                havekids = 1;
            }
        }
        if (!havekids) {
            release(&p->lock);
            return -1;
        }
        sleep(p, &p->lock);
    }
}

void wait_process(int *status) { wait(status); }

void sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    if (lk != &p->lock) { acquire(&p->lock); release(lk); }
    p->chan = chan;
    p->state = SLEEPING;
    sched();
    p->chan = 0;
    if (lk != &p->lock) { release(&p->lock); acquire(lk); }
}

void wakeup(void *chan) {
    for (struct proc *p = proc; p < &proc[NPROC]; p++) {
        if (p != myproc()) {
            acquire(&p->lock);
            if (p->state == SLEEPING && p->chan == chan) {
                p->state = RUNNABLE;
            }
            release(&p->lock);
        }
    }
}