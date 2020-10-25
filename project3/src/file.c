#include<db.h>
#include<file.h>

int table_id_to_fd[TABLE_SIZE];
page_t* pages;
int buf_size;
int start, temp, size, headidx, tailidx;

void print_buffer(){
#ifdef PRINT
	int prev = tailidx;
	for(int i=headidx; i!=tailidx; i=pages[i].nextidx){
		if(pages[i].previdx != prev)
		printf("%d : %d %d\n", i, pages[i].previdx, pages[i].nextidx);
		prev=i;
	}
#endif
}

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
	return 0;
}

int shutdown_buffer(void){
	print_buffer();
	for(int i=headidx, next; i!=tailidx; i=next){
		if(i>=1000 || i<0){
			printf("at %d bomb\n", i);
		}
		next = pages[i].nextidx;
		remove_buffer_element(pages+i);
	}
	remove_buffer_element(pages+tailidx);
	free(pages);
	return 0;
}

void push_buffer_element(page_t* page, int table_id, pagenum_t pagenum, bool is_read){

	//push a node already exist
	if(page-pages == headidx){
		printf("push_error\n");
	}

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
	int nextidx = page->nextidx, previdx = page->previdx;
	if(page->is_dirty){
		file_write_page(page->pagenum, page);
	}
	memset(page, 0, sizeof(page_t));
	//one node
	if(size == 1){
		headidx = tailidx = -1;
		size--;
		return;
	}
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
	if(page->pin_count > 0){
		printf("Error\n");
		return page;
	}
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
	if( pages[headidx].table_id == table_id &&
		pages[headidx].pagenum == pagenum) return headidx;
	for(pagenum_t i=pages[headidx].nextidx; i!=headidx; i=pages[i].nextidx){
		if( pages[i].table_id == table_id && 
			pages[i].pagenum == pagenum){
			return i;
		}
	}

	if(size == buf_size){
		//search victim, tail to head
		if(pages[tailidx].pin_count == 0) return tailidx;
		for(pagenum_t i=pages[tailidx].previdx; i!=tailidx; i=pages[i].previdx){
			if(pages[i].pin_count == 0){
				remove_buffer_element(pages+i);
				//push head after pop, so size is consistent
				push_buffer_element(pages+i, table_id, pagenum, is_read);
				return i;
			}
		}
		printf("cannot search\n");
		exit(-1);
	}
	else{
		// increment size
		push_buffer_element(pages+size, table_id, pagenum, is_read);
		return size-1;
	}
}

void node_to_page(struct node** nodeptr, bool doFree){
	if(*nodeptr == NULL) return;
	struct node* node = *nodeptr;
	pagenum_t pageidx = get_pageidx_by_pagenum(node->table_id, node->pagenum, false);
	page_t *page = pages+pageidx;

	--page->pin_count;
	if(page->pin_count != 0)
		printf("node_to_page : %ld %d\n", node->pagenum, page->pin_count);

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
//	file_write_page(node->pagenum, page);
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
		if(node->buffer_ptr->pin_count < 0){
			printf("page_to_node, %ld %d\n", node->buffer_ptr->pagenum, node->buffer_ptr->pin_count);
		}
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
	if(page->pin_count != 1)
		printf("page_to_node : %ld %d\n", pagenum, page->pin_count);

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
	static int table_id = 0;
	fd = open(pathname, O_RDWR | O_SYNC);
	if(fd == -1){ // pathname does not exist
		fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
		if(fd == -1){
			puts("file create error");
			return -1;
		}
		table_id_to_fd[table_id] = fd;
		page_t *head = get_header_ptr(table_id, false);
		head->header.numOfPages = 1;
		--head->pin_count;
	}
	else table_id_to_fd[table_id] = fd;
	return table_id++;
}
// Allocate an on-disk page from the free page list
page_t* file_alloc_page(int table_id){
	const int fd = table_id_to_fd[table_id];
	page_t* head = get_header_ptr(table_id, true);
	page_t* page;
//	--head->pin_count; - header is already pinned.
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
//	--head->pin_count; - header is already pinned.

	clean.free.nextFreePage = head->header.freePageNum;
	clean.pagenum = pagenum;
	head->header.freePageNum = pagenum;
	head->is_dirty = true;
	page_t* temp = pages + get_pageidx_by_pagenum(table_id, pagenum, false);
	clean.pin_count = temp->pin_count;
	clean.nextidx = temp->nextidx;
	clean.previdx = temp->previdx;
	*temp = clean;
	temp->is_dirty = true;

//	file_write_page(0, head);
//	file_write_page(pagenum, &clean);
}
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
	const int table_id = dest->table_id, fd = table_id_to_fd[table_id];
	if(dest->pin_count != 1){
		printf("read %ld, pin %d\n", pagenum, dest->pin_count);
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest->byte, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
	const int table_id = src->table_id, fd = table_id_to_fd[table_id];
	if(src->pin_count != 0){
		printf("write %ld, pin %d\n", pagenum, src->pin_count);
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	write(fd, src->byte, PAGE_SIZE);
}
