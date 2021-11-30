// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

#define PHYPGNUM ((PGROUNDDOWN(PHYSTOP)-PGROUNDUP(KERNBASE))/PGSIZE)
#define PA2IDX(pa) ((PGROUNDDOWN((pa))-PGROUNDUP(KERNBASE))/PGSIZE)

struct spinlock reflock;
uchar refcnt[PHYPGNUM]={0};

void
kinit()
{
  memset(refcnt, 0, sizeof(refcnt));
  initlock(&reflock, "ref");
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  int idx = PA2IDX((uint64)pa);
  acquire(&reflock);
  if(refcnt[idx] > 0){
    refcnt[idx]--;
    if(refcnt[idx] != 0){
      release(&reflock);
      return;
    }
  }
  release(&reflock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&reflock);
    refcnt[PA2IDX((uint64)r)] = 1;
    release(&reflock);
  }
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}

int
kref(uint64 pa){
  if(pa >= PHYSTOP){
    printf("kref: address over PHYSTOP");
    return -1;
  }
  if(pa < KERNBASE){
    printf("kref: address below KERNBASE");
    return -1;
  }
  uint64 idx = PA2IDX(pa);
  acquire(&reflock);
  refcnt[idx]++;
  release(&reflock);
  if(refcnt[idx]==0){
    panic("kref: max references");
    return -1;
  }
  return 0;
}

int
kgetref(uint64 pa){
  if(pa >= PHYSTOP){
    printf("kgetref: address over PHYSTOP");
    return -1;
  }
  if(pa < KERNBASE){
    printf("kgetref: address below KERNBASE");
    return -1;
  }
  uint64 idx = PA2IDX(pa);
  return (int)(refcnt[idx]);
}