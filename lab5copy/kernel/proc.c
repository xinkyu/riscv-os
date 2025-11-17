// kernel/proc.c (Lab6 修改版)
#include "defs.h"

struct proc proc[NPROC];
struct cpu cpus[1];

struct proc *initproc;
static int nextpid = 1;

extern char trampoline[];

static void freeproc(struct proc *p) {
    if (p->trapframe)
        kfree((void*)p->trapframe);
    p->trapframe = 0;
    if (p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
    
    if (p->kstack) {
        kfree((void*)p->kstack);
    }
    p->kstack = 0;
}

void proc_entry(void) {
    struct proc *p = myproc();
    release(&p->lock);
    if (p->entry) {
        p->entry();
    } else {
        printf("proc_entry: no entry function\n");
    }
    exit(0);
}

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

static struct proc* allocproc(void) {
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == UNUSED) {
            goto found;
        }
        release(&p->lock);
    }
    return 0;

found:
    p->pid = nextpid++;
    p->state = USED;
    
    // 分配 trapframe
    if ((p->trapframe = (struct trapframe*)kalloc()) == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    
    // 创建用户页表
    if ((p->pagetable = proc_pagetable(p)) == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    
    // 设置内核栈
    if ((p->kstack = (uint64)kalloc()) == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    
    // 设置上下文
    p->context.sp = p->kstack + PGSIZE;
    p->context.ra = (uint64)proc_entry;
    
    return p;
}

void procinit(void) {
    for (int i = 0; i < NPROC; i++) {
        spinlock_init(&proc[i].lock, "proc");
        proc[i].state = UNUSED;
        proc[i].is_user = 0;
    }
    printf("procinit: complete\n");
}

int create_process(void (*entry)(void)) {
    struct proc *p = allocproc();
    if (p == 0) {
        return -1;
    }
    
    p->entry = entry;
    p->parent = myproc();
    p->is_user = 0;
    
    acquire(&p->lock);
    p->state = RUNNABLE;
    release(&p->lock);
    
    return p->pid;
}

// Lab6: fork 系统调用
int fork(void) {
    struct proc *p = myproc();
    struct proc *np;
    
    if ((np = allocproc()) == 0)
        return -1;
    
    // 复制用户内存
    if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->sz = p->sz;
    
    // 复制 trapframe
    *(np->trapframe) = *(p->trapframe);
    
    // fork 返回 0 给子进程
    np->trapframe->a0 = 0;
    
    np->parent = p;
    np->is_user = 1;
    
    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);
    
    return np->pid;
}

void scheduler(void) {
    struct cpu *c = mycpu();
    c->proc = 0;
    
    printf("scheduler: starting\n");
    
    while(1) {
        intr_on();
        
        for (struct proc *p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p;
                
                swtch(&c->context, &p->context);
                
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}

void sched(void) {
    int intena;
    struct proc *p = myproc();

    if (!p->lock.locked) {
        printf("sched: lock not held\n"); while(1);
    }
    if (p->state == RUNNING) {
        printf("sched: state is RUNNING\n"); while(1);
    }

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
    
    if (p == initproc)
        printf("init exiting");
    
    acquire(&p->lock);
    
    p->xstate = status;
    p->state = ZOMBIE;
    
    wakeup(p->parent);
    
    sched();
    
    printf("zombie exit");
}

int wait(uint64 addr) {
    struct proc *np;
    int havekids, pid;
    struct proc *p = myproc();
    
    acquire(&p->lock);
    
    for(;;) {
        havekids = 0;
        for(np = proc; np < &proc[NPROC]; np++) {
            if(np->parent == p) {
                acquire(&np->lock);
                havekids = 1;
                if(np->state == ZOMBIE) {
                    pid = np->pid;
                    if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                           sizeof(np->xstate)) < 0) {
                        release(&np->lock);
                        release(&p->lock);
                        return -1;
                    }
                    freeproc(np);
                    release(&np->lock);
                    release(&p->lock);
                    return pid;
                }
                release(&np->lock);
            }
        }
        
        if(!havekids || p->killed) {
            release(&p->lock);
            return -1;
        }
        
        sleep(p, &p->lock);
    }
}

void wait_process(int *status) {
    wait((uint64)status);
}

void sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    
    if (lk == 0) {
        printf("sleep: no lock\n");
        while(1);
    }
    
    if (lk != &p->lock){
        acquire(&p->lock);
        release(lk);
    }
    
    p->chan = chan;
    p->state = SLEEPING;
    
    sched();
    
    p->chan = 0;
    
    if (lk != &p->lock){
        release(&p->lock);
        acquire(lk);
    }
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

// Lab6: 创建第一个用户进程
extern char _binary_initcode_start[], _binary_initcode_size[];

int userinit(void) {
    struct proc *p = allocproc();
    if (p == 0)
        return -1;
    
    initproc = p;
    
    // 分配用户内存，加载 initcode
    uvmalloc(p->pagetable, 0, PGSIZE);
    p->sz = PGSIZE;
    
    // 复制 initcode
    copyout(p->pagetable, 0, _binary_initcode_start, 
            (uint64)_binary_initcode_size);
    
    // 设置 trapframe
    memset(p->trapframe, 0, sizeof(*p->trapframe));
    p->trapframe->epc = 0;
    p->trapframe->sp = PGSIZE;
    
    p->is_user = 1;
    strncpy(p->name, "initcode", sizeof(p->name));
    
    acquire(&p->lock);
    p->state = RUNNABLE;
    release(&p->lock);
    
    return 0;
}