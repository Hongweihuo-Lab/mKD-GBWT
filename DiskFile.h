#ifndef INCLUDED_DISKFILE_H
#define INCLUDED_DISKFILE_H

#include <cstdio>
#include <cstddef>
#include <cstdint>


class DiskFile{

public:
	FILE*  		m_out;			//以写入的方式打开文件
	int32_t		m_diskNum;		//需要写入的磁盘号

public:
	DiskFile(FILE* out, int32_t diskNum);	
	~DiskFile();
};






#endif
