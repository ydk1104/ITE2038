#include<buffer.h>

//private

int bufferManager::push(int table_id, pagenum_t pagenum, bool is_read){
	int i = stk.pop();
	page_t* page = buffer + i;
	page->lock();
	page->table_id = table_id;
	page->pagenum = pagenum;
	//empty list
	if(size++ == 0){
		headidx = tailidx = i;
		page->nextidx = headidx;
		page->previdx = tailidx;
	}
	else{
		page->nextidx = headidx;
		page->previdx = tailidx;
		buffer[headidx].previdx = i;
		buffer[tailidx].nextidx = i;
		headidx = page-buffer;
		tailidx = page->previdx;
	}
	//LRU end, buffer unlock
	bufferManagerLatch.unlock();
	if(is_read){
		file_read_page(pagenum, page);
	}
	return i;
}
void bufferManager::remove(page_t* page){
	pop(page);
	if(page->is_dirty){
		file_write_page(page->pagenum, page);
	}
	memset(page, 0, sizeof(page_t));
	page->table_id = page->pagenum =
	page->nextidx = page->previdx = -1;
	page->unlock();
	return;
}
void bufferManager::pop(page_t* page){
	//caller shuold unlock
	page->lock();
	//one node
	if(size == 1){
		headidx = tailidx = -1;
		size--;
		stk.push(page-buffer);
		return;
	}
	int n = page->nextidx, p = page->previdx;
	buffer[p].nextidx = n;
	buffer[n].previdx = p;
	if(headidx == page-buffer) headidx = n;
	if(tailidx == page-buffer) tailidx = p;
	size--;
	stk.push(page-buffer);
	return;
}
pagenum_t bufferManager::get_pageidx_by_pagenum(int table_id, pagenum_t pagenum, bool is_read){
//	std::unique_lock<std::mutex> lock(bufferManagerLatch);
	bufferManagerLatch.lock();
	//unlock in push, all return has push
	if(size == 0){
		return push(table_id, pagenum, is_read);
	}
	int i = headidx;
	do{
		if( buffer[i].table_id == table_id &&
		    buffer[i].pagenum == pagenum){
			//LRU Policy
			//pop and push, move head
			pop(buffer+i);
			buffer[i].unlock();
			return push(table_id, pagenum, false);
		}
		i = buffer[i].nextidx;
	}while(i!=headidx);

	if(size == cap){
		int i = tailidx;
		// search linked list or wait tail
		// we select wait tail
		remove(buffer+i);
		return push(table_id, pagenum, is_read);
	}
	else{
		//find not-used index
		return push(table_id, pagenum, is_read);
	}
	bufferManagerLatch.unlock();
	return -1;
}

//public

bufferManager::bufferManager(int buf_num):cap(buf_num),size(0),headidx(-1),tailidx(-1),fm(new fileManager(this)),stk(buf_num){
	buffer = new page_t[buf_num];
	//TODO : bad_alloc exception
}

int bufferManager::close_buffer(int table_id){
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

int bufferManager::shutdown_buffer(void){
	if(size == 0) goto end;
	for(int i=headidx, next; i!=tailidx; i=next){
		next = buffer[i].nextidx;
		remove(buffer+i);
	}
	remove(buffer+tailidx);
end:
	delete[] buffer;
	delete fm;
	return 0;
}
page_t* bufferManager::get_header_ptr(int table_id, bool is_read){
	pagenum_t pageidx = get_pageidx_by_pagenum(table_id, 0, is_read);
	page_t *page = buffer+pageidx;
	return page;
}

void bufferManager::page_to_node(int table_id, pagenum_t pagenum, node** nptr){
	node* n = *nptr;
	if(n != NULL){
		n->buffer_ptr->unlock();
		delete n;
		*nptr = NULL;
	}
	if(pagenum == 0) return;
	pagenum_t pageidx = get_pageidx_by_pagenum(table_id, pagenum, true);
	page_t* page = buffer+pageidx;
	*nptr = new node(page);
	return;
}


void bufferManager::node_to_page(node** nptr, bool doFree){
	if(*nptr == NULL) return;
	node* n = *nptr;
//	pagenum_t pageidx = get_pageidx_by_pagenum(n->table_id, n->pagenum, false);
//	page_t *page = pages+pageidx;
	page_t *page = n->buffer_ptr;
	page->unlock();
	page->is_dirty = true;
	if(doFree){
		delete n;
		*nptr = NULL;
	}
	else{
			printf("error");
	}
}


int bufferManager::file_open(char* pathname){
	fm->file_open(pathname);
}

page_t* bufferManager::file_alloc_page(int table_id){
	return fm->file_alloc_page(table_id);
}

void bufferManager::file_free_page(int table_id, pagenum_t pagenum){
	return fm->file_free_page(table_id, pagenum);
}

void bufferManager::file_read_page(pagenum_t pagenum, page_t* dest){
	return fm->file_read_page(pagenum, dest);
}

void bufferManager::file_write_page(pagenum_t pagenum, const page_t* src){
	return fm->file_write_page(pagenum, src);
}

page_t* bufferManager::get_page(int table_id, pagenum_t pagenum, bool is_read){
	return buffer + get_pageidx_by_pagenum(table_id, pagenum, is_read);
}

bufferManager::~bufferManager(){shutdown_buffer();}
