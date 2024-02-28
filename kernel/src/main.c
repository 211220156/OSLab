#include "klib.h"
#include "serial.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "fs.h"
#include "proc.h"
#include "timer.h"
#include "dev.h"

void init_user_and_go();

int main() {
  init_gdt();
  init_serial();
  init_fs();
  init_page(); // uncomment me at Lab1-4
  init_cte(); // uncomment me at Lab1-5
  init_timer(); // uncomment me at Lab1-7
  init_proc(); // uncomment me at Lab2-1
  init_dev(); // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go() {
  // Lab1-2: ((void(*)())eip)();
  // Lab1-4: pdgir, stack_switch_call
  // Lab1-6: ctx, irq_iret
  // Lab1-8: argv
  // Lab2-1: proc
  // Lab3-2: add cwd

//  uint32_t eip = load_elf(NULL, "loaduser");
//  assert(eip != -1);
//  ((void(*)())eip)();
/*
  PD *pgdir = vm_alloc();
//  uint32_t eip = load_elf(pgdir, "loaduser");
//  uint32_t eip = load_elf(pgdir, "pgfault");
  uint32_t eip = load_elf(pgdir, "systest");
  printf("\n0x%x\n", eip);
  assert(eip != -1);
  set_cr3(pgdir);
  stack_switch_call((void*)(USR_MEM - 16), (void*)eip, 0);
*/
/*  ----------before lab2-1----------
  PD *pgdir = vm_alloc();
  Context ctx;
//  char *argv[] = {"echo", "hello", "world", NULL};
  char *argv[] = {"sh1", NULL};
  assert(load_user(pgdir, &ctx, "sh1", argv) == 0);
  set_cr3(pgdir);
  //参数2是栈顶，内核栈从高地址向低地址生长,即分配了一个页面给内核栈
  set_tss(KSEL(SEG_KDATA), (uint32_t)kalloc() + PGSIZE);
  irq_iret(&ctx);
*/

/* 
  proc_t *proc = proc_alloc();
  assert(proc);
  char *argv[] = {"sh1", NULL};
  assert(load_user(proc->pgdir, proc->ctx, "sh1", argv) == 0);
  //proc_run(proc);
  proc_addready(proc);
  while (1) {
    proc_yield();
    printf("proc is changing!!\n");
  }
*/
  proc_t *proc = proc_alloc();
  proc->cwd = iopen("/", TYPE_NONE);
  assert(proc);
//  char *argv[] = {"ping3", "114514", "1919810", NULL};
//  char *argv[] = {"childtest", "1", "10", "25", NULL};
  char *argv[] = {"sh", NULL};
  assert(load_user(proc->pgdir, proc->ctx, "sh", argv) == 0);
  proc_addready(proc);

  //while (1) proc_yield();
  sti();//开中断，避免所有用户进程都在关中断的内核态下，内核进程也一致处于proc_yield的关中断状态，系统处理不了外部中断
  while (1);

}
