#ifndef MEMORY_ALLOC_H
#define MEMORY_ALLOC_H

#include <stdlib.h>
#include <stdint.h>

/* result of the last memroy operation */
enum memory_errno {
  E_SUCCESS,			/* success */
  E_NOMEM,			/* error: not enough memory */
  E_SHOULD_PACK,		/* error: not enough consecutive blocks */
};

struct memory_block {
  struct memory_block* next;
};
/* struct memory_block a;
struct memory_block b;
struct memory_block c;

a.next = &b;
b.next = &c;
c.next = NULL;
 */

struct memory_alloc {
  /* total number of pre-allocated blocks  */
  size_t nb_prealloc_blocks;

  /* size (bytes) of one block */
  size_t block_size;

  /* blocks that can be allocated
   * This is an array of size block_size * allocated_blocks
   */
  void* prealloc_blocks; //pointeur générique vers le début du bloc de mémoire préalloué

  /* number of blocks that are available */
  size_t available_blocks;

  /* first available block */
  struct memory_block* first_block; //premier bloc libre

  /* error of the last memory operation. to be updated during each
   * call to memory_free/memory_alloc/memory_init
   */
  enum memory_errno errno;
};


/* Initialize the memory_alloc structure */
void memalloc_init(struct memory_alloc* m, int nb_blocks, size_t block_size);

/* Finalize the memory_alloc structure, and free all the memory that was malloc'd */
void memalloc_finalize(struct memory_alloc* m);

/* return the number of consecutive blocks starting from first */
int memalloc_nb_consecutive_blocks(struct memory_alloc* m, struct memory_block* first);

/* Print the current status of the memory_alloc structure */
void memalloc_print(struct memory_alloc* m);

/* Allocate size consecutive bytes and return the index of the first
 * memory block
 */
void* memalloc_allocate(struct memory_alloc* m, size_t size);

/* Free the size bytes memory space starting at address addr */
void memalloc_free(struct memory_alloc* m, void* address, size_t size);

/* Print a message corresponding to errno */
void memalloc_error_print(enum memory_errno error_number);

/* Allocate size consecutive bytes and return the index of the first
 * memory block available to be written. Note: Return NULL if
 * not enough available memory blocks.
 */
void* memalloc_lifelike_malloc(struct memory_alloc *m, size_t size);

/* Free the memory blocks designated by addr, which value must have 
 * been previously returned by memalloc_lifelike_malloc().
*/
void memalloc_lifelike_free(struct memory_alloc *m, void* addr);

/* Change the size of the memory block designated by addr to size bytes.
 * The contents will be unchanged in the range from the start of the 
 * region up to the minimum of the old and new sizes. If the new size 
 * is larger than the old size, the added memory will not be initialized.
 * If addr is NULL, then the call is equivalent to
 * memalloc_lifelike_malloc(m, size), for all values of size; if size is equal
 * to zero, and addr is not NULL, then the call is equivalent to 
 * memalloc_lifelike_free(m, ptr). Unless addr is NULL, it must have 
 * been returned by an earlier call to memalloc_lifelike_malloc(). 
 * If the area pointed to was moved, a memalloc_lifelike_free(addr) is done.
 * Note: Return NULL if not enough available memory blocks.
*/
void* memalloc_lifelike_realloc(struct memory_alloc *m, void* addr, size_t size);

// Exercice 4 : Canari

void* memalloc_lifelike_malloc_canary(struct memory_alloc *m, size_t size);

void memalloc_lifelike_free_canary(struct memory_alloc *m, void* addr);

void* memalloc_lifelike_realloc_canary(struct memory_alloc *m, void* addr, size_t size);



#endif	/* MEMORY_ALLOC_H */
