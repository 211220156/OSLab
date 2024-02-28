#include "klib.h"
#include "cte.h"
#include "proc.h"

#define PROC_NUM 64

static __attribute__((used)) int next_pid = 1;

proc_t pcb[PROC_NUM];
static proc_t *curr = &pcb[0];

void init_proc() {
  // Lab2-1, set status and pgdir
  pcb[0].status = RUNNING;
  pcb[0].pgdir = vm_curr();
  pcb[0].kstack = (void*)(KER_MEM - PGSIZE);
  // Lab2-4, init zombie_sem
  sem_init(&(pcb[0].zombie_sem), 0);
  // Lab3-2, set cwd
  pcb[0].cwd = iopen("/", TYPE_NONE);
}

proc_t *proc_alloc() {
  // Lab2-1: find a unused pcb from pcb[1..PROC_NUM-1], return NULL if no such one
  //TODO();
  // init ALL attributes of the pcb
  for (int i = 1; i < PROC_NUM; i++) {
    if (pcb[i].status == UNUSED) {
      pcb[i].pid = next_pid++;
      if (next_pid > 32767) next_pid = 1; //linux pid has a max value: 32767
      pcb[i].status = UNINIT;
      pcb[i].pgdir = vm_alloc();
      pcb[i].brk = 0;
      pcb[i].kstack = kalloc();
      pcb[i].ctx = &(pcb[i].kstack->ctx);
      pcb[i].parent = NULL;
      pcb[i].child_num = 0;
      pcb[i].cwd = NULL;
      //只要sem中没有进程被挂起，不用担心重复初始化sem。而因为进程的sem中只可能在
      //自己wait的时候挂起自己，所以一旦自己有空了，waitlist一定为空.而该进程被free时是自己调用
      //exit函数，所以waitlist一定没有进程。
      sem_init(&(pcb[i].zombie_sem), 0);
      for (int j = 0; j < MAX_USEM; j++) {
        pcb[i].usems[j] = NULL;
      }
      for (int j = 0; j < MAX_UFILE; j++) {
        pcb[i].files[j] = NULL;
      }
      return &pcb[i];
    }
  }
  return NULL;
}

void proc_free(proc_t *proc) {
  // Lab2-1: free proc's pgdir and kstack and mark it UNUSED
  //TODO();
  assert(proc != NULL);
  assert(proc->status != RUNNING);
  
  vm_teardown(proc->pgdir);
  kfree(proc->kstack);
  proc->status = UNUSED;
}

proc_t *proc_curr() {
  return curr;
}

void proc_run(proc_t *proc) {
  assert(proc != NULL);
  proc->status = RUNNING;
  curr = proc;
  set_cr3(proc->pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)STACK_TOP(proc->kstack));
  irq_iret(proc->ctx);
}

void proc_addready(proc_t *proc) {
  // Lab2-1: mark proc READY
  proc->status = READY;
}

void proc_yield() {
  // Lab2-1: mark curr proc READY, then int $0x81
  curr->status = READY;
  INT(0x81);
}

void proc_copycurr(proc_t *proc) {
  // Lab2-2: copy curr proc
  // Lab2-5: dup opened usems
  // Lab3-1: dup opened files
  // Lab3-2: dup cwd
  //TODO();
  vm_copycurr(proc->pgdir);
  proc_t *curr_proc = proc_curr();
  proc->brk = curr_proc->brk;
  //复制返回用户态的上下文。因为fork是系统调用，是从用户态到内核态的中断，返回用户态的上下文保存在内核栈栈顶
  assert(proc->kstack != NULL && curr_proc->kstack != NULL);
  *(proc->kstack) = *(curr_proc->kstack);
  proc->ctx->eax = 0;//系统调用的返回值
  proc->parent = curr_proc;
  curr_proc->child_num++;
  for (int i = 0; i < MAX_USEM; i++) {
    proc->usems[i] = curr_proc->usems[i];
    if (curr_proc->usems[i] != NULL)
      usem_dup(curr_proc->usems[i]);
  }
  for (int i = 0; i < MAX_UFILE; i++) {
    proc->files[i] = curr_proc->files[i];
    if (curr_proc->files[i] != NULL)
      fdup(curr_proc->files[i]);
  }
  if (curr_proc->cwd != NULL)
    proc->cwd = idup(curr_proc->cwd);
  else
    assert(curr_proc->cwd == NULL);
}

void proc_makezombie(proc_t *proc, int exitcode) {
  // Lab2-3: mark proc ZOMBIE and record exitcode, set children's parent to NULL
  // Lab2-5: close opened usem
  // Lab3-1: close opened files
  // Lab3-2: close cwd
  //TODO();
  proc->status = ZOMBIE;
  proc->exit_code = exitcode;
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].parent == proc) {
      pcb[i].parent = NULL;
    }
  }
  if (proc->parent != NULL) {
    sem_v(&(proc->parent->zombie_sem));
  }
  for (int i = 0; i < MAX_USEM; i++) {
    if (proc->usems[i] != NULL)
      usem_close(proc->usems[i]);
  }
  for (int i = 0; i < MAX_UFILE; i++) {
    if (proc->files[i] != NULL)
      fclose(proc->files[i]);
  }
  if (proc->cwd != NULL)
    iclose(proc->cwd);
  else
    assert(proc->cwd == NULL);
}

proc_t *proc_findzombie(proc_t *proc) {
  // Lab2-3: find a ZOMBIE whose parent is proc, return NULL if none
  //TODO();
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].parent == proc && pcb[i].status == ZOMBIE)
      return &pcb[i];
  }
  return NULL;
}

void proc_block() {
  // Lab2-4: mark curr proc BLOCKED, then int $0x81
  curr->status = BLOCKED;
  INT(0x81);
}

int proc_allocusem(proc_t *proc) {
  // Lab2-5: find a free slot in proc->usems, return its index, or -1 if none
  //TODO();
  for (int i = 0; i < MAX_USEM; i++) {
    if (proc->usems[i] == NULL)
      return i;
  }
  return -1;
}

usem_t *proc_getusem(proc_t *proc, int sem_id) {
  // Lab2-5: return proc->usems[sem_id], or NULL if sem_id out of bound
  //TODO();
  if (sem_id < 0 || sem_id >= MAX_USEM)
    return NULL;
  return proc->usems[sem_id];
}

int proc_allocfile(proc_t *proc) {
  // Lab3-1: find a free slot in proc->files, return its index, or -1 if none
  //TODO();
  for (int i = 0; i < MAX_UFILE; i++) {
    if (proc->files[i] == NULL) {
      return i;
    }
  }
  return -1;
}

file_t *proc_getfile(proc_t *proc, int fd) {
  // Lab3-1: return proc->files[fd], or NULL if fd out of bound
  //TODO();
  if (fd < 0 || fd >= MAX_UFILE)
    return NULL;
  return proc->files[fd];
}

void schedule(Context *ctx) {
  // Lab2-1: save ctx to curr->ctx, then find a READY proc and run it
  //TODO();
  proc_t *curr_proc = proc_curr();
  curr_proc->ctx = ctx;
  for (int i = 0; i < PROC_NUM; i++) {
    if (&pcb[i] == curr_proc) {
      for (int j = i + 1; j < PROC_NUM + i + 1; j++) {
        if (pcb[j % PROC_NUM].status == READY) {
          proc_run(&pcb[j % PROC_NUM]);
          break;
        }
      }
      break;
    }
  }
}
