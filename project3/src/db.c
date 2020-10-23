#include<bpt.h>
#include<db.h>
#include<file.h>

int open_table (char *pathname){
	int table_id;
	table_id = file_open(pathname);
	return table_id;
}
int db_insert (int64_t key, char * value){
	page_t header;
	file_read_page(0, &header);
	pagenum_t root = insert(header.header.rootPageNum, key, value);
	if(root==-1) return 1;
	file_read_page(0, &header);
	if(root != header.header.rootPageNum){
		header.header.rootPageNum = root;
		file_write_page(0, &header);
	}
	return 0;
}
int db_find (int64_t key, char * ret_val){
	page_t header;
	file_read_page(0, &header);
	int idx = find(header.header.rootPageNum, key, ret_val);
	if(idx == -1) return 1;
	return 0;
}
int db_delete (int64_t key){
	page_t header;
	file_read_page(0, &header);
	pagenum_t root = delete(header.header.rootPageNum, key);
	if(root==-1) return 1;
	file_read_page(0, &header);
	if(root != header.header.rootPageNum){
		header.header.rootPageNum = root;
		file_write_page(0, &header);
	}
	return 0;
}
