#include <assert.h>
#include "point.h"


using namespace std;

void* newPointVector(uint64_t step, uint64_t& mem){
	void *point;
	switch(step){
		case 2:
			point = new vector<TemPoint<2> >();
			mem = ((vector<TemPoint<2> >*)point)->capacity() * sizeof(TemPoint<2>);
			break;
		case 3:
			point = new vector<TemPoint<3> >();
			mem = ((vector<TemPoint<3> >*)point)->capacity() * sizeof(TemPoint<3>);
			break;
		case 4:
			point = new vector<TemPoint<4> >();
			mem = ((vector<TemPoint<4> >*)point)->capacity() * sizeof(TemPoint<4>);
			break;
		default:
			point = new vector<TemPoint<8> >();
			mem = ((vector<TemPoint<8> >*)point)->capacity() * sizeof(TemPoint<8>);
	}

	return point;
}


void *newPointVector(uint64_t step, uint64_t size, uint64_t& mem){
	void *point;
	switch(step){
		case 2:
			point = new vector<TemPoint<2> >();
			((vector<TemPoint<2> >*)point)->reserve(size);
			mem = ((vector<TemPoint<2> >*)point)->capacity() * sizeof(TemPoint<2>);
			break;
		case 3:
			point = new vector<TemPoint<3> >();
			((vector<TemPoint<3> >*)point)->reserve(size);
			mem = ((vector<TemPoint<3> >*)point)->capacity() * sizeof(TemPoint<3>);
			break;
		case 4:
			point = new vector<TemPoint<4> >();
			((vector<TemPoint<4> >*)point)->reserve(size);
			mem = ((vector<TemPoint<4> >*)point)->capacity() * sizeof(TemPoint<4>);
			break;
		default:
			point = new vector<TemPoint<8> >();
			((vector<TemPoint<8> >*)point)->reserve(size);
			mem = ((vector<TemPoint<8> >*)point)->capacity() * sizeof(TemPoint<8>);
	}

	return point;	
}

uint64_t pointGetX(void*point, uint64_t step, uint64_t idx){
	switch(step){
		case 2:
			return (*(vector<TemPoint<2> >*)point)[idx].m_x;
			break;
		case 3:
			return (*(vector<TemPoint<3> >*)point)[idx].m_x;
			break;
		case 4:
			return (*(vector<TemPoint<4> >*)point)[idx].m_x;
			break;
		default:
			return (*(vector<TemPoint<8> >*)point)[idx].m_x;
	}
}

uint64_t pointGetY(void* point, uint64_t step, uint64_t idx){
	switch(step){
		case 2:
			return (*(vector<TemPoint<2> >*)point)[idx].m_y;
			break;
		case 3:
			return (*(vector<TemPoint<3> >*)point)[idx].m_y;
			break;
		case 4:
			return (*(vector<TemPoint<4> >*)point)[idx].m_y;
			break;
		default:
			return (*(vector<TemPoint<8> >*)point)[idx].m_y;
	}
}

void pointPushBack(void * point, uint64_t step, uint64_t x, uint64_t y){
	switch(step){
		case 2:
			((vector<TemPoint<2> >*)point)->push_back(TemPoint<2>(x, y));
			return;
			break;
		case 3:
			((vector<TemPoint<3> >*)point)->push_back(TemPoint<3>(x, y));
			return;
			break;
		case 4:
			((vector<TemPoint<4> >*)point)->push_back(TemPoint<4>(x, y));
			return;
			break;
		default:
			((vector<TemPoint<8> >*)point)->push_back(TemPoint<8>(x, y));
			return;
	}	
}

uint64_t pointSize(void * point, uint64_t step){
	uint64_t size;
	switch(step){
		case 2:
			size = ((vector<TemPoint<2> >*)point)->size();
			break;
		case 3:
			size = ((vector<TemPoint<3> >*)point)->size();
			break;
		case 4:
			size = ((vector<TemPoint<4> >*)point)->size();
			break;
		default:
			size = ((vector<TemPoint<8> >*)point)->size();

	}
	return size;
}


// static bool cmp_x(TemPoint<8> a, TemPoint<8> b){
// 	return a.m_x < b.m_x;
// }

// static bool cmp_y(TemPoint<8> a, TemPoint<8> b){
// 	return a.m_y < b.m_y;
// }

static bool cmp_x2(TemPoint<2> a, TemPoint<2> b){
	return a.m_x < b.m_x;
}

static bool cmp_y2(TemPoint<2> a, TemPoint<2> b){
	return a.m_y < b.m_y;
}

static bool cmp_x3(TemPoint<3> a, TemPoint<3> b){
	return a.m_x < b.m_x;
}

static bool cmp_y3(TemPoint<3> a, TemPoint<3> b){
	return a.m_y < b.m_y;
}

static bool cmp_x4(TemPoint<4> a, TemPoint<4> b){
	return a.m_x < b.m_x;
}

static bool cmp_y4(TemPoint<4> a, TemPoint<4> b){
	return a.m_y < b.m_y;
}

static bool cmp_x8(TemPoint<8> a, TemPoint<8> b){
	return a.m_x < b.m_x;
}

static bool cmp_y8(TemPoint<8> a, TemPoint<8> b){
	return a.m_y < b.m_y;
}


void point_nth_element(void* point, uint64_t step, uint64_t kth, pair<uint64_t, uint64_t> range, int32_t depth){

	uint64_t size = range.second - range.first + 1;
	switch(step){
		case 2:
			{
				vector<TemPoint<2> >* vp = (vector<TemPoint<2> >*)point;
				vector<TemPoint<2> >::iterator it = vp->begin();
				vector<TemPoint<2> >::iterator beg = it + range.first;
				if(depth % 2 == 1)
					nth_element(beg, beg + kth, beg + size, cmp_x2);
				else
					nth_element(beg, beg + kth, beg + size, cmp_y2);
				break;
			}
		case 3:
			{
				vector<TemPoint<3> >* vp = (vector<TemPoint<3> >*)point;
				vector<TemPoint<3> >::iterator it = vp->begin();
				vector<TemPoint<3> >::iterator beg = it + range.first;
				if(depth % 2 == 1)
					nth_element(beg, beg + kth, beg + size, cmp_x3);
				else
					nth_element(beg, beg + kth, beg + size, cmp_y3);	
				break;
			}
		case 4:
			{
				vector<TemPoint<4> >* vp = (vector<TemPoint<4> >*)point;
				vector<TemPoint<4> >::iterator it = vp->begin();
				vector<TemPoint<4> >::iterator beg = it + range.first;
				if(depth % 2 == 1)
					nth_element(beg, beg + kth, beg + size, cmp_x4);
				else
					nth_element(beg, beg + kth, beg + size, cmp_y4);
				break;
			}
		default:
			{
				vector<TemPoint<8> >* vp = (vector<TemPoint<8> >*)point;
				vector<TemPoint<8> >::iterator it = vp->begin();
				vector<TemPoint<8> >::iterator beg = it + range.first;
				if(depth % 2 == 1)
					nth_element(beg, beg + kth, beg + size, cmp_x8);
				else
					nth_element(beg, beg + kth, beg + size, cmp_y8);
				return;	
			}		
	}
}

void deletePoint(void *point, uint64_t step){
	if(point == NULL){
		assert(false);
	}
	switch(step){
	case 2:
		delete (vector<TemPoint<2> >*)point;
		return;
		break;
	case 3:
		delete (vector<TemPoint<3> >*)point;
		return;
		break;
	case 4:
		delete (vector<TemPoint<4> >*)point;
		return;
		break;
	default:
		delete (vector<TemPoint<8> >*)point;
		return;
	}
}
