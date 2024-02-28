#include "klib.h"
#include "vme.h"
#include "proc.h"

static TSS32 tss;

void init_gdt() {
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_USER);
  gdt[SEG_TSS]   = SEG16(STS_T32A,     &tss,  sizeof(tss)-1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0) {
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;
static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used));

typedef union free_page {
  union free_page *next;
  char buf[PGSIZE];
} page_t; // union free_page 是捆在一起的一个类型
static page_t *free_page_list;

void init_page() {
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");
  // Lab1-4: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
  //TODO();
  for (int i = 0; i < 32; i++) {
    kpd.pde[i].val = MAKE_PDE(&kpt[i], 1);//页表地址是这个吗？
    for (int j = 0; j < NR_PTE; j++) {
      kpt[i].pte[j].val = MAKE_PTE(((i << DIR_SHIFT) | (j << TBL_SHIFT)), 1);
//      kpt[i].pte[j].present = 0;
    }
  }

  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  //TODO();
  free_page_list = (page_t*)KER_MEM;
  for (int i = 1; i < (PHY_MEM - KER_MEM) / PGSIZE; i++) {
    page_t *tmp = (page_t *)(KER_MEM + i * PGSIZE);
    free_page_list->next = tmp;
    int dir = ADDR2DIR(free_page_list), tbl = ADDR2TBL(free_page_list);
    kpt[dir].pte[tbl].present = 0;
    free_page_list = tmp;
  }
  free_page_list->next = NULL;
  free_page_list = (page_t*)KER_MEM;
}

void *kalloc() {
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
  //TODO();
 /* if ((uint32_t)free_page_list >= PHY_MEM) {
    assert(0);
  }*/
  assert((uint32_t)free_page_list <= PHY_MEM);
  assert((uint32_t)free_page_list % 0x1000 == 0);
  page_t *fp = free_page_list;
  int dir = ADDR2DIR(fp), tbl = ADDR2TBL(fp);
  kpt[dir].pte[tbl].present = 1;//分配页时p位置1
  free_page_list = free_page_list->next;
  memset((void *)fp, 0, sizeof(page_t));
  return (void *)fp;
}

void kfree(void *ptr) {
  // Lab1-4: free a page to kernel heap
  // you can just do nothing :)
  //TODO();
  //assert((uint32_t)ptr >= PHY_MEM);
  
  assert((uint32_t)ptr % 0x1000 == 0);
  memset(ptr, 0x3f, PGSIZE);
  ((page_t *)ptr)->next = free_page_list;
  free_page_list = (page_t *)ptr;
  int dir = ADDR2DIR(free_page_list), tbl = ADDR2TBL(free_page_list);
  kpt[dir].pte[tbl].present = 0;
  set_cr3(vm_curr());
}

PD *vm_alloc() {
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
  //TODO();
  PD *pd = kalloc();
  for (int i = 0; i < 32; i++) {
    pd->pde[i].val = MAKE_PDE(&kpt[i], 1);
  }
  for (int i = 32; i < NR_PDE; i++) {
    pd->pde[i].val = 0;
  }
  return pd;
}

void vm_teardown(PD *pgdir) {
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  //TODO();
  for (int i = 32; i < NR_PDE; i++) {
    if (pgdir->pde[i].present == 1) {
      PT *pt = PDE2PT(pgdir->pde[i]);
      for (int j = 0; j < NR_PTE; j++) {
        if (pt->pte[j].present == 1) {
          void* page = PTE2PG(pt->pte[j]);
          kfree(page);
          pt->pte[j].present = 0;
        }
      }
      kfree((void*)pt);
      pgdir->pde[i].present = 0;
    }
  }
  kfree((void*)pgdir);
}

PD *vm_curr() {
  return (PD*)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot) {
  // Lab1-4: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  assert((prot & ~7) == 0);
  //TODO();
  assert(va >= PHY_MEM);//可以认为va一定不低于PHY_MEM（即它对应的页表和物理页如果存在一定是kalloc出来的）
  int pd_index = ADDR2DIR(va);
  PDE *pde = &(pgdir->pde[pd_index]);
  //printf("pde->val = 0x%x\n", pde->val);
  if (pde->present == 0) {
    if (prot != 0) {
      pde->val = MAKE_PDE(kalloc(), prot);
      PT *pt = PDE2PT(*pde); // 根据PDE找页表的地址
      int pt_index = ADDR2TBL(va); // 计算“页表号”
      PTE *pte = &(pt->pte[pt_index]);
      return pte;
    }
    else { //prot == 0
      return NULL;
    }
  }
  else { //pde->present != 0
    PT *pt = PDE2PT(*pde); // 根据PDE找页表的地址
    int pt_index = ADDR2TBL(va); // 计算“页表号”
    PTE *pte = &(pt->pte[pt_index]);
    return pte;
  }
}


void *vm_walk(PD *pgdir, size_t va, int prot) {
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  //TODO();
  assert(va >= PHY_MEM);
  PTE* pte = vm_walkpte(pgdir, va, prot);
  if (pte->present == 0)//没被映射
    return NULL;
  void *page = PTE2PG(*pte); // 根据PTE找物理页的地址
  void *pa = (void*)((uint32_t)page | ADDR2OFF(va));
  return pa;
}

void vm_map(PD *pgdir, size_t va, size_t len, int prot) {
  // Lab1-4: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(start >= PHY_MEM);
  assert(end >= start);
  //TODO();
  for (size_t i = 0; i < len; i += 0x1000) { //遍历每个虚拟页
    PTE* currPTE = vm_walkpte(pgdir, start + i, prot);
    if (currPTE->present == 1)  continue;
    void *paddr = kalloc();
    currPTE->val = MAKE_PTE(paddr, prot);
  }
}

void vm_unmap(PD *pgdir, size_t va, size_t len) {
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  assert(ADDR2OFF(va) == 0);
  assert(ADDR2OFF(len) == 0);
  assert(va >= PHY_MEM);
  //TODO();
  for (size_t i = 0; i < len; i += 0x1000) {
    int dir = ADDR2DIR(va + i), tbl = ADDR2TBL(va + i);
    if (pgdir->pde[dir].present == 1) {
      PT *pt = PDE2PT(pgdir->pde[dir]);
      if (pt->pte[tbl].present == 1) {
        PTE *currPTE = &(pt->pte[tbl]);
        void *page = PTE2PG(*currPTE); // 根据PTE找物理页的地址
        void *pa = (void*)((uint32_t)page | ADDR2OFF(va + i));
        kfree(pa);
        currPTE->val = 0;
      }
    }
    
  }
  if (pgdir == vm_curr())
    set_cr3(vm_curr());
}

void vm_copycurr(PD *pgdir) {
  // Lab2-2: copy memory mapped in curr pd to pgdir
  //TODO();
  PD *curr = vm_curr();
  for (size_t va = PHY_MEM; va < USR_MEM; va += 0x1000) {
    PTE *pte = vm_walkpte(curr, va, 0);
    if (pte != NULL && pte->present == 1) {//this page need to be copy and add mapping in pgdir
      int prot = pte->val & 7;
      vm_map(pgdir, va, 0x1000, prot);
      void *pa = vm_walk(pgdir, va, prot);
      // 根据PTE找原虚拟页对应物理页的地址
      void *page = PTE2PG(*pte);
      void *old_pa = (void*)((uint32_t)page | ADDR2OFF(va));
      memcpy(pa, old_pa, 0x1000);
    }
  }
}

void vm_pgfault(size_t va, int errcode) {
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
