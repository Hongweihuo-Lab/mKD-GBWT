#include "rect.h"
#include <algorithm>

using std::min;
using std::max;

// 构造函数
Rect::Rect(uint64_t lx, uint64_t hx, uint64_t ly, uint64_t hy, bool blx, bool bhx, bool bly, bool bhy) {
	m_lx = lx;
	m_hx = hx;
	m_ly = ly;
	m_hy = hy;
	b_lx = blx;
	b_hx = bhx;
	b_ly = bly;
	b_hy = bhy;
}

//Default Constructor!!
Rect::Rect(){
	m_lx = m_hx = m_ly = m_hy = 0;
	b_lx = b_hx = b_ly = b_hy = false;
}

// 判断一个点是否在这个矩形中。
bool Rect::isInRange(uint64_t x, uint64_t y) const {
	if( (m_lx < x && x < m_hx) || (b_lx == true && m_lx == x)  || (b_hx == true && m_hx == x)){
	
		if((m_ly < y && y < m_hy) || (b_ly == true && m_ly == y) || (b_hy == true && m_hy == y))
			return true;
	}
	return false;				
}

// 判断两个矩形是否相交
bool Rect::isIntersected(const Rect& r) const {
	//两个矩形相交之后依旧是个矩形，求取相交之后矩形的坐标。
	uint64_t minx = max(this->m_lx, r.m_lx);
	uint64_t maxx = min(this->m_hx, r.m_hx);
	uint64_t miny = max(this->m_ly, r.m_ly);
	uint64_t maxy = min(this->m_hy, r.m_hy);
	
	if(minx > maxx || miny > maxy)  
		return false;
	if(minx == maxx) {
		//两个矩形相交于一条y轴的边
		if(m_hx < r.m_hx){
			if(b_hx == false || r.b_lx == false)
				return false;
		}
		if(r.m_hx < m_hx) {
			if(r.b_hx == false || this->b_lx == false )
				return false;
		}
	}

	if(miny == maxy) {
		//两个矩形相交于一条x轴的边上。
		if(m_hy < r.m_hy) {
			if(this->b_hy == false || r.b_ly == false)
				return false;
		}

		if(r.m_hy < m_hy) {
			if(r.b_hy == false || this->b_ly == false)
				return false;
		}
		
	}

	return true;
}


//if fully contained rect r
bool Rect::isContained(const Rect& r) const {
	//判断是否完全包含矩形 r.
	if(r.m_lx <  this->m_lx)
		return false;
	if(r.m_lx == this->m_lx && r.b_lx == true && this->b_lx == false)
		return false;

	if(r.m_hx > this->m_hx)
		return false;
	if(r.m_hx == this->m_hx && r.b_hx == true && this->b_hx == false)
		return false;
	
	if(r.m_ly < this->m_ly)
		return false;
	if(r.m_ly == this->m_ly && r.b_ly == true && this->b_ly == false)
		return false;
	
	if(r.m_hy > this->m_hy)
		return false;
	if(r.m_hy == this->m_hy && r.b_hy == true && this->b_hy == false)
		return false;

	return true;
}

int32_t Rect::writeToDisk(FILE* out){

	int32_t totalBytes = 0;
	totalBytes += fwrite(&m_lx, 1, sizeof(uint64_t), out);
	totalBytes += fwrite(&m_hx, 1, sizeof(uint64_t), out);
	totalBytes += fwrite(&m_ly, 1, sizeof(uint64_t), out);
	totalBytes += fwrite(&m_hy, 1, sizeof(uint64_t), out);

	uint8_t tmp  = 0;
	if(b_lx == true)
		tmp |= 0x1;
	if(b_hx == true)
		tmp |= 0x2;
	if(b_ly == true)
		tmp |= 0x4;
	if(b_hy |= true)
		tmp |= 0x8;

	totalBytes += fwrite(&tmp, 1, sizeof(uint8_t), out);
	return totalBytes;
}

int32_t Rect::BytesInDisk(){

	return (8 + 8 + 8 + 8 + 1);  // 33 bytes.
}

Rect Rect::GetRectFromMem(uint8_t* buf){
	uint64_t lx, hx, ly, hy;
	uint8_t tmp;
	bool blx, bhx, bly, bhy;
	blx = bhx = bly = bhy = false;

	lx = *((uint64_t*)buf);
	hx = *((uint64_t*)(buf + 8));
	ly = *((uint64_t*)(buf + 16));
	hy = *((uint64_t*)(buf + 24));

	tmp = *(buf + 32);

	if(tmp & 0x1 != 0)
		blx = true;
	if(tmp & 0x2 != 0)
		bhx = true;
	if(tmp & 0x4 != 0)
		bly = true;
	if(tmp & 0x8 != 0)
		bhy = true;

	return Rect(lx, hx, ly, hy, blx, bhx, bly, bhy);
}