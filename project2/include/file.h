#ifndef __file__
#define __file__

// file.h
#include<fcntl.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
typedef uint64_t pagenum_t;
typedef struct page_t {
// in-memory page structure
	char byte[4096];
}page_t;
#define PAGE_SIZE 4096
#define TABLE_SIZE 1

int file_open(char*);
// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

#endif
