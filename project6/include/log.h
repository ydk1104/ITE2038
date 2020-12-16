#ifndef __log__
#define __log__

#include <atomic>
#include <mutex>
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
	log_t(info_t* info, char* ptr);
	void redo(bufferManager* bm);
	void undo(bufferManager* bm);
	int64_t get_lsn();
	int64_t get_prev_lsn();
	int32_t get_trx_id();
	int32_t get_type();
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
	int64_t get_lsn();
	int64_t get_prev_lsn();
	int32_t get_trx_id();
	int32_t get_type();
	virtual void redo(bufferManager* bm);
	virtual void undo(bufferManager* bm);
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
	void redo(bufferManager* bm);
	void undo(bufferManager* bm);
	virtual ~operator_info_t();
};


class begin_info_t:public info_t{
public:
	begin_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class commit_info_t:public info_t{
public:
	commit_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class rollback_info_t:public info_t{
public:
	rollback_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id);
};

class update_info_t:public operator_info_t{
public:
	update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image);
};

class compensate_update_info_t:public operator_info_t{
private:
	int32_t next_undo_lsn;
public:
	compensate_update_info_t(int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, char* old_image, char* new_image, int64_t next_undo_lsn);
};

class logManager{
private:
//	bufferManager* bm;
//	std::atomic<int64_t> lsn, offset;
	int64_t lsn, offset;
	std::mutex logBufferLatch;
	char* data;
	int fd;
public:
	logManager();
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image, int64_t next_undo_lsn);
	log_t* make_log_t(char* data_ptr);
	void flush();
};

#pragma pack(pop)

#endif
