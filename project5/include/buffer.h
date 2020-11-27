#ifndef __BUFFER__
#define __BUFFER__

#include<bpt.h>
#include<file.h>

class bufferManager{
private:
	fileManager* fm;
	page_t* buffer;
	int cap, size, headidx, tailidx;
	void push(page_t* page, int table_id, pagenum_t pagenum, bool is_read){
		page->table_id = table_id;
		page->pagenum = pagenum;
		//empty list
		if(size++ == 0){
			headidx = tailidx = page-buffer;
			page->nextidx = headidx;
			page->previdx = tailidx;
		}
		else{
			page->nextidx = headidx;
			page->previdx = tailidx;
			buffer[headidx].previdx = page-buffer;
			buffer[tailidx].nextidx = page-buffer;
			headidx = page-buffer;
			tailidx = page->previdx;
		}
		if(is_read){
			page->pin_count++;
			file_read_page(pagenum, page);
			page->pin_count--;
		}
		return;
	}
	void remove(page_t* page){
		while(page->is_active());
		pop(page);
		if(page->is_dirty){
			file_write_page(page->pagenum, page);
		}
		memset(page, 0, sizeof(page_t));
		page->table_id = page->pagenum =
		page->nextidx = page->previdx = -1;
		return;
	}
	void pop(page_t* page){
		//one node
		if(size == 1){
			headidx = tailidx = -1;
			size--;
			return;
		}
		int n = page->nextidx, p = page->previdx;
		buffer[p].nextidx = n;
		buffer[n].previdx = p;
		if(headidx == page-buffer) headidx = n;
		if(tailidx == page-buffer) tailidx = p;
		size--;
		return;
	}
	pagenum_t get_pageidx_by_pagenum(int table_id, pagenum_t pagenum, bool is_read){
		if(size == 0){
			push(buffer+0, table_id, pagenum, is_read);
			return 0;
		}
		int i = headidx;
		do{
			if( buffer[i].table_id == table_id &&
			    buffer[i].pagenum == pagenum){
				//LRU Policy
				//pop and push, move head
				pop(buffer+i);
				push(buffer+i, table_id, pagenum, false);
				return i;
			}
			i = buffer[i].nextidx;
		}while(i!=headidx);

		if(size == cap){
			int i = tailidx;
			// search linked list or wait tail
			// we select wait tail
			remove(buffer+i);
			push(buffer+i, table_id, pagenum, is_read);
			return i;
		}
		else{
			//find not-used index
			int i=0;
			while(buffer[i].nextidx != -1) i++;
			push(buffer+i, table_id, pagenum, is_read);
			return i;
		}
		return -1;
	}
public:
	bufferManager(int buf_num):cap(buf_num),size(0),headidx(-1),tailidx(-1),fm(new fileManager(this)){
		buffer = new page_t[buf_num];
		//TODO : bad_alloc exception
	}
//	int init_buffer(int buf_num);

	int close_buffer(int table_id){
		if(size == 0) return 0;
		for(int i=headidx, next; i!=tailidx; i=next){
			next = buffer[i].nextidx;
			if(buffer[i].table_id == table_id){
				remove(buffer+i);
			}
		}
		if(buffer[tailidx].table_id == table_id){
			remove(buffer+tailidx);
		}
		return 0;
	}
	
	int shutdown_buffer(void){
		if(size == 0) goto end;
		for(int i=headidx, next; i!=tailidx; i=next){
			next = buffer[i].nextidx;
			remove(buffer+i);
		}
		remove(buffer+tailidx);
end:
		delete[] buffer;
		return 0;
	}
	page_t* get_header_ptr(int table_id, bool is_read){
		pagenum_t pageidx = get_pageidx_by_pagenum(table_id, 0, is_read);
		page_t *page = buffer+pageidx;
		++page->pin_count;
		return page;
	}

	void node_to_page(node** nptr, bool doFree);
	void page_to_node(int table_id, pagenum_t pagenum, node** nptr);

	int file_open(char* pathname){
		fm->file_open(pathname);
	}

	page_t* file_alloc_page(int table_id){
		return fm->file_alloc_page(table_id);
	}

	void file_free_page(int table_id, pagenum_t pagenum){
		return fm->file_free_page(table_id, pagenum);
	}

	void file_read_page(pagenum_t pagenum, page_t* dest){
		return fm->file_read_page(pagenum, dest);
	}

	void file_write_page(pagenum_t pagenum, const page_t* src){
		return fm->file_write_page(pagenum, src);
	}

	pagenum_t get_page(int table_id, pagenum_t pagenum, bool is_read){
		return get_pageidx_by_pagenum(table_id, pagenum, is_read);
	}

	~bufferManager(){delete[] buffer;}
};

#endif
