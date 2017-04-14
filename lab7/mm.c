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

/* use explicit free list and first fit strategy */

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
    "515030910036",
    /* First member's full name */
    "Lv Jiawei",
    /* First member's email address */
    "ljw9609@sjtu.edu.cn",
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

/*Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* prev and next free block in list */
#define GET_PREV(bp) (*(unsigned int *)(bp))
#define GET_NEXT(bp) (*((unsigned int *)(bp) + 1))
#define SET_PREV(bp, val) (*(unsigned int *)(bp) = (val))
#define SET_NEXT(bp, val) (*((unsigned int *)(bp) + 1) = (val))

static char *heap_listp;
static void *free_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);

/* free list funcs */
static void init_free(void);
static void delete_from_free(void *bp);
static void add_to_free(void *bp);



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void){
    init_free();    //init free list

    /* Create the initial empty heap */
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); //alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue hdr
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue ftr
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //eplogue hdr
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUCKSIZE bytes */
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;

    
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size){
    size_t asize;       //adjusted block size
    size_t extend_size;  //extend heap size
    char *bp;

    if(size == 0) return NULL;

    /* adjust block size  */
    if(size <= DSIZE) 
        asize = 2 * DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* find fit in free list */
    if((bp = find_fit(asize)) != NULL) {
        bp = place(bp, asize);
        return bp;
    }

    /* if not find fit,then extend heap */
    extend_size = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extend_size / WSIZE)) == NULL) 
        return NULL;
    bp = place(bp, asize);
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size){
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t asize;
    void *newptr;

    if(ptr == NULL) 
        return mm_malloc(size);
    else if(size == 0){
        mm_free(ptr);
        return NULL;
    }

    if(size <= DSIZE) 
        asize = 2 * DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    if(asize <= old_size)
        return ptr;
    else{
    	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

	    if(!next_alloc && (old_size + next_size) >= asize){
            delete_from_free(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(old_size + next_size, 1));
            PUT(FTRP(ptr), PACK(old_size + next_size, 1));
            return ptr;
        }
        else{
            newptr = mm_malloc(size);
            memcpy(newptr, ptr, old_size);
            mm_free(ptr);
            return newptr;
        }
    }

}

/* extend heap if there is not enough space */
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    /* Initialize free block hdr/ftr and the epilogue hdr */
    PUT(HDRP(bp), PACK(size, 0)); //Free block hdr
    PUT(FTRP(bp), PACK(size, 0)); //Free block ftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //New epilogue hdr

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}

/* coalesce free blocks */
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc)        //neither prev nor next is free
        add_to_free(bp);
    else if(prev_alloc && !next_alloc){ //next is free
        delete_from_free(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        add_to_free(bp);
    }
    else if(!prev_alloc && next_alloc){ //prev is free
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else{                               //both prev and next are free
        delete_from_free(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/* use first fit strategy */
static void *find_fit(size_t asize){
    void *bp = free_listp;
    while(bp){
        if(GET_SIZE(HDRP(bp)) >= asize) 
            return bp;
        bp = GET_NEXT(bp);
    }
    return NULL; 
}

/* dynamic allocate space */
static void *place(void *bp, size_t asize){
    size_t origin_size = GET_SIZE(HDRP(bp));

    if((origin_size - asize) >= (2 * DSIZE)){ //split and allocate
        PUT(HDRP(bp), PACK((origin_size - asize), 0));
        PUT(FTRP(bp), PACK((origin_size - asize), 0));
        
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        return bp;
    }
    else{ 
        delete_from_free(bp);
        PUT(HDRP(bp), PACK(origin_size, 1));
        PUT(FTRP(bp), PACK(origin_size, 1));
        return bp;
    }
}

/* initialize free list */
static void init_free(void){
    free_listp = 0;
}

/* delete block from free list*/
static void delete_from_free(void *bp){
    unsigned int *prev = GET_PREV(bp);
    unsigned int *next = GET_NEXT(bp);
    if(prev) 
        SET_NEXT(prev, next);
    else 
        free_listp = next;
    if(next) 
        SET_PREV(next, prev);
}

/* add free block to free list */
static void add_to_free(void *bp){
    if(free_listp) 
        SET_PREV(free_listp, bp);
    SET_PREV(bp, 0);
    SET_NEXT(bp, free_listp);
    free_listp = bp;
}
