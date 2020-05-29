// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct
{
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head[NBUCKET];
  struct spinlock lock[NBUCKET];
} bcache;

uint b_hash(uint blockno)
{
  return blockno % NBUCKET;
}

void binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUCKET; i++)
    initlock(&bcache.lock[i], "bcache.bucket");

  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; i++)
  {
    struct buf *head = &bcache.head[i];
    head->prev = head;
    head->next = head;
  }
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint _h = b_hash(blockno);
  uint h = _h;

  acquire(&bcache.lock[h]);
  for (b = bcache.head[h].next; b != &bcache.head[h]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[h]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  while (1)
  {
    h = (h + 1) % NBUCKET;
    if (h == _h)
      break;
    
    acquire(&bcache.lock[h]);

    // Not cached; recycle an unused buffer.
    for (b = bcache.head[h].prev; b != &bcache.head[h]; b = b->prev)
    {
      if (b->refcnt == 0)
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&bcache.lock[h]);
        b->next = bcache.head[_h].next;
        b->prev = &bcache.head[_h];
        b->next->prev = b;
        b->prev->next = b;
        release(&bcache.lock[_h]);
        acquiresleep(&b->lock);
        return b;
      }
    }

    release(&bcache.lock[h]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint h = b_hash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[h].next;
    b->prev = &bcache.head[h];
    bcache.head[h].next->prev = b;
    bcache.head[h].next = b;
  }
  release(&bcache.lock[h]);
}

void bpin(struct buf *b)
{
  uint h = b_hash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt++;
  release(&bcache.lock[h]);
}

void bunpin(struct buf *b)
{

  uint h = b_hash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;
  release(&bcache.lock[h]);
}
