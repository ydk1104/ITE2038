#include<buffer.h>

void bufferManager::node_to_page(node** nptr, bool doFree){
	if(*nptr == NULL) return;
	node* n = *nptr;
//	pagenum_t pageidx = get_pageidx_by_pagenum(n->table_id, n->pagenum, false);
//	page_t *page = pages+pageidx;
	page_t *page = n->buffer_ptr;
	--page->pin_count;
	page->is_dirty = true;
	if(doFree){
		delete n;
		*nptr = NULL;
	}
}

void bufferManager::page_to_node(int table_id, pagenum_t pagenum, node** nptr){
	node* n = *nptr;
	if(n != NULL){
		--n->buffer_ptr->pin_count;
		delete n;
		*nptr = NULL;
	}
	if(pagenum == 0) return;
	pagenum_t pageidx = get_pageidx_by_pagenum(table_id, pagenum, true);
	page_t* page = buffer+pageidx;
	*nptr = new node(page);
	++page->pin_count;
	return;
}
