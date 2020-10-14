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
		file_write_page(0, &src);
	}
	
	table_id_to_fd[table_id] = fd;
	return table_id++;
}
// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
	const int table_id = 0, fd = table_id_to_fd[table_id];
	
	return -1;
}
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
	const int table_id = 0, fd = table_id_to_fd[table_id];
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
