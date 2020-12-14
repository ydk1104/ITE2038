#include "buffer.h"
#include "log.h"

info_t::info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type):
	log_size(log_size),lsn(prev_lsn),trx_id(trx_id),type(type){printf("log %d\n", type);}

//write to / read from buffer
void info_t::write(char* data_ptr){
	memcpy(data_ptr, this, log_size);
}

void info_t::read(char* data_ptr){
	int log_size = *(int*)data_ptr;
	memcpy(this, data_ptr, log_size);
}
int64_t info_t::get_lsn(){
	return lsn;
}
int64_t info_t::get_prev_lsn(){
	return prev_lsn;
}
int32_t info_t::get_trx_id(){
	return trx_id;
}
int32_t info_t::get_type(){
	return type;
}

// override
void info_t::redo(bufferManager* bm){}
void info_t::undo(bufferManager* bm){}
info_t::~info_t(){}

operator_info_t::operator_info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image):
	info_t(log_size, lsn, prev_lsn, trx_id, type),
	table_id(table_id), pageNum(pageNum), offset(offset), data_length(data_length){
		memcpy(&this->old_image, old_image, data_length);
		memcpy(&this->new_image, new_image, data_length);
	}
operator_info_t::~operator_info_t(){}

//physical redo & undo
//interact with buffer manger
void operator_info_t::redo(bufferManager* bm){
//	get_page(table_id, pageNum);
//	memcpy(page->data[offset], new_image, data_length);
}

void operator_info_t::undo(bufferManager* bm){
	node* n = NULL;
	//get page lock
	//it is useless in out project spec - we already get record_lock, record ptr doesn't move
	bm->page_to_node(table_id, pageNum, &n);
	char* ptr = (char*)n->buffer_ptr;
	ptr += offset;
	memcpy(ptr, old_image, data_length);
	bm->node_to_page(&n, true);
}

begin_info_t::begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, BEGIN){}

update_info_t::update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image){}

commit_info_t::commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMMIT){}

rollback_info_t::rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, ROLLBACK){}

compensate_update_info_t::compensate_update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image, int64_t next_undo_lsn):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMPENSATE_UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image),
	next_undo_lsn(next_undo_lsn){}

//log	
log_t::log_t(info_t* info):info(info){}

void log_t::redo(bufferManager* bm){
	return info->redo(bm);
}

void log_t::undo(bufferManager* bm){
	return info->undo(bm);
}

int64_t log_t::get_lsn(){
	return info->get_lsn();
}

int64_t log_t::get_prev_lsn(){
	return info->get_prev_lsn();
}

int32_t log_t::get_trx_id(){
	return info->get_trx_id();
}

int32_t log_t::get_type(){
	return info->get_type();
}

//logManager
logManager::logManager():lsn(0){}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type){
	switch(type){
		case BEGIN :
			return new log_t(new begin_info_t(lsn, prev_lsn, trx_id));
		case COMMIT :
			return new log_t(new commit_info_t(lsn, prev_lsn, trx_id));
		case ROLLBACK :
			return new log_t(new rollback_info_t(lsn, prev_lsn, trx_id));
		default:
			return NULL;
	}
}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image){
	switch(type){
		case UPDATE :
			return new log_t(new update_info_t(lsn, prev_lsn, trx_id, table_id, pageNum, offset, 120, old_image, new_image));
		default:
			return NULL;
	}
}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image, int64_t next_undo_lsn){
	switch(type){
		case COMPENSATE_UPDATE :
			return new log_t(new compensate_update_info_t(lsn, prev_lsn, trx_id, table_id, pageNum, offset, 120, old_image, new_image, next_undo_lsn));
		default:
			return NULL;
	}
}


