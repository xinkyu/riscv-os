# 实验三：页表与内存管理

## 实验目标

本实验的目标是为我们的内核构建两项核心的内存管理功能：一个简单的物理内存页分配器和一个符合 RISC-V Sv39 规范的页表管理系统。最终，内核将能够启用虚拟内存并在虚拟地址空间中运行。

## 核心实现

### 1. 物理内存分配器 (`kernel/kalloc.c`)

该模块负责管理以页（4KB）为单位的物理内存。

* **数据结构**: 我们采用了一种高效且简洁的方法来管理空闲内存：将所有空闲的物理页组织成一个**侵入式链表**。每个空闲页的起始位置被强制转换为 `struct run` 结构体，其中包含指向下一个空闲页的指针。
* **初始化 (`kinit` & `freerange`)**: 内核初始化时，`kinit` 函数调用 `freerange`。`freerange` 负责将从内核镜像末尾（由链接器符号 `end` 指定）到物理内存顶端（0x88000000，即 128MB）的所有页面逐一释放到空闲链表中。
* **分配 (`kalloc`)**: 分配一个物理页时，只需从空闲链表的头部取下一个节点，并返回其地址。
* **释放 (`kfree`)**: 释放一个物理页时，将其添加到空闲链表的头部。

### 2. 虚拟内存 (`kernel/vm.c`)

该模块实现了 RISC-V Sv39 三级页表机制。

* **页表遍历 (`walk`)**: 这是页表操作的核心函数。它接收一个根页表的地址和一个虚拟地址，然后根据虚拟地址的高 27 位（分为三级，每级 9 位），逐级查找或创建页表。如果中间级的页表不存在且 `alloc` 参数为 1，则会调用 `kalloc` 分配一个新页表。
* **地址映射 (`mappages`)**: 该函数用于建立一段虚拟地址到物理地址的映射。它在一个循环中，对指定范围内的每个页面调用 `walk` 函数找到对应的 PTE，然后填入物理地址和权限标志。
* **内核页表创建 (`kvminit`)**: 在 `kmain` 中调用，负责创建内核自身的页表。它为几个关键的内存区域建立了映射：
    1. **UART 设备**: 将物理地址 `0x10000000` 恒等映射，权限为 R/W。
    2. **内核代码段 (`.text`)**: 从 `0x80000000` 到 `etext`，恒等映射，权限为 R/X（只读、可执行）。
    3. **内核数据段和剩余内存**: 从 `etext` 到 `PHYSTOP`，恒等映射，权限为 R/W。
* **激活 MMU (`kvminithart`)**: 这是启用虚拟内存的最后一步。它将内核根页表的物理地址写入 `satp` 寄存器，然后执行 `sfence.vma` 指令来清空 TLB，使新的页表映射生效。

### 3. 测试功能 (`kernel/test.c`)

为了验证内存管理功能的正确性，实现了以下测试：

* **测试辅助函数**:
    - `create_pagetable()`: 创建一个新的空页表
    - `map_page()`: 映射单个页面
    - `walk_lookup()`: 查找虚拟地址对应的PTE（不分配）
    - `assert()`: 简单的断言实现

* **测试1：物理内存分配器测试 (`test_physical_memory`)**
    - 测试基本分配和释放
    - 验证页对齐（地址末尾为 0x000）
    - 测试数据写入和读取
    - 测试释放后重新分配

* **测试2：页表操作测试 (`test_pagetable`)**
    - 创建新页表
    - 测试虚拟地址到物理地址映射
    - 验证地址转换功能
    - 检查权限位设置（R/W/X）

* **测试3：虚拟内存激活测试 (`test_virtual_memory`)**
    - 验证启用分页前后的系统状态
    - 确保内核代码仍可执行
    - 确保内核数据仍可访问
    - 确保设备访问（UART）正常工作

## 实验结果

运行 `make run` 后，内核依次完成：

1. **物理内存分配器初始化**
   ```
   kinit: physical memory allocator initialized.
   ```

2. **Lab3 测试套件**
   ```
   === Test 1: Physical Memory Allocator ===
    Page allocation and alignment test passed
    Memory write test passed
    Free and reallocation test passed

   === Test 2: Page Table Operations ===
    Page table created
    Basic mapping test passed
    Address translation test passed
    Permission bits test passed

   === Test 3: Virtual Memory Activation ===
   kvminit: kernel page table created.
   kvminithart: virtual memory enabled.
    Kernel code still executable
    Kernel data still accessible
    Device access still working
   ```

3. **所有测试通过**
   ```
   ===== Lab3 Tests Passed! =====
   ```

内核在虚拟地址空间中稳定运行，证明了内存管理系统的正确性。

## 技术要点

### RISC-V Sv39 分页机制

- **三级页表结构**: 39位虚拟地址 = 27位VPN (9+9+9) + 12位页内偏移
- **页表项格式**: 
  - [63:54] 保留
  - [53:10] 物理页号 (PPN)
  - [9:0] 标志位 (V, R, W, X, U, G, A, D)
- **地址转换**: 
  - Level 2: VPN[2] 索引根页表
  - Level 1: VPN[1] 索引二级页表
  - Level 0: VPN[0] 索引三级页表
  - 最终得到 PTE，提取 PPN 拼接页内偏移得到物理地址

### 关键数据结构

```c
typedef uint64 pte_t;          // 页表项
typedef uint64 *pagetable_t;   // 页表指针

#define PGSIZE 4096            // 页大小 4KB
#define PTE_V (1L << 0)        // Valid
#define PTE_R (1L << 1)        // Readable
#define PTE_W (1L << 2)        // Writable
#define PTE_X (1L << 3)        // Executable
```

### 内存布局

```
物理内存布局 (恒等映射):
0x80000000 -----> 内核代码段 (.text)     [R-X]
0x80xxxxxx -----> 内核数据段 (.data/.bss) [RW-]
...
0x10000000 -----> UART 设备               [RW-]
...
0x88000000 -----> 物理内存顶端
```

## 编译与运行

```bash
cd lab3
make clean
make run
```

退出 QEMU: `Ctrl-A` 然后按 `X`

## 文件结构

```
lab3/
├── kernel/
│   ├── entry.S          # 启动入口
│   ├── main.c           # 内核主函数
│   ├── kalloc.c         # 物理内存分配器
│   ├── vm.c             # 虚拟内存管理
│   ├── test.c           # 测试套件（新增）
│   ├── trap.c           # 陷阱处理框架
│   ├── printf.c         # 格式化输出（支持 %p）
│   ├── console.c        # 控制台驱动
│   ├── uart.c           # UART 驱动
│   ├── defs.h           # 函数声明
│   ├── riscv.h          # RISC-V 定义
│   └── kernel.ld        # 链接脚本
├── Makefile
└── ReadMe.md
```

## 关键改进

1. **完善的测试框架**: 添加了完整的测试套件，验证所有内存管理功能
2. **辅助函数**: 导出 `create_pagetable`, `map_page`, `walk_lookup` 供测试使用
3. **printf 增强**: 支持 `%p` 格式打印指针地址
4. **类型定义**: 在 `defs.h` 中包含 `riscv.h` 以提供类型定义

## 调试建议

1. **验证页对齐**: 使用 `assert(((uint64)addr & 0xFFF) == 0)`
2. **检查映射**: 使用 `walk_lookup` 验证虚拟地址映射
3. **GDB 调试**: 
   ```bash
   make qemu-gdb  # 终端1
   riscv64-unknown-elf-gdb kernel.elf  # 终端2
   ```

## 参考资料

- RISC-V Privileged ISA Specification
- xv6-riscv 源码
- QEMU RISC-V virt machine 文档

---
