#include <stdarg.h>
#include "defs.h"
#include "klog.h"


// 日志条目结构体：存储单条日志的所有元数据
struct klog_record {
    uint64 timestamp;               // 时间戳
    int level;                      // 日志等级 (TRACE/DEBUG/INFO...)
    int cpu;                        // 产生日志的 CPU 核心 ID
    int pid;                        // 产生日志的进程 ID
    char component[KLOG_COMPONENT_MAX]; // 组件名 (例如 "FS", "SCHED")
    char message[KLOG_MESSAGE_MAX];     // 格式化后的具体消息内容
};

// 全局日志状态结构体：管理整个日志系统
struct klog_state {
    struct spinlock lock;           // 自旋锁，用于多核并发保护
    struct klog_record buffer[KLOG_BUFFER_SIZE]; // 环形缓冲区数组
    int next;                       // 下一个写入位置的索引
    int count;                      // 当前缓冲区中的有效日志数量
    enum klog_level buffer_level;   // 缓冲区记录等级阈值 (低于此等级不存入内存)
    enum klog_level console_level;  // 控制台输出等级阈值 (低于此等级不打印到屏幕)
    int console_enabled;            // 控制台输出的总开关
    int initialized;                // 初始化标志
    
    // 统计信息
    uint64 total_generated;         // 产生的总日志数
    uint64 stored;                  // 存入缓冲区的日志数
    uint64 overwritten;             // 因缓冲区满而被覆盖的日志数
    uint64 console_emitted;         // 输出到控制台的日志数
};

static struct klog_state klog;

static const char *level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};
// 将日志等级限制在合法范围内
static int clamp_level(enum klog_level level) {
    if (level < KLOG_LEVEL_TRACE) return KLOG_LEVEL_TRACE;
    if (level > KLOG_LEVEL_FATAL) return KLOG_LEVEL_FATAL;
    return level;
}
//安全的字符串拷贝
static void copy_string(char *dst, int max, const char *src) {
    if (!src) {
        src = "kernel";
    }
    int i = 0;
    for (; i < max - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';// 确保字符串以 null 结尾
}
// 向缓冲区追加单个字符
static void append_char(char **ptr, int *remaining, char c) {
    if (*remaining <= 1) {// 预留 1 字节给结尾的 \0
        if (*remaining == 1) {
            **ptr = '\0';
            (*remaining)--;
        }
        return;
    }
    **ptr = c;
    (*ptr)++;
    (*remaining)--;
}
// 向缓冲区追加字符串
static void append_string(char **ptr, int *remaining, const char *s) {
    if (!s) {
        s = "(null)";
    }
    while (*s && *remaining > 1) {
        append_char(ptr, remaining, *s++);
    }
}
// 向缓冲区追加整数 (支持不同进制)
static void append_uint(char **ptr, int *remaining, unsigned long long x, int base, int prefix0x) {
    char buf[32];
    int i = 0;
    // 处理 0 的特殊情况
    if (x == 0) {
        buf[i++] = '0';
    } else {
        // 转换为字符串 (逆序)
        while (x && i < (int)sizeof(buf)) {
            int digit = x % base;
            buf[i++] = "0123456789abcdef"[digit];
            x /= base;
        }
    }
    // 添加 0x 前缀 (如果是十六进制且要求前缀)
    if (prefix0x && *remaining > 2) {
        append_char(ptr, remaining, '0');
        append_char(ptr, remaining, 'x');
    }
    // 将缓冲区内容倒序写入目标
    while (i > 0 && *remaining > 1) {
        append_char(ptr, remaining, buf[--i]);
    }
}

// 向缓冲区追加有符号整数
static void append_int(char **ptr, int *remaining, long long v) {
    if (v < 0) {
        append_char(ptr, remaining, '-');
        append_uint(ptr, remaining, (unsigned long long)(-v), 10, 0);
    } else {
        append_uint(ptr, remaining, (unsigned long long)v, 10, 0);
    }
}

// 核心格式化函数：类似 vsnprintf 的精简版实现
static void kvsnprintf(char *dst, int max, const char *fmt, va_list ap) {
    char *ptr = dst;
    int remaining = max;
    if (max <= 0) return;

    while (*fmt && remaining > 1) {
        if (*fmt != '%') {
            append_char(&ptr, &remaining, *fmt++);
            continue;
        }
        fmt++;
        char spec = *fmt++;
        if (spec == '\0') break;

        // 解析格式化符号
        switch (spec) {
            case 's': // 字符串
                append_string(&ptr, &remaining, va_arg(ap, const char*));
                break;
            case 'c': // 字符
                append_char(&ptr, &remaining, (char)va_arg(ap, int));
                break;
            case 'd': // 整数
                append_int(&ptr, &remaining, va_arg(ap, int));
                break;
            case 'u': // 无符号整数
                append_uint(&ptr, &remaining, va_arg(ap, unsigned int), 10, 0);
                break;
            case 'x': // 十六进制
                append_uint(&ptr, &remaining, va_arg(ap, unsigned int), 16, 0);
                break;
            case 'p': // 指针 (带 0x 前缀)
                append_uint(&ptr, &remaining, va_arg(ap, uint64), 16, 1);
                break;
            case 'l': { // 长整型处理 (%ld, %lu, %lx)
                char next = *fmt++;
                if (next == 'd') append_int(&ptr, &remaining, va_arg(ap, long));
                else if (next == 'u') append_uint(&ptr, &remaining, va_arg(ap, unsigned long), 10, 0);
                else if (next == 'x') append_uint(&ptr, &remaining, va_arg(ap, unsigned long), 16, 0);
                else { // 无法识别，原样打印
                    append_char(&ptr, &remaining, '%');
                    append_char(&ptr, &remaining, 'l');
                    append_char(&ptr, &remaining, next);
                }
                break;
            }
            case '%':
                append_char(&ptr, &remaining, '%');
                break;
            default:
                append_char(&ptr, &remaining, '%');
                append_char(&ptr, &remaining, spec);
                break;
        }
    }
    // 确保字符串终止
    if (remaining > 0) {
        *ptr = '\0';
    } else {
        dst[max - 1] = '\0';
    }
}
// 实际打印函数：将日志条目输出到标准输出 (UART/VGA)
static void print_record(const struct klog_record *rec) {
    printf("[%s][time=%lu][cpu=%d pid=%d][%s] %s\n",
           level_names[clamp_level(rec->level)],
           rec->timestamp,
           rec->cpu,
           rec->pid,
           rec->component,
           rec->message);
}
// 初始化日志系统
void klog_init(void) {
    if (klog.initialized) {
        return;
    }
    spinlock_init(&klog.lock, "klog");
    klog.next = 0;
    klog.count = 0;
    // 默认配置：详细记录到内存，仅警告/错误输出到屏幕
    klog.buffer_level = KLOG_LEVEL_TRACE;
    klog.console_level = KLOG_LEVEL_WARN;
    klog.console_enabled = 1;
    klog.total_generated = 0;
    klog.stored = 0;
    klog.overwritten = 0;
    klog.console_emitted = 0;
    klog.initialized = 1;
    printf("[klog] initialized (buffer=%d, console-level=%d)\n", KLOG_BUFFER_SIZE, klog.console_level);
}
// 写入日志的主函数 (外部调用入口)
void klog_write(enum klog_level level, const char *component, const char *fmt, ...) {
    if (!klog.initialized || fmt == 0) {
        return;
    }

    struct klog_record rec;
    rec.timestamp = get_time();
    rec.level = clamp_level(level);
    // 获取 CPU ID (如果当前环境允许)
    struct cpu *cpu = mycpu();
    if (cpu) {
        int idx = (int)(cpu - cpus);
        if (idx >= 0 && idx < NCPU) {
            rec.cpu = idx;
        } else {
            rec.cpu = -1;
        }
    } else {
        rec.cpu = -1;
    }
    // 获取进程 PID
    struct proc *p = myproc();
    rec.pid = p ? p->pid : -1;
    copy_string(rec.component, KLOG_COMPONENT_MAX, component);
    // 格式化消息内容 (此时还没加锁，提升并发性能)
    va_list args;
    va_start(args, fmt);
    kvsnprintf(rec.message, KLOG_MESSAGE_MAX, fmt, args);
    va_end(args);

    int should_console = 0;
    // 进入临界区：操作全局缓冲区
    acquire(&klog.lock);
    klog.total_generated++;

    // 检查是否需要写入内存缓冲区
    if (rec.level >= klog.buffer_level) {
        // 如果缓冲区满了，增加覆盖计数
        if (klog.count == KLOG_BUFFER_SIZE) {
            klog.overwritten++;
        } else {
            klog.count++;
        }
        // 写入环形缓冲区 (覆盖旧数据)
        klog.buffer[klog.next] = rec;
        // 更新写入指针 (循环回绕)
        klog.next = (klog.next + 1) % KLOG_BUFFER_SIZE;
        klog.stored++;
    }

    // 检查是否需要输出到控制台
    if (klog.console_enabled && rec.level >= klog.console_level) {
        should_console = 1;
    }
    release(&klog.lock); // 退出临界区

    // 执行控制台输出
    // 持有锁打印会阻塞其他核心写入日志。
    if (should_console) {
        print_record(&rec);
        // 仅为了更新统计数据再次加锁 
        acquire(&klog.lock);
        klog.console_emitted++;
        release(&klog.lock);
    }
}
// 动态设置缓冲区记录等级
void klog_set_buffer_level(enum klog_level level) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.buffer_level = clamp_level(level);
    release(&klog.lock);
}
// 动态设置控制台输出等级
void klog_set_console_level(enum klog_level level) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.console_level = clamp_level(level);
    release(&klog.lock);
}
// 开启或关闭控制台输出
void klog_enable_console(int enable) {
    if (!klog.initialized) return;
    acquire(&klog.lock);
    klog.console_enabled = enable ? 1 : 0;
    release(&klog.lock);
}
// 调试功能：打印内存中最近的 N 条日志
void klog_dump_recent(int max_entries) {
    if (!klog.initialized) return;
    if (max_entries <= 0 || max_entries > KLOG_BUFFER_SIZE) {
        max_entries = KLOG_BUFFER_SIZE;
    }

    acquire(&klog.lock);
    int available = klog.count;
    if (available > max_entries) {
        available = max_entries;
    }
    // 计算环形缓冲区的起始读取位置 (倒推)
    // 公式：(next - available + SIZE) % SIZE
    int start = (klog.next - available + KLOG_BUFFER_SIZE) % KLOG_BUFFER_SIZE;
    for (int i = 0; i < available; i++) {
        int idx = (start + i) % KLOG_BUFFER_SIZE;
        struct klog_record rec = klog.buffer[idx];
        release(&klog.lock);
        print_record(&rec);
        acquire(&klog.lock);
    }
    release(&klog.lock);
}
// 打印日志系统运行状态摘要
void klog_summary(void) {
    if (!klog.initialized) {
        printf("[klog] not initialized\n");
        return;
    }
    acquire(&klog.lock);
    printf("[klog] total=%lu stored=%lu overwritten=%lu console=%lu buffer_level=%d console_level=%d enabled=%d size=%d\n",
           klog.total_generated,
           klog.stored,
           klog.overwritten,
           klog.console_emitted,
           klog.buffer_level,
           klog.console_level,
           klog.console_enabled,
           klog.count);
    release(&klog.lock);
}

void klog_get_stats(struct klog_stats *stats) {
    if (!stats) return;
    if (!klog.initialized) {
        // 如果未初始化，返回空数据
        stats->total_generated = 0;
        stats->stored = 0;
        stats->overwritten = 0;
        stats->console_emitted = 0;
        stats->buffered_entries = 0;
        stats->buffer_level = KLOG_LEVEL_TRACE;
        stats->console_level = KLOG_LEVEL_INFO;
        stats->console_enabled = 0;
        return;
    }

    acquire(&klog.lock);
    stats->total_generated = klog.total_generated;
    stats->stored = klog.stored;
    stats->overwritten = klog.overwritten;
    stats->console_emitted = klog.console_emitted;
    stats->buffered_entries = klog.count;
    stats->buffer_level = klog.buffer_level;
    stats->console_level = klog.console_level;
    stats->console_enabled = klog.console_enabled;
    release(&klog.lock);
}
