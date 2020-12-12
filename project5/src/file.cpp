#include<bpt.h>
#include<db.h>
#include<file.h>

int table_count;
int table_id_to_fd[TABLE_SIZE];
int buf_size;
int start, temp, size, headidx, tailidx;

int fileManager::file_open(char* pathname){
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
		head->data.header.numOfPages = 1;
		head->unlock();
	}
	else table_id_to_fd[table_count] = fd;
	return table_count++;
}
// Allocate an on-disk page from the free page list
page_t* fileManager::file_alloc_page(int table_id){
	const int fd = table_id_to_fd[table_id];
	page_t* head = get_header_ptr(table_id, true);
	page_t* page;
	int freePageNum = head->data.header.freePageNum;
	if(freePageNum){
		page_t free_page;
		page = get_page_by_pagenum(table_id, freePageNum, true);
		head->data.header.freePageNum = page->data.free.nextFreePage;
	}
	else{
		page = get_page_by_pagenum(table_id, freePageNum = head->data.header.numOfPages++, false);
	}
	head->is_dirty = true;
	head->unlock();
	return page;
}
// Free an on-disk page to the free page list
void fileManager::file_free_page(int table_id, pagenum_t pagenum){
	const int fd = table_id_to_fd[table_id];
	page_t *head = get_header_ptr(table_id, true);
	page_t* clean = get_page_by_pagenum(table_id, pagenum, false);
	memset(clean->byte, 0, sizeof(clean->byte));
	clean->data.free.nextFreePage = head->data.header.freePageNum;
	clean->pagenum = pagenum;
	clean->table_id = table_id;
	head->data.header.freePageNum = pagenum;
	head->is_dirty = true;
	clean->is_dirty = true;
	head->unlock();
}
// Read an on-disk page into the in-memory page structure(dest)
void fileManager::file_read_page(pagenum_t pagenum, page_t* dest){
	const int table_id = dest->table_id, fd = table_id_to_fd[table_id];
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest->byte, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void fileManager::file_write_page(pagenum_t pagenum, const page_t* src){
	const int table_id = src->table_id, fd = table_id_to_fd[table_id];
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	write(fd, src->byte, PAGE_SIZE);
}

page_t* fileManager::get_header_ptr(int table_id, bool is_read){
	return bm->get_header_ptr(table_id, is_read);
}

page_t* fileManager::get_page_by_pagenum(int table_id, pagenum_t pagenum, bool is_read){
	return bm->get_page(table_id, pagenum, is_read);
}
