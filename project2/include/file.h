#ifndef __file__
#define __file__

// file.h
#include<bpt.h>
#include<fcntl.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdbool.h>
#include<unistd.h>
typedef uint64_t pagenum_t;
typedef struct{
// in-memory page structure
	union{
		char byte[4096];
		struct{
			union{
				struct{
					uint64_t freePageNum;
					uint64_t rootPageNum;
					uint64_t numOfPages;
				}header;
				struct{
					uint64_t nextFreePage;
				}free;
				struct{
					struct{
						uint64_t parentPageNum;
						uint32_t isLeaf;
						uint32_t numOfKeys;
						char reserved[104];
						uint64_t pageNum;
					}page;
					union{
						struct{
							uint64_t key;
							uint64_t pageNum;
						}internal[248];
						struct{
							uint64_t key;
							char value[120];
						}leaf[31];
					};
				};
			};
		pagenum_t pagenum;
		};
	};
}page_t;

#define PAGE_SIZE 4096
#define TABLE_SIZE 1

typedef struct node node;

pagenum_t get_pageidx_by_pagenum(pagenum_t pagenum);
void node_to_page(node** nodeptr, bool do_free);
void page_to_node(pagenum_t pagenum, node** nodeptr);
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
