#ifndef __log__
#define __log__

#include"type.h"

#pragma pack(push, 1);

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
	log_t(int type);
};

class info_t{
private:
	int32_t log_size;
	int64_t lsn;
	int64_t prev_lsn;
	int32_t trx_id;
	int32_t type;
public:
	info_t(int32_t log_size, int64_t lsn, int64_t prev_lsn, int32_t trx_id, int32_t type);
	void write();
	void read();
};

class operator_info_t:public info_t{
private:
	int32_t table_id;
	pagenumt_t pagenNum;
	int32_t offset;
	int32_t data_length;
	record old_image;
	record new_image;
public:
	operator_info_t(int32_t log_size, int64_t lsn, int32_t prev_lsn, int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, int32_t data_length, record* old_image, record* new_image);
	void redo();
	void undo();
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
private:
public:
	update_log_t();
};

class compensate_info_t:public operator_info_t{
private:
	int32_t next_undo_lsn;
public:
	compensate_update_info_t();
};

class info_t_factory{
public:
};

class logManager{
private:
	buffer;
	log_t* log;
	int64_t lsn;
public:
	static info_t* make_info_t(int64_t prev_lsn, int32_t trx_id, int32_t type);
	static inf_t* make_info_t(int64_t prev_lsn, int32_t trx_id, int32_t type, record* old_image, record* new_image, int64_t next_undo_lsn);
};

#pragma pack(pop);

#endif
