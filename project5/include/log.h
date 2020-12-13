#ifndef __log__
#define __log__

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
	void redo();
	void undo();
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
	virtual void redo();
	virtual void undo();
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
	void redo();
	void undo();
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
	int64_t lsn;
public:
	logManager();
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image);
	log_t* make_log_t(int64_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image, int64_t next_undo_lsn);
};

#pragma pack(pop)

#endif
