#ifndef INCLUDED_POINT_H
#define INCLUDED_POINT_H

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <sys/types.h>
#include <stdint.h>
#include <utility>

#pragma pack(1)

struct Point{
public:
	uint64_t m_y;
	uint32_t m_x;
	Point(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}
};


template<int step>
struct TemPoint{
public:	
	// uint64_t m_x;
	uint64_t m_y;
	uint32_t m_x;

	TemPoint(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}
};

template<>
struct TemPoint<2>{
public:
	uint32_t m_x;
	uint16_t m_y;

	TemPoint(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}
};

template<>
struct TemPoint<3>{
public:
	uint32_t m_x;
	uint32_t m_y;

	TemPoint(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}
	
};

template<>
struct TemPoint<4>{
public:
	uint32_t m_x;
	uint32_t m_y;

	TemPoint(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}	
};

template<>
struct TemPoint<8>{
public:
	uint32_t m_x;
	uint64_t m_y;

	TemPoint(uint64_t x = 0, uint64_t y = 0): m_x(x), m_y(y){

	}	
};

#pragma pack()//恢复对齐状态

void* newPointVector(uint64_t step,uint64_t& mem);
void *newPointVector(uint64_t step, uint64_t size, uint64_t& mem);
uint64_t pointGetX(void*point, uint64_t step, uint64_t idx);
uint64_t pointGetY(void* point, uint64_t step, uint64_t idx);
void pointPushBack(void * point, uint64_t step, uint64_t x, uint64_t y);
uint64_t pointSize(void * point, uint64_t step);
void point_nth_element(void* point, 
				uint64_t step, 
				uint64_t kth, 
				std::pair<uint64_t, uint64_t> range, 
				int32_t depth);
void deletePoint(void *point, uint64_t step);
#endif