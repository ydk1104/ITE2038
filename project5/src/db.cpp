#include<bpt.h>
#include<db.h>
#include<file.h>

char* pathname_to_table_id[TABLE_SIZE];
int open_table_cnt;
static trxManager *tm;

int init_db (int buf_num){
	tm = new trxManager;
	return init_bpt(buf_num, tm);
}

int trx_begin(void){
	return tm->trx_begin();
}

int trx_commit(int trx_id){
	if(tm->find(trx_id) == false) return 0;
	return tm->trx_commit(trx_id);
}

//in db, table_id is 0 base, but output is 1 base
//so return table_id+1
int open_table (char *pathname){
	for(int i=0; i<open_table_cnt; i++){ // 0 base
		if(strcmp(pathname, pathname_to_table_id[i])) continue;
		return i + 1; // table_id >= 1
	}
	if(open_table_cnt >= TABLE_SIZE) return -1; // table_id <= TABLE_SIZE(10)

	int table_id;
	table_id = file_open(pathname);
	if(table_id >= TABLE_SIZE) return -1; // table_id <= TABLE_SIZE(10)
	open_table_cnt++;
	char *temp = new char[strlen(pathname) + 1];
	strcpy(temp, pathname);
	pathname_to_table_id[table_id] = temp;
	return table_id + 1; // table_id >= 1
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_insert (int table_id, int64_t key, char * value){
	--table_id;
	page_t* header = get_header_ptr(table_id, true);
	pagenum_t rootPageNum = header->data.header.rootPageNum;
	header->unlock();
	pagenum_t root = insert(table_id, rootPageNum, key, value);
	if(root==-1) return 1;
	header = get_header_ptr(table_id, true);
	if(root != header->data.header.rootPageNum){
		header->data.header.rootPageNum = root;
		header->is_dirty = true;
	}
	//TODO : make get_root_page_num
	header->unlock();
	return 0;
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_find (int table_id, int64_t key, char * ret_val, int trx_id){
	if(tm->find(trx_id) == false) return -1;
	--table_id;
	page_t* header = get_header_ptr(table_id, true);
	pagenum_t rootPageNum = header->data.header.rootPageNum;
	header->unlock();
	int idx = find(table_id, rootPageNum, key, ret_val, trx_id);
	if(idx != 0) return tm->trx_abort(trx_id);
	return 0;
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_update (int table_id, int64_t key, char * values, int trx_id){
	if(tm->find(trx_id) == false) return -1;
	--table_id;
	page_t* header = get_header_ptr(table_id, true);
	pagenum_t rootPageNum = header->data.header.rootPageNum;
	header->unlock();
	int idx = update(table_id, rootPageNum, key, values, trx_id, false);
	if(idx != 0) return tm->trx_abort(trx_id);
	return 0;
}
int db_undo_update (int table_id, int64_t key, char * old_values, int trx_id){
	page_t* header = get_header_ptr(table_id, true);
	pagenum_t rootPageNum = header->data.header.rootPageNum;
	header->unlock();
	update(table_id, rootPageNum, key, old_values, trx_id, true);
	return 0;
}
//in db, table_id is 0 base, but input is 1 base
//so we use table_id-1
int db_delete (int table_id, int64_t key){
	--table_id;
	page_t* header = get_header_ptr(table_id, true);
	pagenum_t rootPageNum = header->data.header.rootPageNum;
	header->unlock();
	pagenum_t root = delete_main(table_id, rootPageNum, key);
	if(root==-1) return 1;
	header = get_header_ptr(table_id, true);
	if(root != header->data.header.rootPageNum){
		header->data.header.rootPageNum = root;
		header->is_dirty = true;
	}
	header->unlock();
	return 0;
}
int close_table(int table_id){
	return close_buffer(table_id-1);
}

int shutdown_db(void){
	for(int i=0; i<open_table_cnt; i++){
		delete[] pathname_to_table_id[i];
		pathname_to_table_id[i] = NULL;
	}
	open_table_cnt = 0;
	delete tm;
	return shutdown_buffer();
}
