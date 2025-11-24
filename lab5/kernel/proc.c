// kernel/proc.c 
#include "defs.h" 

// 数组的全局定义
struct proc proc[NPROC];
struct cpu cpus[1];

struct proc *initproc;
static int nextpid = 1;

// 释放进程 (必须在使用前定义)
static void freeproc(struct proc *p) {
    if (p->kstack) {
        kfree((void*)p->kstack);
    }
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

// 新进程的入口包装,让所有进程都统一从这里启动
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

// 辅助函数：获取当前CPU
struct cpu* mycpu(void) {
    return &cpus[0];
}

// 辅助函数：获取当前进程
struct proc* myproc(void) {
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

// 查找一个空闲的进程槽
static struct proc* allocproc(void) {
    for (struct proc *p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == UNUSED) {
            p->pid = nextpid++;
            p->state = USED;
             // 分配内核栈：每个进程有独立的内核栈（一页大小）
            if ((p->kstack = (uint64)kalloc()) == 0) {
                freeproc(p); // 使用 freeproc 清理
                release(&p->lock);
                return 0;
            }
            // 设置上下文以从 proc_entry 开始执行
            p->context.sp = p->kstack + PGSIZE;
            p->context.ra = (uint64)proc_entry;

            release(&p->lock);
            return p;
        }
        release(&p->lock);
    }
    return 0;
}

// 初始化进程表
void procinit(void) {
    for (int i = 0; i < NPROC; i++) {
        spinlock_init(&proc[i].lock, "proc");
        proc[i].state = UNUSED;
    }
    printf("procinit: complete\n");
}

// 创建一个新进程（内核线程）
int create_process(void (*entry)(void)) {
    struct proc *p = allocproc();
    if (p == 0) {
        return -1;
    }
    
    p->entry = entry;
    p->parent = myproc();
    
    acquire(&p->lock);
    p->state = RUNNABLE;// 标记为就绪，等待调度器调度
    release(&p->lock);
    
    return p->pid;
}

// 调度器循环
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
                // 上下文切换：保存调度器的上下文到c->context，恢复进程p的上下文
                swtch(&c->context, &p->context);
                
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}

// 切换到调度器
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
    // 上下文切换：保存进程p的上下文，恢复调度器的上下文
    swtch(&p->context, &mycpu()->context);
    mycpu()->intena = intena;
}

// 主动让出CPU
void yield(void) {
    struct proc *p = myproc();
    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

// 进程退出
void exit(int status) {
    struct proc *p = myproc();
    
    acquire(&p->lock);
    
    p->state = ZOMBIE;
    p->xstate = status;
    
    if (p->parent) {
        wakeup(p->parent);
    }

    sched();
    
    printf("exit: unreachable\n");
    while(1);
}

// 等待子进程退出
int wait(int *status) {
    struct proc *p = myproc();
    int havekids, pid;
    
    acquire(&p->lock); // 持有 p->lock

    while(1) {
        havekids = 0;
        for (struct proc *cp = proc; cp < &proc[NPROC]; cp++) {
            if (cp->parent == p) {
                // (更健壮的实现也需要锁 cp->lock, 但我们保持简单)
                if (cp->state == ZOMBIE) {
                    havekids = 1;
                    pid = cp->pid;
                    if (status) *status = cp->xstate;
                    freeproc(cp);
                    release(&p->lock); // 4. 释放 p->lock 并返回
                    return pid;
                }
                havekids = 1;
            }
        }

        if (!havekids) {
            release(&p->lock); // 4. 释放 p->lock 并返回
            return -1;
        }

        //  有子女但未退出，睡眠等待
        sleep(p, &p->lock);
        // 3. sleep 返回时，p->lock 仍然被持有
    }
}


void wait_process(int *status) {
    wait(status);
}


void sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    
    if (lk == 0) {
        printf("sleep: no lock\n");
        while(1);
    }
    
    if (lk != &p->lock){
        // Case 1: 不同的锁 (例如 producer/consumer)
        // 当前持有 lk (例如 buffer_lock).
        // 我们需要 p->lock 来安全地修改进程状态。
        acquire(&p->lock);  // 持有进程锁，保护状态修改
        release(lk);        // 释放外部锁（如缓冲区锁），避免死锁
    }
    
    // Case 2: 相同的锁 (例如 wait)
    // 当前持有 lk (它就是 p->lock).
    // 我们 *不需要* 再次 acquire(&p->lock).
    // 我们将在下面调用 sched() 之前 *持有* p->lock.
    
    p->chan = chan;
    p->state = SLEEPING;
    
    // 调用 sched()。
    // 根据 sched() 的设计, 它 *期望* p->lock 是被持有的, 
    // 并且 p->state *不是* RUNNING (现在是 SLEEPING, 完美).
    sched();

    // --- 唤醒 ---
    // 当 sched() 返回时, 我们仍然持有 p->lock.
    
    p->chan = 0;
    
    if (lk != &p->lock){
        // Case 1: 不同的锁 (producer/consumer)
        // 释放 p->lock 之前, 重新获取我们之前释放的 lk.
        release(&p->lock);
        acquire(lk);
    }
    
    // Case 2: 相同的锁 (wait)
    // 我们什么都不做, 只是返回.
    // p->lock 仍然被持有, wait() 函数的
    // while(1) 循环会继续执行 (或者在循环开始时释放它).
    // 在我们的 wait() 中, 它会在下一次循环开始时释放.
}

// 唤醒
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