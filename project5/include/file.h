#ifndef __file__
#define __file__

// file.h
#include<type.h>
#include<buffer.h>
#include<fcntl.h>
#include<mutex>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdbool.h>
#include<unistd.h>

struct page_t{
// in-memory page structure
	union{
		char byte[4096];
		struct{
			union{
				struct{
					pagenum_t freePageNum;
					pagenum_t rootPageNum;
					pagenum_t numOfPages;
				}header;
				struct{
					pagenum_t nextFreePage;
				}free;
				struct{
					struct{
						pagenum_t parentPageNum;
						uint32_t isLeaf;
						uint32_t numOfKeys;
						char reserved[104];
						pagenum_t pageNum;
					}page;
					union{
						struct{
							int64_t key;
							pagenum_t pageNum;
						}internal[248];
						struct{
							int64_t key;
							record value;
//							char value[120];
						}leaf[31];
					};
				}pageData;
			};
		}data;
	};
	int table_id;
	pagenum_t pagenum;
	int is_dirty;
	std::mutex page_latch;
	int nextidx, previdx;
	void lock(){page_latch.lock();}
	void unlock(){page_latch.unlock();}
	page_t(){
		memset(this, 0, sizeof(page_t));
		previdx = nextidx = pagenum = -1;
	}
};

#define PAGE_SIZE 4096
#define TABLE_SIZE 10

class fileManager{
private:
	int table_count;
	int table_id_to_fd[TABLE_SIZE];
	bufferManager* bm;
public:
	fileManager(bufferManager* bm):table_count(0),bm(bm){}
	int file_open(char*);
	// Allocate an on-disk page from the free page list
	page_t* file_alloc_page(int table_id);
	// Free an on-disk page to the free page list
	void file_free_page(int table_id, pagenum_t pagenum);
	// Read an on-disk page into the in-memory page structure(dest)
	void file_read_page(pagenum_t pagenum, page_t* dest);
	// Write an in-memory page(src) to the on-disk page
	void file_write_page(pagenum_t pagenum, const page_t* src);
	page_t* get_header_ptr(int table_id, bool is_read);
	page_t* get_page_by_pagenum(int table_id, pagenum_t pagenum, bool is_read);
};

#endif
