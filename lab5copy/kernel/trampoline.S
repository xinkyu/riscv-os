#ifndef __ASSEMBLER__
void uservec(void);
void userret(void);
#else
.section .text
.globl uservec
.globl userret
.align 4


uservec:
  
    csrrw sp, sscratch, sp
    
    
    sd ra, 40(sp)
    sd gp, 72(sp)
    sd tp, 80(sp)
    sd t0, 88(sp)
    sd t1, 96(sp)
    sd t2, 104(sp)
    sd s0, 112(sp)
    sd s1, 120(sp)
    sd a0, 128(sp)
    sd a1, 136(sp)
    sd a2, 144(sp)
    sd a3, 152(sp)
    sd a4, 160(sp)
    sd a5, 168(sp)
    sd a6, 176(sp)
    sd a7, 184(sp)
    sd s2, 192(sp)
    sd s3, 200(sp)
    sd s4, 208(sp)
    sd s5, 216(sp)
    sd s6, 224(sp)
    sd s7, 232(sp)
    sd s8, 240(sp)
    sd s9, 248(sp)
    sd s10, 256(sp)
    sd s11, 264(sp)
    sd t3, 272(sp)
    sd t4, 280(sp)
    sd t5, 288(sp)
    sd t6, 296(sp)
    
  
    csrr t0, sscratch
    sd t0, 16(sp)
    
   
    ld t1, 0(sp)
    csrw satp, t1
    sfence.vma zero, zero
    
  
    ld sp, 8(sp)
    
 
    call usertrap
    
userret:
 
    

    csrw satp, a0
    sfence.vma zero, zero
    
    mv sp, a1
    
    ld ra, 40(sp)
    ld gp, 72(sp)
    ld tp, 80(sp)
    ld t0, 88(sp)
    ld t1, 96(sp)
    ld t2, 104(sp)
    ld s0, 112(sp)
    ld s1, 120(sp)
    ld a0, 128(sp)
    ld a1, 136(sp)
    ld a2, 144(sp)
    ld a3, 152(sp)
    ld a4, 160(sp)
    ld a5, 168(sp)
    ld a6, 176(sp)
    ld a7, 184(sp)
    ld s2, 192(sp)
    ld s3, 200(sp)
    ld s4, 208(sp)
    ld s5, 216(sp)
    ld s6, 224(sp)
    ld s7, 232(sp)
    ld s8, 240(sp)
    ld s9, 248(sp)
    ld s10, 256(sp)
    ld s11, 264(sp)
    ld t3, 272(sp)
    ld t4, 280(sp)
    ld t5, 288(sp)
    ld t6, 296(sp)
    
  
    ld t0, 16(sp)
    csrw sscratch, t0
    
    sret
#endif