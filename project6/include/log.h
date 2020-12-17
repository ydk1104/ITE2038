#ifndef __log__
#define __log__

#include <atomic>
#include <mutex>
#include <set>
#include <vector>
#include"type.h"

#pragma pack(push, 1)

enum{
	BEGIN,
	UPDATE,
	COMMIT,
	ROLLBACK,
	COMPENSATE_UPDATE,
};

class info_t;

class log_t{
private:
	info_t* info;
	char* data_ptr;
public:
	log_t(info_t* info);
	log_t(info_t* info, char* ptr);
	int redo(bufferManager* bm);
	void undo(bufferManager* bm);
	void undo_with_log(bufferManager* bm, logManager* lm);
	int32_t get_log_size();
	int64_t get_lsn();
	int64_t get_prev_lsn();
	int32_t get_trx_id();
	int32_t get_type();
	int32_t get_table_id();
	int64_t get_next_undo_lsn();
	~log_t();
};

class info_t{
private:
	int32_t log_size;
	int64_t lsn;
	int64_t prev_lsn;
	int32_t trx_id;
	int32_t type;
public:
	info_t(char* data);
	info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type);
	void write(char*);
	void read(char*);
	int32_t get_log_size();
	int64_t get_lsn();
	int64_t get_prev_lsn();
	int32_t get_trx_id();
	int32_t get_type();
	virtual int32_t get_table_id();
	virtual int64_t get_next_undo_lsn();
	virtual int redo(bufferManager* bm);
	virtual void undo(bufferManager* bm);
	virtual void undo_with_log(bufferManager* bm, logManager* lm);
	virtual ~info_t();
};

class operator_info_t:public info_t{
private:
	int32_t table_id;
	pagenum_t pageNum;
	int32_t offset;
	int32_t data_length;
	char old_image[120];
	char new_image[120];
public:
	operator_info_t(char* data);
	operator_info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image);
	int32_t get_table_id();
	int redo(bufferManager* bm);
	void undo(bufferManager* bm);
	void undo_with_log(bufferManager* bm, logManager* lm);
	virtual void undo_logging(logManager* lm);
	virtual ~operator_info_t();
};


class begin_info_t:public info_t{
public:
	begin_info_t(char* data);
	begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class commit_info_t:public info_t{
public:
	commit_info_t(char* data);
	commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class rollback_info_t:public info_t{
public:
	rollback_info_t(char* data);
	rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class update_info_t:public operator_info_t{
public:
	update_info_t(char* data);
	update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image);
	void undo_logging(logManager* lm);
};

class compensate_update_info_t:public operator_info_t{
private:
	int64_t next_undo_lsn;
public:
	compensate_update_info_t(char* data);
	compensate_update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image, int64_t next_undo_lsn);
	int64_t get_next_undo_lsn();
};

class logManager{
private:
//	bufferManager* bm;
//	std::atomic<int64_t> lsn, offset;
	int64_t lsn, offset, remove_offset;
	std::mutex logBufferLatch;
	char* data;
	int fd;
public:
	logManager();
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image, int64_t next_undo_lsn);
	log_t* make_log_t(char* data_ptr);
	void open_log(char* pathname);
	void analysis(std::set<int>& loser, std::set<int>& winner, std::vector<log_t*>& logs, int* table_ids);
	void flush();
	void truncate(char* path, int len);
};

#pragma pack(pop)

#endif
