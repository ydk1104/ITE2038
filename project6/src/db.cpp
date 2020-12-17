#include<stdarg.h>
#include<bpt.h>
#include<db.h>
#include<file.h>

char* pathname_to_table_id[TABLE_SIZE];
int open_table_cnt;
static bufferManager *bm;
static logManager *lm;
static trxManager *tm;

char* table_id_to_file_name[15] = {
"DATA00.db",
"DATA01.db",
"DATA02.db",
"DATA03.db",
"DATA04.db",
"DATA05.db",
"DATA06.db",
"DATA07.db",
"DATA08.db",
"DATA09.db",
"DATA10.db",

};

#define print(s, ...) printf(s, ##__VA_ARGS__)

int init_db (int buf_num, int flag, int log_num, char* log_path, char* logmsg_path){
	bm = new bufferManager(buf_num);
	lm = new logManager;
	tm = new trxManager(lm);
	
//TODO : move it to lm->recovety(); or make recovery_manager

// phase 1 : analysis
	print("[ANALYSIS] Analysis pass start\n");
	#include<set>
	#include<vector>
	std::set<int> trx_loser, trx_winner;
	std::vector<log_t*> logs;
	int table_ids[15] = {0, };
	lm->open_log(log_path);
	lm->analysis(trx_loser, trx_winner, logs, table_ids);
	for(int i=0; i<15; i++){
		if(table_ids[i]) bm->file_open(table_id_to_file_name[i+1]);
	}
	
	print("[ANALYSIS] Analysis success. Winner:");
	for(auto i : trx_winner) print(" %d", i);
	print(", Loser:");
	for(auto i : trx_loser) print(" %d", i);
	print(".\n");

// phase 2 : redo history
	print("[REDO] Redo pass start\n");
	for(auto i : logs){
		auto lsn = i->get_lsn();
		auto trx_id = i->get_trx_id();
		printf("type : %d\n", i->get_type());
		if(i->redo(bm)){
			print("LSN %lu [CONSIDER-REDO] Transaction id %d\n", lsn, trx_id);
			continue;
		}
		switch(i->get_type()){
			case BEGIN:
				print("LSN %lu [BEGIN] Transaction id %d\n", lsn, trx_id);
				break;
			case UPDATE:
				print("LSN %lu [UPDATE] Transaction id %d redo apply\n", lsn, trx_id);
				break;
			case COMMIT:
				print("LSN %lu [COMMIT] Transaction id %d\n", lsn, trx_id);
				break;
			case ROLLBACK:
				print("LSN %lu [ROLLBACK] Transaction id %d\n", lsn, trx_id);
				break;
			case COMPENSATE_UPDATE:
				print("LSN %lu [CLR] next undo lsn %lu\n", lsn, i->get_next_undo_lsn());
				break;

		}
	}
	print("[REDO] Redo pass end\n");
// phase 3 : undo
	print("[UNDO] Undo pass start\n");

//truncate
	lm->truncate(log_path, 0);
// flush
	for(int i=0; i<15; i++){
		if(table_ids[i]) bm->close_buffer(i);
	}
	return init_bpt(bm, tm);
}

int trx_begin(void){
	return tm->trx_begin();
}

int trx_commit(int trx_id){
	//TODO : GET LOCK or check in tm->trx_commit
	if(tm->find(trx_id) == false) return 0;
	return tm->trx_commit(trx_id);
}

int trx_abort(int trx_id){
	//TODO : GET LOCK or check in tm->trx_abort
	if(tm->find(trx_id) == false) return 0;
	return tm->trx_abort(trx_id, bm);
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
	if(idx != 0) return tm->trx_abort(trx_id, bm);
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
	int idx = update(table_id, rootPageNum, key, values, trx_id);
	if(idx != 0) return tm->trx_abort(trx_id, bm);
	return 0;
}
int db_undo_update (int table_id, int64_t key, char * old_values, int trx_id){
	//page_t* header = get_header_ptr(table_id, true);
	//pagenum_t rootPageNum = header->data.header.rootPageNum;
	//header->unlock();
	//update(table_id, rootPageNum, key, old_values, trx_id, true);
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
