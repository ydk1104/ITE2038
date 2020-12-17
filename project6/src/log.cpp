#include "buffer.h"
#include "log.h"

info_t::info_t(char* data){
	read(data);
	printf("%d %d\n", type, lsn);
}

info_t::info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type):
	log_size(log_size - 8),lsn(lsn),prev_lsn(prev_lsn),trx_id(trx_id),type(type){}

//write to / read from buffer
void info_t::write(char* data_ptr){
	memcpy(data_ptr,(char*)this + 8, log_size);
}

void info_t::read(char* data_ptr){
	int32_t log_size = *(int32_t*)data_ptr;
	memcpy((char*)this + 8, data_ptr, log_size);
}

int32_t info_t::get_log_size(){
	return log_size;
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

int32_t info_t::get_table_id(){
	return 0;
}

int64_t info_t::get_next_undo_lsn(){
	return 0;
}

// override
int info_t::redo(bufferManager* bm){return 0;}
void info_t::undo(bufferManager* bm){}
info_t::~info_t(){}

operator_info_t::operator_info_t(char* data):info_t(data){}
operator_info_t::operator_info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image):
	info_t(log_size, lsn, prev_lsn, trx_id, type),
	table_id(table_id), pageNum(pageNum), offset(offset), data_length(data_length){
		memcpy(&this->old_image, old_image, data_length);
		memcpy(&this->new_image, new_image, data_length);
	}
operator_info_t::~operator_info_t(){}

int32_t operator_info_t::get_table_id(){
	return table_id;
}

//physical redo & undo
//interact with buffer manger
int operator_info_t::redo(bufferManager* bm){
	node* n = NULL;
	bm->page_to_node(table_id, pageNum, &n);
	if(n->pageLSN >= get_lsn()){
		//TODO : is_dirty isn't true
		bm->node_to_page(&n, true);
		return 1;
	}
	char* ptr = (char*)n->buffer_ptr;
	ptr += offset;
	memcpy(ptr, new_image, data_length);
	bm->node_to_page(&n, true);
	return 0;
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

begin_info_t::begin_info_t(char* data):info_t(data){}
begin_info_t::begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, BEGIN){}

update_info_t::update_info_t(char* data):operator_info_t(data){}
update_info_t::update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image){}

commit_info_t::commit_info_t(char* data):info_t(data){}
commit_info_t::commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMMIT){}

rollback_info_t::rollback_info_t(char* data):info_t(data){}
rollback_info_t::rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id):
	info_t(sizeof(*this), lsn, prev_lsn, trx_id, ROLLBACK){}

compensate_update_info_t::compensate_update_info_t(char* data):operator_info_t(data){}
compensate_update_info_t::compensate_update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image, int64_t next_undo_lsn):
	operator_info_t(sizeof(*this), lsn, prev_lsn, trx_id, COMPENSATE_UPDATE,
		table_id, pageNum, offset, data_length, old_image, new_image),
	next_undo_lsn(next_undo_lsn){}

int64_t compensate_update_info_t::get_next_undo_lsn(){
	return next_undo_lsn;
}

//log

log_t::log_t(info_t* info):info(info){}

log_t::log_t(info_t* info, char* data_ptr):info(info){
	info->write(data_ptr);
}

log_t::~log_t(){delete info;}

int log_t::redo(bufferManager* bm){
	return info->redo(bm);
}

void log_t::undo(bufferManager* bm){
	return info->undo(bm);
}

int32_t log_t::get_log_size(){
	return info->get_log_size();
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

int32_t log_t::get_table_id(){
	return info->get_table_id();
}

int64_t log_t::get_next_undo_lsn(){
	return info->get_next_undo_lsn();
}

//logManager
//1048576 = 1MB, logBuffer size = 100MB
logManager::logManager():lsn(0),offset(0),data(new char[104857600]){}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type){
	std::unique_lock<std::mutex> l(logBufferLatch);
	auto temp_offset = lsn - this->offset;
	auto temp_lsn = lsn;
	lsn += sizeof(begin_info_t) - 8;
	switch(type){
		case BEGIN :
			return new log_t(new begin_info_t(temp_lsn, prev_lsn, trx_id),
							data+temp_offset);
		case COMMIT :
			return new log_t(new commit_info_t(temp_lsn, prev_lsn, trx_id),
							data+temp_offset);
		case ROLLBACK :
			return new log_t(new rollback_info_t(temp_lsn, prev_lsn, trx_id),
							data+temp_offset);
		default:
			return NULL;
	}
}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image){
	std::unique_lock<std::mutex> l(logBufferLatch);
	auto temp_offset = lsn - this->offset;
	auto temp_lsn = lsn;
	lsn += sizeof(update_info_t) - 8;
	switch(type){
		case UPDATE :
			return new log_t(new update_info_t(temp_lsn, prev_lsn, trx_id, table_id, pageNum, offset, 120, old_image, new_image),
							data+temp_offset);
		default:
			return NULL;
	}
}

log_t* logManager::make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image, int64_t next_undo_lsn){
	std::unique_lock<std::mutex> l(logBufferLatch);
	auto temp_offset = lsn - this->offset;
	auto temp_lsn = lsn;
	lsn += sizeof(update_info_t) - 8;
	switch(type){
		case COMPENSATE_UPDATE :
			return new log_t(new compensate_update_info_t(temp_lsn, prev_lsn, trx_id, table_id, pageNum, offset, 120, old_image, new_image, next_undo_lsn),
							data+temp_offset);
		default:
			return NULL;
	}
}

log_t* logManager::make_log_t(char* data_ptr){
	//&info_t::type
	//hardcode because of info_t::type is private
	int32_t type = *(int32_t*)(data_ptr + 24);
	switch(type){
		case BEGIN :
			return new log_t(new begin_info_t(data_ptr));
		case UPDATE :
			return new log_t(new update_info_t(data_ptr));
		case COMMIT :
			return new log_t(new commit_info_t(data_ptr));
		case ROLLBACK :
			return new log_t(new rollback_info_t(data_ptr));
		case COMPENSATE_UPDATE :
			return new log_t(new compensate_update_info_t(data_ptr));
	}
}

void logManager::open_log(char* pathname){
	//TODO : use file API
	fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
	lsn = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	read(fd, data, lsn);
}

void logManager::analysis(std::set<int>& loser, std::set<int>& winner, std::vector<log_t*>& logs, int* table_ids){
	if(lsn == 0) return;
	int64_t temp_offset = 0;
	do{
		log_t* temp_ptr = make_log_t(data + temp_offset);
		logs.emplace_back(temp_ptr);
		switch(temp_ptr->get_type()){
			case BEGIN:
				loser.insert(temp_ptr->get_trx_id());
				break;
			case COMMIT:
			case ROLLBACK:
				loser.erase(temp_ptr->get_trx_id());
				winner.insert(temp_ptr->get_trx_id());
			case UPDATE:
			case COMPENSATE_UPDATE:
				table_ids[temp_ptr->get_table_id()] = true;
			default:
				break;
		}
		temp_offset += temp_ptr->get_log_size();
	}while(temp_offset < lsn);
	offset = logs[0]->get_lsn();
}

void logManager::flush(){
	//TODO : use file API
	std::unique_lock<std::mutex> l(logBufferLatch);
	lseek(fd, offset - remove_offset, SEEK_SET);
	write(fd, data, lsn - offset);
	offset = lsn;
}

void logManager::truncate(char* path, int len){
	ftruncate(fd, len);
	offset = lsn;
	remove_offset = offset;
}

