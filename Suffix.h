#ifndef INCLUDED_SUFFIX_H
#define INCLUDED_SUFFIX_H

#include <cstdint>

class Suffix{

public:
	uint8_t firstChara;		//这个后缀范围所在的首字母
	bool 	exist;
	int64_t left;			//表示这个首字母所在的后缀范围。
	int64_t right;

	int32_t* diskNum;	//磁盘页。

public:
	Suffix();
	~Suffix();
};


#endif