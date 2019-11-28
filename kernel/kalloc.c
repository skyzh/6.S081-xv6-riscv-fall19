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

struct kmem_t {
  struct spinlock lock;
  struct run *freelist;
} kmem[8];

void
kinit()
{
  /*
  initlock(&kmem[0].lock, "kmem0");
  initlock(&kmem[1].lock, "kmem1");
  initlock(&kmem[2].lock, "kmem2");
  initlock(&kmem[3].lock, "kmem3");
  initlock(&kmem[4].lock, "kmem4");
  initlock(&kmem[5].lock, "kmem5");
  initlock(&kmem[6].lock, "kmem6");
  initlock(&kmem[7].lock, "kmem7");*/
  for (int i = 0; i < NCPU; i++)
    initlock(&kmem[i].lock, "kmem");
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int hart = cpuid();
  pop_off();
  struct kmem_t* k = &kmem[hart];
  acquire(&k->lock);
  r->next = k->freelist;
  k->freelist = r;
  release(&k->lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int hart = cpuid();
  pop_off();
  struct kmem_t* k = &kmem[hart];

  acquire(&k->lock);
  r = k->freelist;
  if(r)
    k->freelist = r->next;
  release(&k->lock);

  if (!r) {
    for (int i = 0; i < NCPU; i++) {
      struct kmem_t* k = &kmem[i];
      acquire(&k->lock);
      r = k->freelist;
      if(r) {
        k->freelist = r->next;
        release(&k->lock);
        break;
      }
      release(&k->lock);
    }
  }
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
