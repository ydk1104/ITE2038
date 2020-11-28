#ifndef __BUFFER__
#define __BUFFER__

#include<bpt.h>
#include<file.h>

class bufferManager{
private:
	fileManager* fm;
	page_t* buffer;
	int cap, size, headidx, tailidx;
	void push(page_t* page, int table_id, pagenum_t pagenum, bool is_read);
	void remove(page_t* page);
	void pop(page_t* page);
	pagenum_t get_pageidx_by_pagenum(int table_id, pagenum_t pagenum, bool is_read);
public:
	bufferManager(int buf_num);
	//(int buf_num):cap(buf_num),size(0),headidx(-1),tailidx(-1),fm(new fileManager(this)){
	//	buffer = new page_t[buf_num];
	//	//TODO : bad_alloc exception
//}
	int close_buffer(int table_id);
	int shutdown_buffer(void);
	page_t* get_header_ptr(int table_id, bool is_read);

	void node_to_page(node** nptr, bool doFree);
	void page_to_node(int table_id, pagenum_t pagenum, node** nptr);

	int file_open(char* pathname);
	page_t* file_alloc_page(int table_id);
	void file_free_page(int table_id, pagenum_t pagenum);
	void file_read_page(pagenum_t pagenum, page_t* dest);
	void file_write_page(pagenum_t pagenum, const page_t* src);
	pagenum_t get_page(int table_id, pagenum_t pagenum, bool is_read);
	~bufferManager();
};

#endif
