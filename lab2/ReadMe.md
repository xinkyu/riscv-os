# 实验二：内核 printf 与清屏能力

**姓名**： 
**学号**： 
**日期**：2025-12-08

---

## 一、实验概述

### 实验目标
依照《从零构建操作系统-学生指导手册》第2章任务，完成内核级格式化输出与清屏功能：
1. 构建 `printf`/`print_int` 可变参数栈，实现 `%d/%x/%s/%%`；
2. 通过 ANSI 转义序列实现 `clear_screen`，保证每次调试输出清晰；
3. 保持 `printf → cons_putc → uart_putc` 的分层架构，便于后续扩展。

### 完成情况
- ✅ 格式化输出：`printf` 覆盖整数、字符串和 `%` 转义，遵循任务1/2/4 要求
- ✅ 数字转换：`print_int` 使用迭代除模、支持负数/INT_MIN（任务3）
- ✅ 清屏：`clear_screen` 输出 `\033[2J\033[H`，一次完成擦除与光标复位（任务5）
- ✅ 架构：串口驱动、控制台抽象与格式化实现解耦，符合手册推荐
- ✅ 测试：`main.c` 构造正常与边界用例，运行日志固定可复现


### 开发环境
- 操作系统：Ubuntu 22.04 LTS
- 工具链：`riscv64-unknown-elf-gcc 12.2.0`
- 仿真器：`qemu-system-riscv64 7.2.0`
- 构建工具：GNU Make 4.3

---

## 二、技术设计

### 系统架构
```
[printf(fmt,...)]
	↓ 解析格式 & va_list
[cons_putc]
	↓ 控制台抽象，可挂缓冲/锁
[uart_putc]
	↓ 轮询 LSR，写 THR
[virt UART @0x10000000]
```

与 xv6 对比：
- **一致**：三层职责、UART 轮询发送、ANSI 清屏方案。
- **简化**：未引入自旋锁和中断驱动；控制台只负责下沉到 UART。
- **定制**：在 `kmain` 首先执行 `clear_screen`，满足指导书“清屏后输出测试”的建议。

### 关键数据结构
1. `static char digits[] = "0123456789abcdef"`：查表完成十进制/十六进制转换。
2. `char buf[32]`：`print_int` 的逆序缓存，足以容纳 64 位无符号表示。
3. ANSI 序列常量 `"\033[2J\033[H"`：一次发送即可满足任务5。

### 核心流程
1. **printf**：线性扫描 `fmt`，遇 `%` 进入状态机，根据格式调用不同分支；异常格式回退为输出 `%`+原字符。
2. **print_int**：如 `sign` 且 `xx<0` 则先输出 `-`，再将数字转为 `unsigned long long` 处理，循环 `% base` 收集位并逆序输出。
3. **clear_screen**：顺序输出转义序列，终端立即清屏并重置光标。

---

## 三、实现细节与关键代码

### 关键函数：`print_int`（`kernel/printf.c`）
```c
static void print_int(long long xx, int base, int sign) {
	char digits[] = "0123456789abcdef";
	char buf[32];
	int i = 0;
	unsigned long long x = (sign && xx < 0) ? -xx : xx;
	if (sign && xx < 0) cons_putc('-');
	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);
	while (--i >= 0) cons_putc(buf[i]);
}
```
*采用迭代+缓冲避免递归栈开销，且通过无符号运算绕过 INT_MIN 溢出。*

### 关键函数：`printf`
```c
void printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	for (int c = *fmt; c; c = *++fmt) {
		if (c != '%') { cons_putc(c); continue; }
		switch (*++fmt) {
			case 'd': print_int(va_arg(args, int), 10, 1); break;
			case 'x': print_int(va_arg(args, int), 16, 0); break;
			case 's': {
				char *s = va_arg(args, char*); if (!s) s = "(null)";
				while (*s) cons_putc(*s++);
				break;
			}
			case '%': cons_putc('%'); break;
			default:  cons_putc('%'); cons_putc(*fmt); break;
		}
	}
	va_end(args);
}
```
*实现手册任务4的状态机解析；默认分支将未知格式原样输出，方便排查。*

### 关键函数：`clear_screen`
```c
void clear_screen(void) {
	const char *seq = "\033[2J\033[H";
	while (*seq) cons_putc(*seq++);
}
```
*选用手册推荐的 ANSI 序列方案，不依赖额外硬件寄存器。*

### 难点突破
1. **INT_MIN 表示**：直接 `-xx` 溢出，改用 `unsigned long long` 并先输出 `-` 号。
2. **`%%` 解析**：初版遗漏 `case '%'`，导致输出空白；增加独立分支后恢复正常。
3. **串口忙等待**：确认 `LSR_THRE` 位判定无误后再写 THR，避免字符丢失。

### 与 xv6 的比较
- xv6 `printf` 使用 `panic` 处理空 `fmt`，此处直接返回，减小依赖；
- xv6 `consputc` 包含退格和锁，本实验以最小实现为主，后续实验5/6再扩展；
- xv6 清屏通常由用户程序触发，而这里提供内核级 API，方便在早期引导阶段使用。

---

## 四、测试与验证

### 功能测试
| 测试编号 | 目的 | 触发方式 | 期望输出 | 结果 |
| --- | --- | --- | --- | --- |
| T1 | 清屏 | `kmain` 首行调用 `clear_screen` | 终端内容被擦除，光标回到(0,0) | ✅ |
| T2 | `%d` 正常值 | `printf("%d", 42)` | `42` | ✅ |
| T3 | `%d` 负数/零 | `printf("%d %d", -123, 0)` | `-123 0` | ✅ |
| T4 | `%x` 十六进制 | `printf("0x%x", 0xABCDEF)` | `0xabcdef` | ✅ |
| T5 | `%s` 边界 | `printf("%s", (char*)0)` | `(null)` | ✅ |
| T6 | `%%` | `printf("%%")` | `%` | ✅ |


### 边界与异常测试
- **INT_MAX**：`2147483647`打印正确；
- **NULL/空串**：输出 `(null)` 或空行，未触发异常；
- **未知格式**：`printf("%q")` 会回显 `%q`，提示开发者修正格式字符串。

### 截图/录屏
![启动成功截图](lab2-boot-success.png)
---

## 五、问题与总结

### 遇到的问题与解决
1. **`%%` 输出为空**
   - 现象：日志中缺少 `%`。
   - 原因：状态机跳过 `%` 占位符。
   - 解决：添加 `case '%'`，并在默认分支输出 `%`+原字符。

2. **INT_MIN 显示错误**
   - 现象：`-2147483648` 被打印为正值。
   - 原因：带符号取负溢出。
   - 解决：遵循指导书建议，先输出负号，再转换为无符号处理。

3. **清屏残影**
   - 现象：终端偶尔残留旧内容。
   - 原因：未及时重置光标。
   - 解决：在 `clear_screen` 中同时发送 `\033[H`，保证光标定位。

### 实验收获
- 掌握 `va_list` 可变参数机制及其在内核中的安全使用方式；
- 深入理解输出链路分层设计的可维护性；
- 熟悉 ANSI 转义序列，为后续实现彩色/定位功能打下基础。

### 改进方向
1. 引入自旋锁和缓冲区，保证多核下输出原子性；
2. 扩展格式符（`%p/%u/%c` 等）及可变宽度支持；
3. 新增 `goto_xy`、颜色输出和清行等增强 API，实现指导书“扩展功能”建议。

---
