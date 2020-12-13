#include "log.h"

info_t::info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type):
		log_size(log_size),lsn(prev_lsn),trx_id(trx_id),type(type){}

void info_t::write(char* data_ptr){
	memcpy(data_ptr, this, log_size);
}
void info_t::read(char* data_ptr){
	int log_size = *(int*)data_ptr;
	memcpy(this, data_ptr, log_size);
}

begin_info_t::begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):info_t(sizeof(*this), lsn, prev_lsn, trx_id, BEGIN){}

update_info_t::update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):info_t(sizeof(*this), lsn, prev_lsn, trx_id, UPDATE){}

commit_info_t::commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMMIT){}

rollback_info_t::rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):info_t(sizeof(*this), lsn, prev_lsn, trx_id, ROLLBACK){}

compensate_update_info_t::update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int64_t next_undo_lsn):info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMPENSATE_UPDATE), next_undo_lsn(next_undo_lsn){}

static info_t* logManager::make_info_t(int64_t prev_lsn, int32_t trx_id, int32_t type){
	switch(type){
		case BEGIN :
			return new begin_info_t(lsn, prev_lsn, trx_id);
		case COMMIT :
			return new commit_info_t(lsn, prev_lsn, trx_id);
		case ROLLBACK :
			return new rollback_info_t(lsn, prev_lsn, trx_id);
		default:
			return NULL;
	}
}

static info_t* logManager::make_info_t(int64_t prev_lsn, int32_t trx_id, int32_t type, record* old_image, record* new_image){
	switch(type){
		case UPDATE :
			return new update_info_t(lsn, prev_lsn, trx_id, old_image, new_image);
		default:
			return NULL;
	}
}

static info_t* logManager::make_info_t(int64_t prev_lsn, int32_t trx_id, int32_t type, record* old_image, record* new_image, int64_t next_undo_lsn){
	switch(type){
		case COMPENSATE_UPDATE :
			return new compensate_update_info_t(lsn, prev_lsn, trx_id, old_image, new_image, next_undo_lsn);
		default:
			return NULL;
	}
}

log_t::log_t(int type):info(make_info_t(type)){}

