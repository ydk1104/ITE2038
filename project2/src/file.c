#include<db.h>
#include<file.h>

int table_id_to_fd[TABLE_SIZE];
page_t pages[100];
int start, end;

void node_to_page(node* node){
	pagenum_t pagenum = -1;
	for(int i=start; i!=end; i=(i+1)%100){
		if(pages[i].node == node){
			pagenum = i;
			break;
		}
	}
	if(pagenum == -1){
		pagenum = end++;
	}
	page_t *page = pages+pagenum;
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
