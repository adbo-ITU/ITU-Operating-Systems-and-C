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
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define align(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (align(sizeof(size_t)))

#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

/* Pack a size and allocated bit into a word */
#define make_header(block_size, is_allocated) ((block_size) | (is_allocated))

/* Read and write a word at address p */
#define get_val(ptr) (*(unsigned int *)(ptr))
#define set_val(ptr, val) (*(unsigned int *)(ptr) = (val))

/* Read the size and allocated fields from pointer p */
#define get_size(ptr) (get_val(ptr) & ~0x7)
#define is_allocated(ptr) (get_val(ptr) & 0x1)
#define is_free(ptr) (!(is_allocated(ptr)))

/* Given block ptr bp, compute address of its header and footer. */
#define get_header_ptr(block_ptr) ((char *)(block_ptr)-WSIZE)
#define get_footer_ptr(block_ptr) ((char *)(block_ptr) + get_size(get_header_ptr(block_ptr)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define get_next_block_ptr(block_ptr) ((char *)(block_ptr) + get_size(get_header_ptr(block_ptr)))
#define get_prev_block_ptr(block_ptr) ((char *)(block_ptr)-get_size(((char *)(bp)-DSIZE)))

#define here (printf("%s:%d - %s\n", __FILE__, __LINE__, __func__))
#define dbg(val) printf("%s = 0x%x\n", #val, (unsigned int)val);

/*
 * Given the size of the data, calculates how many bytes are needed to store the
 * block:
 *   - 1 word for header
 *   - 1 word for footer
 *   - 8-byte aligned data size
 */
#define calc_block_size(data_size) (WSIZE + WSIZE + align(data_size));

#define max(x, y) ((x) > (y) ? (x) : (y))

/* Private functions in this file */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void place(void *ptr, size_t size);
static void heapdump();
static void mm_check();

/* Global (private) variables */
static void *heap_ptr = NULL;
static void *free_list = NULL;

static int MIN_BLOCK_SIZE = calc_block_size(1);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_ptr = mem_sbrk(4 * WSIZE);

    if (heap_ptr == (void *)-1)
    {
        return -1;
    }

    set_val(heap_ptr, 0);                                   /* Alignment padding */
    set_val(heap_ptr + (1 * WSIZE), make_header(DSIZE, 1)); /* Prologue header */
    set_val(heap_ptr + (2 * WSIZE), make_header(DSIZE, 1)); /* Prologue footer */
    set_val(heap_ptr + (3 * WSIZE), make_header(0, 1));     /* Epilogue header */

    heap_ptr += (2 * WSIZE); // Start heap at prologue footer

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    void *first_real_block_ptr = extend_heap(CHUNKSIZE / WSIZE);
    if (first_real_block_ptr == NULL)
        return -1;

    // Start free list at the new block we got after extending the heap
    free_list = first_real_block_ptr;
    // Set the initial payload (pointer to next free item) to NULL
    set_val(free_list, 0);

    here;
    heapdump();

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    printf("Allocate %d bytes\n", size);

    // Ignore spurious requests
    if (size == 0)
        return NULL;

    // Adjust block size to include overhead and alignment reqs.
    // Size will always be at least 1 due to the check above
    size_t block_size = calc_block_size(size);

    // Search for a fitting block
    void *block_ptr = free_list;
    void *prev_free_block_ptr = NULL;
    while (block_ptr != NULL)
    {
        if (get_size(get_header_ptr(block_ptr)) >= size)
            break;

        // Travel to the next free item, whose pointer is stored as the payload
        prev_free_block_ptr = block_ptr;
        block_ptr = (void *)get_val(block_ptr);
    }

    if (block_ptr != NULL)
    {
        // Make the previous free block point to the next free block
        if (prev_free_block_ptr != NULL)
            set_val(prev_free_block_ptr, get_val(block_ptr));
        else
            free_list = get_val(block_ptr);

        place(block_ptr, block_size);

        return block_ptr;
    }

    // No fit found. Get more memory and place the block
    size_t extend_size = max(block_size, CHUNKSIZE);
    if ((block_ptr = extend_heap(extend_size / WSIZE)) == NULL)
        return NULL;

    here;
    heapdump();

    dbg(prev_free_block_ptr);
    dbg(free_list);
    dbg(block_ptr);

    void *free_block_ptr = free_list;
    printf("\n");
    while (free_block_ptr != NULL)
    {
        printf("cmp %p == %p\n", block_ptr, get_val(free_block_ptr));
        if (block_ptr == get_val(free_block_ptr))
        {
            printf("Parent to %p found at %p\n", block_ptr, free_block_ptr);
        }

        free_block_ptr = (void *)get_val(free_block_ptr);
    }
    printf("\n");

    place(block_ptr, block_size);

    here;
    heapdump();

    exit(1);

    return block_ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    here;

    size_t size = get_size(get_header_ptr(ptr));

    set_val(get_header_ptr(ptr), make_header(size, 0));
    set_val(get_footer_ptr(ptr), make_header(size, 0));

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

/*
 * Places a block at the given block pointer. If the placed block leaves space
 * for another block after it, the existing block will be split in two.
 */
static void place(void *ptr, size_t size)
{
    here;
    dbg(ptr);

    size_t cur_size = get_size(get_header_ptr(ptr));
    size_t remaining_size = cur_size - size;

    // There is space for another block, so we split
    if (remaining_size >= MIN_BLOCK_SIZE)
    {
        set_val(get_header_ptr(ptr), make_header(size, 1));
        set_val(get_footer_ptr(ptr), make_header(size, 1));

        void *next_block_ptr = get_next_block_ptr(ptr);
        set_val(get_header_ptr(next_block_ptr), make_header(remaining_size, 0));
        set_val(get_footer_ptr(next_block_ptr), make_header(remaining_size, 0));

        dbg(next_block_ptr);
        dbg(free_list);

        // Add the next block to the free list
        set_val(next_block_ptr, free_list);
        free_list = next_block_ptr;
    }
    // There is not space for another block, so we don't split
    else
    {
        set_val(get_header_ptr(ptr), make_header(cur_size, 1));
        set_val(get_footer_ptr(ptr), make_header(cur_size, 1));
    }

    here;
}

/* Increases the size of the heap with the given number of words */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    set_val(get_header_ptr(bp), make_header(size, 0));                  /* Free block header */
    set_val(get_footer_ptr(bp), make_header(size, 0));                  /* Free block footer */
    set_val(get_header_ptr(get_next_block_ptr(bp)), make_header(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = is_allocated(get_footer_ptr(get_prev_block_ptr(bp)));
    size_t next_alloc = is_allocated(get_header_ptr(get_next_block_ptr(bp)));
    size_t size = get_size(get_header_ptr(bp));

    // When no surrounding blocks are free, we cannot coalesce
    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    // When only the next block is free
    else if (prev_alloc && !next_alloc)
    {
        size += get_size(get_header_ptr(get_next_block_ptr(bp)));
        set_val(get_header_ptr(bp), make_header(size, 0));
        set_val(get_footer_ptr(bp), make_header(size, 0));
    }
    // When only the previous block is free
    else if (!prev_alloc && next_alloc)
    {
        size += get_size(get_header_ptr(get_prev_block_ptr(bp)));
        set_val(get_footer_ptr(bp), make_header(size, 0));
        set_val(get_header_ptr(get_prev_block_ptr(bp)), make_header(size, 0));
        bp = get_prev_block_ptr(bp);
    }
    // When both surrounding blocks are free
    else
    {
        size += get_size(get_header_ptr(get_prev_block_ptr(bp))) +
                get_size(get_footer_ptr(get_next_block_ptr(bp)));
        set_val(get_header_ptr(get_prev_block_ptr(bp)), make_header(size, 0));
        set_val(get_footer_ptr(get_next_block_ptr(bp)), make_header(size, 0));
        bp = get_prev_block_ptr(bp);
    }
    return bp;
}

/* Debugging utilities */
static void print_block(void *ptr)
{
    unsigned int *next = (unsigned int *)get_next_block_ptr(ptr);
    printf("[%s] Addr: %p. Size: %d. Next: %p.\n",
           is_allocated(get_header_ptr(ptr)) ? "Allocated" : "Free     ",
           ptr, get_size(get_header_ptr(ptr)), next);
}

static void print_free_block(void *ptr)
{
    printf("[%s] Addr: %p. Size: %d. Payload: %p.\n",
           is_allocated(get_header_ptr(ptr)) ? "Allocated" : "Free     ",
           ptr, get_size(get_header_ptr(ptr)), (void *)get_val(ptr));
}

static void heapdump()
{
    printf("\n");
    printf("----------------- HEAP DUMP -----------------\n");

    printf("- Heap boundary: %p to %p\n", mem_heap_lo(), mem_heap_hi());
    printf("- Heap size: %d bytes\n", mem_heapsize());
    printf("- Heap start ptr: %p\n", heap_ptr);
    printf("- Free list start ptr: %p\n", free_list);

    printf("\nHeap dump:\n");

    // Print all blocks
    int n = 0;
    unsigned int *ptr = (unsigned int *)heap_ptr;
    while (get_size(get_header_ptr(ptr)) > 0)
    {
        printf("%d. ", n);
        print_block(ptr);

        ptr = (unsigned int *)get_next_block_ptr(ptr);
        n++;
    }
    printf("E. ");
    print_block(ptr);

    printf("\nFree list:\n");
    void *block_ptr = free_list;
    n = 0;
    while (block_ptr != NULL)
    {
        printf("%d. ", n);
        print_free_block(block_ptr);
        block_ptr = (void *)get_val(block_ptr);
        n++;
    }

    printf("--------------- END HEAP DUMP ---------------\n\n");
}

static void mm_check()
{
    void *block_ptr = free_list;
    while (block_ptr != NULL)
    {
        if (is_allocated(get_header_ptr(block_ptr)))
        {
            printf("ILLEGAL: allocated block detected on free list:\n   ");
            print_block(block_ptr);
        }

        block_ptr = (void *)get_val(block_ptr);
    }
}
