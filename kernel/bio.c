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
#define HASH(blkid) ((blkid) % NBUCKET)

struct bucket{
  struct buf head;
  struct spinlock lock;
};

struct {
  struct buf bufs[NBUF];
  struct bucket hashtable[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  char tmp[20];
  
  for(int i=0; i<NBUCKET; i++){
    snprintf(tmp, 9, "bcache-%d", i);
    initlock(&bcache.hashtable[i].lock, tmp);
    
    bcache.hashtable[i].head.prev \
      = bcache.hashtable[i].head.next \
      = &bcache.hashtable[i].head;
  }

  for(b = bcache.bufs; b < bcache.bufs+NBUF; b++){
    b->next = bcache.hashtable[0].head.next;
    b->prev = &bcache.hashtable[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hashtable[0].head.next->prev = b;
    bcache.hashtable[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = HASH(blockno);

  acquire(&bcache.hashtable[idx].lock);

  // Is the block already cached?
  for(b = bcache.hashtable[idx].head.next; b != &bcache.hashtable[idx].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.hashtable[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int j=0; j<NBUCKET; j++){
    int i = (idx+j) % NBUCKET;

    if(i != idx)
      acquire(&bcache.hashtable[i].lock);
    
    b = 0;
    struct buf* tmp;
    for(tmp = bcache.hashtable[i].head.next; tmp != &bcache.hashtable[i].head; tmp = tmp->next){
      if(tmp->refcnt == 0 &&
          (b == 0 || tmp->timestamp < b->timestamp))
        b = tmp;
    }
    
    if(b){
      if(i != idx){
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.hashtable[i].lock);

        b->next = bcache.hashtable[idx].head.next;
        b->prev = &bcache.hashtable[idx].head;
        bcache.hashtable[idx].head.next->prev = b;
        bcache.hashtable[idx].head.next = b;
      }

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.hashtable[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }else{
      if(i != idx)
        release(&bcache.hashtable[i].lock);
    }
  }
  
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int idx = HASH(b->blockno);

  acquire(&bcache.hashtable[idx].lock);
  b->refcnt--;

  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);

  release(&bcache.hashtable[idx].lock);
}

void
bpin(struct buf *b) {
  int idx = HASH(b->blockno);
  acquire(&bcache.hashtable[idx].lock);
  b->refcnt++;
  release(&bcache.hashtable[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = HASH(b->blockno);
  acquire(&bcache.hashtable[idx].lock);
  b->refcnt--;
  release(&bcache.hashtable[idx].lock);
}


