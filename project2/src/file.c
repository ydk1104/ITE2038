#include<db.h>
#include<file.h>

int table_id_to_fd[TABLE_SIZE];

int file_open(char* pathname){
	int fd;
	static int table_id = 0;
	fd = open(pathname, O_RDWR | O_SYNC);
	if(fd == -1){ // pathname does not exist
		fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 666);
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
