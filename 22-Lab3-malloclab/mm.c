/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
  /* Team name */
  "adbo",
  /* First member's full name */
  "Adrian Borup",
  /* First member's email address */
  "adbo@itu.dk",
  /* Second member's full name (leave blank if none) */
  "",
  /* Second member's email address (leave blank if none) */
  ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define MIN_BLOCK_SIZE (2 * DSIZE) /* Blocks must at least be 2x double words */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from pointer p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *heap_listp = NULL;

void print_block(void *ptr) {
    unsigned int* next = (unsigned int*) NEXT_BLKP(ptr);
    printf("[%s] Addr: %p. Size: %d. Next: %p.\n",
      GET_ALLOC(HDRP(ptr)) ? "Allocated" : "Free     ",
      ptr, GET_SIZE(HDRP(ptr)), next);
}

void heapdump() {
  printf("\n");
  printf("----------------- HEAP DUMP -----------------\n");

  printf("- Heap boundary: %p to %p\n", mem_heap_lo(), mem_heap_hi());
  printf("- Heap size: %d bytes\n", mem_heapsize());
  printf("- Heap start ptr: %p\n", heap_listp);

  printf("\nHeap dump:\n");

  // Print all words
  // int n = 0;
  // unsigned int* p = (unsigned int*) mem_heap_lo();
  // while ((void *) p <= (unsigned int*) mem_heap_hi() +  1) {
  //   printf("[%4d]  %p: 0x%x\n", n, p, *p);
  //   p += 1;
  //   n += 1;
  // }

  // Print all blocks
  int n = 0;
  unsigned int* ptr = (unsigned int*) heap_listp;
  while (GET_SIZE(HDRP(ptr)) > 0) {
    printf("%d. ", n);
    print_block(ptr);

    ptr = (unsigned int*) NEXT_BLKP(ptr);
    n++;
  }
  printf("E. ");
  print_block(ptr);

  printf("--------------- END HEAP DUMP ---------------\n\n");
}

static void *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) {
		return bp;
	}
  else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT (FTRP(bp), PACK(size,0));
	}
  else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
  else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
			GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
  PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  heap_listp = mem_sbrk(4 * WSIZE);

  if (heap_listp == (void *) -1) {
    return -1;
  }

  PUT(heap_listp, 0);                            /* Alignment padding */
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */

  heap_listp += (2 * WSIZE); // Start heap at prologue footer

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;
  }

  return 0;
}

/* Searches the heap for a block large enough to hold `size` using first-fit. */
static void *find_fit(size_t size) {
  void *ptr = heap_listp;

  // All blocks are at least 1 in size except the epilogue (end of heap)
  while (GET_SIZE(HDRP(ptr)) > 0) {
    if (!GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)) >= size) {
      return ptr;
    }

    ptr = NEXT_BLKP(ptr);
  }

  return NULL;
}

static void place(void *ptr, size_t size) {
  size_t cur_size = GET_SIZE(HDRP(ptr));
  size_t remaining_size = cur_size - size;

  // There is space for another block, so we split
  if (remaining_size >= MIN_BLOCK_SIZE) {
    PUT(HDRP(ptr), PACK(size, 1));
    PUT(FTRP(ptr), PACK(size, 1));

    PUT(HDRP(NEXT_BLKP(ptr)), PACK(remaining_size, 0));
    PUT(FTRP(NEXT_BLKP(ptr)), PACK(remaining_size, 0));
  }
  // There is not space for another block, so we don't split
  else {
    PUT(HDRP(ptr), PACK(cur_size, 1));
    PUT(FTRP(ptr), PACK(cur_size, 1));
  }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  size_t asize;	/* Adjusted block size */
  size_t extendsize;	/* Amount to extend heap if no fit */
  char *bp;

  /* Ignore spurious requests */
  if (size == 0)
    return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
	  return NULL;

	place(bp, asize);

	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

//void mm_check() {
//  printf("- Heap boundary: %p to %p\n", mem_heap_lo(), mem_heap_hi());
//  printf("- Heap size: %d\n", mem_heapsize());
//  printf("- Heap start ptr: %p\n", heap_listp);
//}

