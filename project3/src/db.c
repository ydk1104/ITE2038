#include<bpt.h>
#include<db.h>
#include<file.h>

int init_db (int buf_num){
	return init_buffer(buf_num);
}
//in db, table_id is 0 base, but output is 1 base
//so return table_id+1
int open_table (char *pathname){
	static char* pathname_to_table_id[TABLE_SIZE] = {0, };
	static int open_table_cnt = 0;
	for(int i=0; i<open_table_cnt; i++){ // 0 base
		if(strcmp(pathname, pathname_to_table_id[i])) continue;
		return i + 1; // table_id >= 1
	}
	if(open_table_cnt >= TABLE_SIZE) return -1; // table_id <= TABLE_SIZE(10)

	int table_id;
	table_id = file_open(pathname);
	if(table_id >= TABLE_SIZE) return -1; // table_id <= TABLE_SIZE(10)
	open_table_cnt++;
	char *temp = malloc(sizeof(char) * (strlen(pathname) + 1));
	strcpy(temp, pathname);
	pathname_to_table_id[table_id] = temp;
	return table_id + 1; // table_id >= 1
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_insert (int table_id, int64_t key, char * value){
	--table_id;
//	page_t header;
//	file_read_page(0, &header);
	page_t* header = get_header_ptr(true);
	pagenum_t root = insert(table_id, header->header.rootPageNum, key, value);
	if(root==-1) return 1;
//	file_read_page(0, &header);
	if(root != header->header.rootPageNum){
		header->header.rootPageNum = root;
		header->is_dirty = true;
	}
	--header->pin_count;
	return 0;
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_find (int table_id, int64_t key, char * ret_val){
	--table_id;
	page_t* header = get_header_ptr(true);
	int idx = find(table_id, header->header.rootPageNum, key, ret_val);
	--header->pin_count;
	if(idx == -1) return 1;
	return 0;
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_delete (int table_id, int64_t key){
	--table_id;
	page_t* header = get_header_ptr(true);
	pagenum_t root = delete(table_id, header->header.rootPageNum, key);
	if(root==-1) return 1;
	if(root != header->header.rootPageNum){
		header->header.rootPageNum = root;
		header->is_dirty = true;
	}
	--header->pin_count;
	return 0;
}
int close_table(int table_id){
}
int shutdown_db(void){
	return shutdown_buffer();
}
