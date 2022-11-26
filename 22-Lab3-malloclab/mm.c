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

// Given a block pointer to a free block, get the free block it points to as the next one
#define get_next_free_block_ptr(block_ptr) ((void *)get_val(block_ptr))
// Given a block pointer to a free block, get the free block it points to as the previous one
#define get_prev_free_block_ptr(block_ptr) ((void *)get_val((char *)(block_ptr) + WSIZE))

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
static void *find_fit(size_t size);
static void place(void *ptr, size_t size);
static void heapdump();
static void mm_check();
static void set_next_free_block_ptr(void *block_ptr, void *next);
static void set_prev_free_block_ptr(void *block_ptr, void *prev);
static void add_to_free_list(void *block_ptr);
static void remove_from_free_list(void *block_ptr);

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
    set_next_free_block_ptr(free_list, 0);
    set_prev_free_block_ptr(free_list, 0);

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // Ignore spurious requests
    if (size == 0)
        return NULL;

    // Adjust block size to include overhead and alignment reqs.
    // Size will always be at least 1 due to the check above
    size_t block_size = calc_block_size(size);

    // Search for a fitting block
    void *block_ptr = find_fit(block_size);

    if (block_ptr == NULL)
    {
        // No fit found. Get more memory.
        size_t extend_size = max(block_size, CHUNKSIZE);
        block_ptr = extend_heap(extend_size / WSIZE);
        if (block_ptr == NULL)
            return NULL;
    }

    remove_from_free_list(block_ptr);
    place(block_ptr, block_size);

    return block_ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = get_size(get_header_ptr(ptr));

    set_val(get_header_ptr(ptr), make_header(size, 0));
    set_val(get_footer_ptr(ptr), make_header(size, 0));

    add_to_free_list(ptr);

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
 * Search for a block on the free list of at least the given size. Uses first-fit search.
 */
static void *find_fit(size_t size)
{
    void *block_ptr = free_list;
    while (block_ptr != NULL)
    {
        if (get_size(get_header_ptr(block_ptr)) >= size)
            return block_ptr;

        block_ptr = get_next_free_block_ptr(block_ptr);
    }

    return NULL;
}

/*
 * Places a block at the given block pointer. If the placed block leaves space
 * for another block after it, the existing block will be split in two.
 */
static void place(void *ptr, size_t size)
{
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

        // Add the next block to the free list
        add_to_free_list(next_block_ptr);
    }
    // There is not space for another block, so we don't split
    else
    {
        set_val(get_header_ptr(ptr), make_header(cur_size, 1));
        set_val(get_footer_ptr(ptr), make_header(cur_size, 1));
    }
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

    add_to_free_list(bp);

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * Coalesces adjacent free blocks into a single free block.
 *
 * If an encountered block is marked as free, that block MUST be on the free list.
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = is_allocated(get_footer_ptr(get_prev_block_ptr(bp)));
    size_t next_alloc = is_allocated(get_header_ptr(get_next_block_ptr(bp)));
    size_t size = get_size(get_header_ptr(bp));

    if (!next_alloc)
        remove_from_free_list(get_next_block_ptr(bp));
    if (!prev_alloc)
        remove_from_free_list(get_prev_block_ptr(bp));
    if (is_free(get_header_ptr(bp)))
        remove_from_free_list(bp);

    // When no surrounding blocks are free, we cannot coalesce
    if (prev_alloc && next_alloc)
    {
        // Do nothing
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

    add_to_free_list(bp);

    return bp;
}

/* Utilities */
static void set_next_free_block_ptr(void *block_ptr, void *next)
{
    if (block_ptr == NULL)
        return;

    set_val(block_ptr, (unsigned int)next);
}

static void set_prev_free_block_ptr(void *block_ptr, void *next)
{
    if (block_ptr == NULL)
        return;

    set_val((char *)(block_ptr) + WSIZE, (unsigned int)next);
}

static void add_to_free_list(void *block_ptr)
{
    set_next_free_block_ptr(block_ptr, free_list);
    set_prev_free_block_ptr(free_list, block_ptr);
    set_prev_free_block_ptr(block_ptr, NULL);
    free_list = block_ptr;
}

static void remove_from_free_list(void *block_ptr)
{
    void *prev = get_prev_free_block_ptr(block_ptr);
    void *next = get_next_free_block_ptr(block_ptr);

    if (prev != NULL)
    {
        set_next_free_block_ptr(prev, next);
        set_prev_free_block_ptr(next, prev);
    }
    else
    {
        set_prev_free_block_ptr(next, NULL);
        free_list = next;
    }
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
    printf("[%s] Addr: %p. Size: %d. prev->%p. next->%p.\n",
           is_allocated(get_header_ptr(ptr)) ? "Allocated" : "Free",
           ptr,
           get_size(get_header_ptr(ptr)),
           (void *)get_prev_free_block_ptr(ptr),
           (void *)get_next_free_block_ptr(ptr));
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
    int num_nil_prev = 0;
    int num_nil_next = 0;

    if (free_list != NULL && get_prev_free_block_ptr(free_list) != NULL)
    {
        printf("ILLEGAL: free list points to block with non-null previous element\n");
        exit(1);
    }

    void *block_ptr = free_list;
    while (block_ptr != NULL)
    {
        if (is_allocated(get_header_ptr(block_ptr)))
        {
            printf("ILLEGAL: allocated block detected on free list:\n   ");
            print_block(block_ptr);
            exit(1);
        }

        if (is_free(get_header_ptr(block_ptr)))
        {
            void *np = get_next_free_block_ptr(block_ptr);
            void *pp = get_prev_free_block_ptr(block_ptr);
            if (np != NULL && (np < mem_heap_lo() || np > mem_heap_hi()))
            {
                printf("ILLEGAL: found free list next reference pointing outside the heap\n");
                exit(1);
            }
            if (pp != NULL && (pp < mem_heap_lo() || pp > mem_heap_hi()))
            {
                printf("ILLEGAL: found free list prev reference pointing outside the heap\n");
                exit(1);
            }

            if (np == NULL)
                num_nil_next++;
            if (pp == NULL)
                num_nil_prev++;
        }

        block_ptr = (void *)get_val(block_ptr);
    }

    if (num_nil_prev > 1)
    {
        printf("ILLEGAL: found more than one item on the free list with a NULL prev reference\n");
        exit(1);
    }
    if (num_nil_next > 1)
    {
        printf("ILLEGAL: found more than one item on the free list with a NULL next reference\n");
        exit(1);
    }
}
