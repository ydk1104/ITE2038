#include<db.h>
#include<file.h>

int table_id_to_fd[TABLE_SIZE];
page_t pages[100];
int start, end;

pagenum_t get_pageidx_by_node(node* node){
	pagenum_t pageidx = -1;
	for(pagenum_t i=start; i!=end; i=(i+1)%100){
		if(pages[i].node == node){
			return i;
		}
	}
	return end++;
}

pagenum_t get_pageidx_by_pagenum(pagenum_t pagenum){
	pagenum_t pageidx = -1;
	for(pagenum_t i=start; i!=end; i=(i+1)%100){
		if(pages[i].node && pages[i].node->pagenum == pagenum){
			return i;
		}
	}
	return end++;
}

void node_to_page(node* node){
	pagenum_t pageidx = get_pageidx_by_node(node);

	page_t *page = pages+pageidx;
	page->page.parentPageNum = (pagenum_t)node->parent;
	page->page.isLeaf = node->is_leaf;
	page->page.numOfKeys = node->num_keys;
	int leaf_order = 31, internal_order = 248;
	leaf_order = internal_order = 4;
	if(node -> is_leaf){
		for(int i=0; i<node->num_keys; i++){
			page->leaf[i].key = node->keys[i];
			strncpy(page->leaf[i].value, ((record*)node->pointers[i])->value, 120);
		}
	}
	else{
		for(int i=0; i<node->num_keys; i++){
			page->internal[i].key = node->keys[i];
			page->internal[i].pageNum = (pagenum_t)node->pointers[i];
		}
	}
	file_write_page(node->pagenum, page);
}

struct node* page_to_node(pagenum_t pagenum){
	if(pagenum == 0) return NULL;
		
	pagenum_t pageidx = get_pageidx_by_pagenum(pagenum);	
	int leaf_order = 31, internal_order = 248;
	leaf_order = internal_order = 4;
	page_t* page = pages+pageidx;
	if(page->node && page->node->pagenum == pagenum) return page->node;
	
	file_read_page(pagenum, page);
	struct node* node = malloc(sizeof(struct node));
	page->node = node;
	node->parent = (struct node*)(page->page.parentPageNum);
	node->is_leaf = page->page.isLeaf;
	node->num_keys = page->page.numOfKeys;
	node->next = NULL;
	node->pagenum = pagenum;
	if(node -> is_leaf){
		node->keys = malloc((leaf_order - 1) * sizeof(int64_t));
		node->pointers = malloc(leaf_order * sizeof(void*));
		for(int i=0; i<node->num_keys; i++){
			node->keys[i] = page->leaf[i].key;
			node->pointers[i] = malloc(sizeof(record));
			strncpy(((record*)node->pointers[i])->value, page->leaf[i].value, 120);
		}
	}
	else{
		node->keys = malloc((internal_order - 1) * sizeof(int64_t));
		node->pointers = malloc(internal_order * sizeof(void*));
		for(int i=0; i<node->num_keys; i++){
			node->keys[i] = page->internal[i].key;
			node->pointers[i] = (void*)page->internal[i].pageNum;
		}
	}
	return node;
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
		puts("New file");
		page_t src = {0, };
		src.header.numOfPages = 1;
		table_id_to_fd[table_id] = fd;
		file_write_page(0, &src);
	}
	else table_id_to_fd[table_id] = fd;
	return table_id++;
}
// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	page_t head;
	file_read_page(0, &head);
	int freePageNum = head.header.freePageNum;
	if(freePageNum){
		page_t free_page;
		file_read_page(freePageNum, &free_page);
		head.header.freePageNum = free_page.free.nextFreePage;
	}
	else{
		page_t free_page = {0, };
		file_write_page(freePageNum = head.header.numOfPages++, &free_page);
	}
	file_write_page(0, &head);
	return freePageNum;
}
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	page_t head, clean = {0, };

	file_read_page(0, &head);
	clean.free.nextFreePage = head.header.freePageNum;
	head.header.freePageNum = pagenum;
	file_write_page(0, &head);
	file_write_page(pagenum, &clean);
}
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest->byte, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	write(fd, src->byte, PAGE_SIZE);
}
