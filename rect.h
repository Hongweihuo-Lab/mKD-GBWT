#ifndef INCLUDED_RECT_H
#define INCLUDED_RECT_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
//定义一个矩形，这个矩形有四条边

class Rect{

public:
	uint64_t m_lx;
	uint64_t m_hx;
	uint64_t m_ly;
	uint64_t m_hy;

	bool 	b_lx;
	bool 	b_hx;
	bool	b_ly;
	bool 	b_hy;

public:
	//构造函数
	Rect(uint64_t lx, uint64_t hx, uint64_t ly, uint64_t hy, 
		bool blx = true, bool bhx = true, bool bly = true, bool bhy = true);
	
	// Default Constructor, It must be needed!!
	Rect();

	//判断一个点是否在这个矩形中
	bool isInRange(uint64_t x, uint64_t y) const;

	//判断两个矩形是否相交
	bool isIntersected(const Rect& r) const;

	//判断是否包含矩形R
	bool isContained(const Rect& r) const;

	uint64_t getLowX(){return m_lx;}
	uint64_t getHighX(){return m_hx;}
	uint64_t getLowY(){return m_ly;}
	uint64_t getHighY(){return m_hy;}

	void setLowX(uint64_t lx) { m_lx = lx;}
	void setLowX(uint64_t lx, bool blx) { m_lx = lx; b_lx = blx;}
	void setHighX(uint64_t hx) { m_hx = hx;}
	void setHighX(uint64_t hx, bool bhx) { m_hx = hx; b_hx = bhx;}
	void setLowY(uint64_t ly) { m_ly = ly;}
	void setLowY(uint64_t ly, bool bly) { m_ly = ly; b_ly = bly;}
	void setHighY(uint64_t hy) { m_hy = hy;}
	void setHighY(uint64_t hy, bool bhy) { m_hy = hy; b_hy = bhy;}

	int32_t writeToDisk(FILE* out);

	static int32_t BytesInDisk();

	static Rect GetRectFromMem(uint8_t* buf);
};


#endif