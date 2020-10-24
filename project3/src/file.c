#include<db.h>
#include<file.h>

int table_id_to_fd[TABLE_SIZE];
page_t* pages;
int buf_size;
int start, temp, end;

int init_buffer(int buf_num){
	pages = calloc(buf_num, sizeof(page_t));
	if(pages == NULL) return 1;
	buf_size = buf_num;
	return 0;
}

void remove_buffer_element(page_t* page){
	if(page->is_dirty){
		file_write_page(page->pagenum, page);
		page->is_dirty = 0;
	}
	return;
}

int shutdown_buffer(void){
	for(int i=0; i!=end; i++){
		remove_buffer_element(pages+i);
	}
	free(pages);
	return 0;
}

page_t* get_header_ptr(void){
	pagenum_t pageidx = get_pageidx_by_pagenum(0, true);
	page_t *page = pages+pageidx;
	if(page->pin_count > 0) return page;
	++page->pin_count;
	return page;
}

pagenum_t get_pageidx_by_pagenum(pagenum_t pagenum, bool is_read){
	pagenum_t pageidx = -1;
	for(pagenum_t i=0; i!=end; i++){
		if(pages[i].pagenum == pagenum){
			return i;
		}
	}
	if(end == buf_size){
		for(pagenum_t i=0; i!=end; i++){
			if(pages[i].pin_count == 0){
				remove_buffer_element(pages+i);
				pages[i].pagenum = pagenum;
				if(is_read){
					pages[i].pin_count++;
					file_read_page(pagenum, pages+i);
					pages[i].pin_count--;
				}
				return i;
			}
		}
	}
	else{
		pages[end].pagenum = pagenum;
		if(is_read){
			pages[end].pin_count++;
			file_read_page(pagenum, pages+end);
			pages[end].pin_count--;
		}
		return end++;
	}
}

void node_to_page(struct node** nodeptr, bool doFree){
	if(*nodeptr == NULL) return;
	struct node* node = *nodeptr;
	pagenum_t pageidx = get_pageidx_by_pagenum(node->pagenum, false);
	page_t *page = pages+pageidx;

	--page->pin_count;
	if(page->pin_count != 0)
		printf("node_to_page : %ld %d\n", node->pagenum, page->pin_count);

	memset(page, 0, sizeof(page_t));
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

void page_to_node(pagenum_t pagenum, struct node ** nodeptr){
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
		free(node->keys);
		free(node->pages);
		free(*nodeptr);
		*nodeptr = NULL;
	}
	if(pagenum == 0){
		return;
	}
	*nodeptr = node = malloc(sizeof(struct node));

	pagenum_t pageidx = get_pageidx_by_pagenum(pagenum, true);	
	page_t* page = pages+pageidx;
//	if(node->pagenum == pagenum) return; // correct?
	++page->pin_count;
	if(page->pin_count != 1)
		printf("page_to_node : %ld %d\n", pagenum, page->pin_count);

	node->parent = page->page.parentPageNum;
	node->is_leaf = page->page.isLeaf;
	node->num_keys = page->page.numOfKeys;
	node->pagenum = pagenum;
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
		page_t src = {0, };
		src.header.numOfPages = 1;
		table_id_to_fd[table_id] = fd;
		//file_write_page(0, &src);
	}
	else table_id_to_fd[table_id] = fd;
	return table_id++;
}
// Allocate an on-disk page from the free page list
page_t* file_alloc_page(){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	page_t* head = get_header_ptr();
	page_t* page;
//	--head->pin_count; - header is already pinned.
	int freePageNum = head->header.freePageNum;
	if(freePageNum){
		page_t free_page;
		page = pages+get_pageidx_by_pagenum(freePageNum, true);
		head->header.freePageNum = page->free.nextFreePage;
	}
	else{
		page_t free_page = {0, };
		page = pages+get_pageidx_by_pagenum(freePageNum, false);
		*page = free_page;
	}
	++page->pin_count;
	return page;
}
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	page_t clean = {0, }, *head = get_header_ptr();
//	--head->pin_count; - header is already pinned.

	clean.free.nextFreePage = head->header.freePageNum;
	head->header.freePageNum = pagenum;
	head->is_dirty = true;
//	file_write_page(0, head);
	file_write_page(pagenum, &clean);
}
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	if(dest->pin_count != 1){
		printf("read %ld, pin %d\n", pagenum, dest->pin_count);
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest->byte, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	if(src->pin_count != 0){
		printf("write %ld, pin %d\n", pagenum, src->pin_count);
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	write(fd, src->byte, PAGE_SIZE);
}
