# kernel/kernelvec.S

.section .text
.align 2
.global kernelvec # 声明为全局符号，供 trap.c 使用

kernelvec:
    # 1. 保存所有通用寄存器到栈上
    #    sp 已经由 entry.S 初始化，我们可以直接使用
    addi sp, sp, -256
    sd ra, 0(sp)
    sd sp, 8(sp) # 保存 sp 自身
    sd gp, 16(sp)
    sd tp, 24(sp)
    sd t0, 32(sp)
    sd t1, 40(sp)
    sd t2, 48(sp)
    sd s0, 56(sp)
    sd s1, 64(sp)
    sd a0, 72(sp)
    sd a1, 80(sp)
    sd a2, 88(sp)
    sd a3, 96(sp)
    sd a4, 104(sp)
    sd a5, 112(sp)
    sd a6, 120(sp)
    sd a7, 128(sp)
    sd s2, 136(sp)
    sd s3, 144(sp)
    sd s4, 152(sp)
    sd s5, 160(sp)
    sd s6, 168(sp)
    sd s7, 176(sp)
    sd s8, 184(sp)
    sd s9, 192(sp)
    sd s10, 200(sp)
    sd s11, 208(sp)
    sd t3, 216(sp)
    sd t4, 224(sp)
    sd t5, 232(sp)
    sd t6, 240(sp)

    # --- 实验六修改 ---
    # 将指向 trapframe 的指针 (即当前 sp) 
    # 放入 a0，作为参数传递给 kerneltrap
    mv a0, sp
    # --- 修改结束 ---

    # 2. 调用 C 语言实现的陷阱处理函数
    call kerneltrap

    # 3. 从栈上恢复所有通用寄存器
    ld ra, 0(sp)
    # ld sp, 8(sp) # 不恢复 sp，因为我们需要用它来弹出栈
    ld gp, 16(sp)
    ld tp, 24(sp)
    ld t0, 32(sp)
    ld t1, 40(sp)
    ld t2, 48(sp)
    ld s0, 56(sp)
    ld s1, 64(sp)
    ld a0, 72(sp)
    ld a1, 80(sp)
    ld a2, 88(sp)
    ld a3, 96(sp)
    ld a4, 104(sp)
    ld a5, 112(sp)
    ld a6, 120(sp)
    ld a7, 128(sp)
    ld s2, 136(sp)
    ld s3, 144(sp)
    ld s4, 152(sp)
    ld s5, 160(sp)
    ld s6, 168(sp)
    ld s7, 176(sp)
    ld s8, 184(sp)
    ld s9, 192(sp)
    ld s10, 200(sp)
    ld s11, 208(sp)
    ld t3, 216(sp)
    ld t4, 224(sp)
    ld t5, 232(sp)
    ld t6, 240(sp)
    addi sp, sp, 256

    # 4. 从 S 模式陷阱返回
    sret