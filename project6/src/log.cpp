#include "log.h"

info_t::info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type):
	log_size(log_size),lsn(prev_lsn),trx_id(trx_id),type(type){}

//write to / read from buffer
void info_t::write(char* data_ptr){
	memcpy(data_ptr, this, log_size);
}

void info_t::read(char* data_ptr){
	int log_size = *(int*)data_ptr;
	memcpy(this, data_ptr, log_size);
}

operator_info_t::operator_info_t(int32_t log_size, int64_t lsn, int32_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length):
	info_t(log_size, lsn, prev_lsn, trx_id, type),
	table_id(table_id, pageNum(pageNum), offset(offset), data_length(data_length), old_image(*old_image), new_image(*new_image){}

//physical redo & undo
//interact with buffer manger
void operator_info_t::redo(){
//	get_page(table_id, pageNum);
//	memcpy(page->data[offset], new_image, data_length);
}

void operator_info_t::undo(){
//	get_page(table_id, pageNum);
//	memcpy(page->data[offset], old_image, data_length);
}

begin_info_t::begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, BEGIN){}

update_info_t::update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, record* old_image, record* new_image):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image){}

commit_info_t::commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMMIT){}

rollback_info_t::rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, ROLLBACK){}

compensate_update_info_t::compensate_update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int64_t next_undo_lsn):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMPENSATE_UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image),
	next_undo_lsn(next_undo_lsn){}

//log	
log_t::log_t(info_t* info):info(info){}

static log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type){
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

static log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, record* old_image, record* new_image){
	switch(type){
		case UPDATE :
			return new log_t(new update_info_t(lsn, prev_lsn, trx_id, old_image, new_image));
		default:
			return NULL;
	}
}

static log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, record* old_image, record* new_image, int64_t next_undo_lsn){
	switch(type){
		case COMPENSATE_UPDATE :
			return new log_t(new compensate_update_info_t(lsn, prev_lsn, trx_id, old_image, new_image, next_undo_lsn));
		default:
			return NULL;
	}
}


