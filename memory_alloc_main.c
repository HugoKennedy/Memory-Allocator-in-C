#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "memory_alloc.h"

int main(int argc, char**argv) {

  struct memory_alloc m;
  int nb_blocks = 8;
  size_t block_size = 16;

  memalloc_init(&m, nb_blocks, block_size);
  memalloc_print(&m);

  /* use 1/3 of the blocks */
  size_t alloc_size = (nb_blocks * block_size) / 3;
  char* buffer = memalloc_allocate(&m, 64);
  for(int i=0; i<alloc_size; i++)
    buffer[i]='a';

  buffer[alloc_size-1] = '\0';
  printf("%s\n", buffer);
  memalloc_free(&m, buffer, alloc_size);
  
  return EXIT_SUCCESS;
}
