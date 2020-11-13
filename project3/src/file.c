#include<db.h>
#include<file.h>

int table_count;
int table_id_to_fd[TABLE_SIZE];
page_t* pages;
int buf_size;
int start, temp, size, headidx, tailidx;

int init_buffer(int buf_num){
	pages = calloc(buf_num, sizeof(page_t));
	if(pages == NULL) return 1;
	buf_size = buf_num;
	for(int i=0; i<buf_num; i++){
		pages[i].previdx = -1;
		pages[i].nextidx = -1;
		pages[i].pagenum = -1;
	}
	headidx = -1, tailidx = -1;
	
	table_count = 0;

	return 0;
}

int close_buffer(int table_id){
	if(size == 0) return 0;
	for(int i=headidx, next; i!=tailidx; i=next){
		next = pages[i].nextidx;
		if(pages[i].table_id == table_id){
			while(pages[i].pin_count);
			remove_buffer_element(pages+i);
		}
	}
	if(pages[tailidx].table_id == table_id){
		while(pages[tailidx].pin_count);
		remove_buffer_element(pages+tailidx);
	}
	return 0;
}

int shutdown_buffer(void){
	if(size == 0) return 0;
	for(int i=headidx, next; i!=tailidx; i=next){
		next = pages[i].nextidx;
		while(pages[i].pin_count);
		remove_buffer_element(pages+i);
	}
	while(pages[tailidx].pin_count);
	remove_buffer_element(pages+tailidx);
	free(pages);
	table_count = 0;
	return 0;
}

void push_buffer_element(page_t* page, int table_id, pagenum_t pagenum, bool is_read){

	page->table_id = table_id;
	page->pagenum = pagenum;
	//empty list
	if(size == 0){
		headidx = tailidx = page-pages;
		page->nextidx = headidx;
		page->previdx = tailidx;
	}
	else{
		page->nextidx = headidx;
		page->previdx = tailidx;
		pages[headidx].previdx = page-pages;
		pages[tailidx].nextidx = page-pages;
		headidx = page-pages;
		tailidx = page->previdx;
	}
	size++;
	if(is_read){
		page->pin_count++;
		file_read_page(pagenum, page);
		page->pin_count--;
	}
}

void remove_buffer_element(page_t* page){
	pop_buffer_element(page);
	if(page->is_dirty){
		file_write_page(page->pagenum, page);
	}
	memset(page, -1, sizeof(page_t));
	page->pin_count = 0;
	return;
}

void pop_buffer_element(page_t* page){
	//one node
	if(size == 1){
		headidx = tailidx = -1;
		size--;
		return;
	}
	int nextidx = page->nextidx, previdx = page->previdx;
	pages[previdx].nextidx = nextidx;
	pages[nextidx].previdx = previdx;
	if(headidx == page-pages) headidx = nextidx;
	if(tailidx == page-pages) tailidx = previdx;
	size--;
	return;
}

page_t* get_header_ptr(int table_id, bool is_read){
	pagenum_t pageidx = get_pageidx_by_pagenum(table_id, 0, is_read);
	page_t *page = pages+pageidx;
/*	if(page->pin_count > 1){
		printf("Error\n");
		return page;
	} */
	++page->pin_count;
	return page;
}

pagenum_t get_pageidx_by_pagenum(int table_id, pagenum_t pagenum, bool is_read){

	//empty list
	if(size == 0){
		push_buffer_element(pages+0, table_id, pagenum, is_read);
		return 0;
	}

	//search page, head to tail
	int i = headidx;
	do{
		if( pages[i].table_id == table_id &&
		    pages[i].pagenum == pagenum){
			//LRU Policy
			//pop and push, move head
			pop_buffer_element(pages+i);
			push_buffer_element(pages+i, table_id, pagenum, false);
			return i;
		}
		i = pages[i].nextidx;
	}while(i!=headidx);

	if(size == buf_size){
		//search victim, tail to head
		int i = tailidx;
		do{
			if(pages[i].pin_count == 0){
				remove_buffer_element(pages+i);
				//push head after pop, so size is consistent
				push_buffer_element(pages+i, table_id, pagenum, is_read);
				return i;
			}
			i = pages[i].previdx;
		}while(i != tailidx);
		printf("cannot search\n");
		exit(-1);
	}
	else{
		// find not-used index
		int i=0;
		while(pages[i].nextidx != -1) i++;
		push_buffer_element(pages+i, table_id, pagenum, is_read);
		return i;
	}
}

void node_to_page(struct node** nodeptr, bool doFree){
	if(*nodeptr == NULL) return;
	struct node* node = *nodeptr;
	pagenum_t pageidx = get_pageidx_by_pagenum(node->table_id, node->pagenum, false);
	page_t *page = pages+pageidx;

	--page->pin_count;
//	if(page->pin_count != 0)
//		printf("node_to_page : %ld %d\n", node->pagenum, page->pin_count);

	page->page.parentPageNum = (pagenum_t)node->parent;
	page->page.isLeaf = node->is_leaf;
	page->page.numOfKeys = node->num_keys;
	int leaf_order = DEFAULT_LEAF_ORDER, internal_order = DEFAULT_INTERNAL_ORDER;
	if(node -> is_leaf){
		page->page.pageNum = node->pages[leaf_order-1];
		for(int i=0; i<node->num_keys; i++){
			page->leaf[i].key = node->keys[i];
			strncpy(page->leaf[i].value, ((record*)node->pointers[i])->value, 120);
		}
	}
	else{
		page->page.pageNum = node->pages[0];
		for(int i=0; i<node->num_keys; i++){
			page->internal[i].key = node->keys[i];
			page->internal[i].pageNum = node->pages[i+1];
		}
	}
	page->is_dirty = true;
	if(doFree){
		free(node->keys);
		if(node->is_leaf){
			for(int i=0; i<node->num_keys; i++){
				free(node->pointers[i]);
			}	
		}
		free(node->pages);
		free(*nodeptr);
		*nodeptr = NULL;
	}
}

void page_to_node(int table_id, pagenum_t pagenum, struct node ** nodeptr){
	int leaf_order = DEFAULT_LEAF_ORDER,
		internal_order = DEFAULT_INTERNAL_ORDER;
	
	struct node * node = *nodeptr;
	if(node != NULL){
		if(node->is_leaf){
			for(int i=0; i<node->num_keys; i++){
				free(node->pointers[i]);
			}
		}
		--node->buffer_ptr->pin_count;
//		if(node->buffer_ptr->pin_count < 0){
//			printf("page_to_node, %ld %d\n", node->buffer_ptr->pagenum, node->buffer_ptr->pin_count);
//		}
		free(node->keys);
		free(node->pages);
		free(*nodeptr);
		*nodeptr = NULL;
	}
	if(pagenum == 0){
		return;
	}
	*nodeptr = node = malloc(sizeof(struct node));

	pagenum_t pageidx = get_pageidx_by_pagenum(table_id, pagenum, true);	
	page_t* page = pages+pageidx;
//	if(node->pagenum == pagenum) return; // correct?
	++page->pin_count;
//	if(page->pin_count != 1)
//		printf("page_to_node : %ld %d\n", pagenum, page->pin_count);

	node->parent = page->page.parentPageNum;
	node->is_leaf = page->page.isLeaf;
	node->num_keys = page->page.numOfKeys;
	node->pagenum = pagenum;
	node->table_id = table_id;
	node->buffer_ptr = page;
	if(node -> is_leaf){
		node->keys = malloc((leaf_order - 1) * sizeof(int64_t));
		node->pointers = malloc(leaf_order * sizeof(void*));
		node->pages[leaf_order-1] = page->page.pageNum;
		for(int i=0; i<node->num_keys; i++){
			node->keys[i] = page->leaf[i].key;
			node->pointers[i] = malloc(sizeof(record));
			strncpy(((record*)node->pointers[i])->value, page->leaf[i].value, 120);
		}
	}
	else{
		node->keys = malloc((internal_order - 1) * sizeof(int64_t));
		node->pages = malloc(internal_order * sizeof(pagenum_t*));
		node->pages[0] = page->page.pageNum;
		for(int i=0; i<node->num_keys; i++){
			node->keys[i] = page->internal[i].key;
			node->pages[i+1] = page->internal[i].pageNum;
		}
	}
	return;
}

int file_open(char* pathname){
	int fd;
	fd = open(pathname, O_RDWR | O_SYNC);
	if(fd == -1){ // pathname does not exist
		fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
		if(fd == -1){
			puts("file create error");
			return -1;
		}
		table_id_to_fd[table_count] = fd;
		page_t *head = get_header_ptr(table_count, false);
		head->header.numOfPages = 1;
		--head->pin_count;
	}
	else table_id_to_fd[table_count] = fd;
	return table_count++;
}
// Allocate an on-disk page from the free page list
page_t* file_alloc_page(int table_id){
	const int fd = table_id_to_fd[table_id];
	page_t* head = get_header_ptr(table_id, true);
	page_t* page;
	--head->pin_count;// - header is already pinned, but pin_count == 2
	int freePageNum = head->header.freePageNum;
	if(freePageNum){
		page_t free_page;
		page = pages+get_pageidx_by_pagenum(table_id, freePageNum, true);
		head->header.freePageNum = page->free.nextFreePage;
	}
	else{
		page = pages+get_pageidx_by_pagenum(table_id, freePageNum = head->header.numOfPages++, false);
	}
	++page->pin_count;
	return page;
}
// Free an on-disk page to the free page list
void file_free_page(int table_id, pagenum_t pagenum){
	const int fd = table_id_to_fd[table_id];
	page_t clean = {0, }, *head = get_header_ptr(table_id, true);
	--head->pin_count; //- header is already pinned., but pin_count == 2

	clean.free.nextFreePage = head->header.freePageNum;
	clean.pagenum = pagenum;
	clean.table_id = table_id;
	head->header.freePageNum = pagenum;
	head->is_dirty = true;
	page_t* temp = pages + get_pageidx_by_pagenum(table_id, pagenum, false);
	clean.pin_count = temp->pin_count;
	clean.nextidx = temp->nextidx;
	clean.previdx = temp->previdx;
	*temp = clean;
	temp->is_dirty = true;

}
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
	const int table_id = dest->table_id, fd = table_id_to_fd[table_id];
//	if(dest->pin_count != 1){
//		printf("read %ld, pin %d\n", pagenum, dest->pin_count);
//	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest->byte, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
	const int table_id = src->table_id, fd = table_id_to_fd[table_id];
//	if(src->pin_count != 0){
//		printf("write %ld, pin %d\n", pagenum, src->pin_count);
//	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	write(fd, src->byte, PAGE_SIZE);
}
