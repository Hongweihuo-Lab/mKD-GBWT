#include "test_kdtree.h"

using namespace std;

void test_kdtree::Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWidth) {
	KDTree::Set(B, xBitWidth, yBitWidth, SABitWidth);
}


void test_kdtree::test_ReadFromDisk() {
	// const char * fileName = "raw_data/tmp1.kdt";
	// const char * pointName = "raw_data/tmp1.point";
	// uint32_t B = 4*1024;
	// // uint32_t B = 1024;
	// //构建一颗KD-树。
	// DiskKDTree diskkdt(fileName, B);

	// //找到要访问的点集,便于验证正确性。
	// vector<pair<uint64_t, uint64_t> > *points = readPointsFromFile(pointName);

	// Rect r(1, 25*1024*1024-1, 1, 25*1024*1024-1);

	// uint64_t n = 10;  //随机验证10000次.
	// uint64_t m = points->size();  //点的总数

	// for(uint64_t i = 0; i < n; i++) {
	// 	//每次随机生成查询矩形。
	// 	uint64_t x;
	// 	cout << "请输入查询矩形： " ;
	// 	cin >> x; r.setLowX(x);
	// 	cin >> x; r.setHighX(x);
	// 	cin >> x; r.setLowY(x);
	// 	cin >> x; r.setHighY(x);

	// 	//在diskKDT中进行查询，找到需要的结果
	// 	vector<pair<uint64_t, uint64_t> >* result = diskkdt.Locate(r);

	// 	//对结果进行验证。
	// 	uint64_t pNum = 0;
	// 	for(uint64_t k = 0; k < m; k++){
	// 		pair<uint64_t, uint64_t> p = (*points)[k];
	// 		if(r.isInRange(p.first, p.second))
	// 			pNum++;
	// 	}
		
	// 	//如果二者的数量不想等,表示有错误。
	// 	if(pNum != result->size()) {
	// 		/*其实将这些结果记录到日志里很有好处，可以便于调试错误*/	
	// 		cout << "\n----------------------" <<endl;
	// 		cout << "i = " << i << endl;
	// 		cout << "rect = (" << r.m_lx << ", " << r.m_hx << ", "
	// 							<< r.m_ly << ", " << r.m_hy << ")" << endl;
	// 		cout << "DiskKDT.Locate(): size = " << result->size() << endl;
	// 		cout << "the other size = " << pNum << endl;
	// 		cout << "------------------------" <<endl;
	// 	} else {
	// 		cout << "--Successed! (" << result->size() << ", " << pNum << ")" << endl;
	// 	}
	// }

}


void test_kdtree::test_SaveToDisk() {

	Set(4*1024, 46, 30, 22);
	// Set(1024, 46, 30);

	clock_t build_time = 0, locate_time = 0;

	vector<uint64_t> vx, vy;

	uint64_t n = 10 * 1024 * 1024;

	for(uint64_t i = 0; i < n; i++){
		vx.push_back(i);
		vy.push_back(i);
	}

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle(vx.begin(), vx.end(), std::default_random_engine(seed));
	seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle(vy.begin(), vy.end(), std::default_random_engine(seed));

	build_time = std::clock();  // measure time
	KDTree tree(vx, vy);
	tree.SaveToDiskFile("raw_data/tmp1.kdt");
	build_time = std::clock() - build_time;

	// pair<int, int> px = make_pair(0, 6200);
	// pair<int, int> py = make_pair(0, 6000);

	/**为什么测试locate的时间很少? 经常就是0，但是结果是正确的**/
	// locate_time = clock();
	// vector<pair<int, int> >* result = tree.locate(px, py);
	// locate_time = clock() - locate_time;

	// for(vector<pair<int, int> >::iterator it = result->begin(); it != result->end(); it++){
	// 	//cout << "(" << (it->first) << ", " << (it->second) << ")" << endl;
	// }

	cout << "build_time = " << ((double)build_time) / CLOCKS_PER_SEC << endl;
	cout << "locate_time = " << ((double)locate_time) / CLOCKS_PER_SEC << ", clocks = " 
			<< locate_time << endl;
	cout << "n_AllNode = " << KDTree::n_AllNode << endl;
	cout << "n_PointNode = " << KDTree::n_PointNode << endl;
	cout << "n_Point = " << KDTree::n_Point << endl;

	cout << endl;

	//将点存储进去文件中，便于后续进行正确性判断。
	const char* pointFile = "raw_data/tmp1.point";
	
	uint64_t * x_arr = new uint64_t[n];
	uint64_t * y_arr = new uint64_t[n];
	for(uint64_t i = 0; i < n; i++){
		x_arr[i] = vx[i];
		y_arr[i] = vy[i];
	}
	
	writePointsToFile(pointFile, n, x_arr, y_arr);
	delete []x_arr;
	delete []y_arr;
}



void test_kdtree::writePointsToFile(const char* pointFile, uint64_t n, uint64_t* x_arr, uint64_t* y_arr){

	FILE* out = fopen(pointFile, "w");
	if(!out) {
		fprintf(stderr, "writePointsToFile: can't open file %s.\n", pointFile);
		exit(1);
	}

	fwrite(&n, 1, sizeof(uint64_t), out);
	fwrite(x_arr, 1, n * sizeof(uint64_t), out);
	fwrite(y_arr, 1, n*sizeof(uint64_t), out);
	
	fclose(out);
}


vector<pair<uint64_t, uint64_t> >* test_kdtree::readPointsFromFile(const char* pointFile) {

	FILE* in = fopen(pointFile, "r");
	if(!in) {
		fprintf(stderr, "readPointsFromFile: can't open file %s.\n", pointFile);
		exit(1);
	}
	
	uint64_t n, nsize = 0;
	nsize += fread(&n, 1, sizeof(uint64_t), in);
	uint64_t * x_arr = new uint64_t[n];
	uint64_t * y_arr = new uint64_t[n];

	nsize += fread(x_arr, 1, sizeof(uint64_t)*n, in);
	nsize += fread(y_arr, 1, sizeof(uint64_t)*n, in);

	fclose(in);

	vector<pair<uint64_t, uint64_t> >* result = new vector<pair<uint64_t, uint64_t> >(n);

	for(uint64_t i = 0; i < n; i++){
		(*result)[i] = make_pair(x_arr[i], y_arr[i]);
	}

	delete []x_arr;
	delete []y_arr;

	return result;
}







/**测试100万条数据的运行时间 **
*
* 100万条数据，随机排序，运行时间为0.5s。（大大超出了预期）
* 
*****/
void test_kdtree::test_quickSort(){
	vector<int> v;
	clock_t time;

	for(int i = 0; i < 1000000; i++) {
		v.push_back(i+1);
	}

	std::random_shuffle(v.begin(), v.end());
	
	time = std::clock();
	std::sort(v.begin(), v.end());
	time = std::clock() - time;

	cout << ((double)time) / CLOCKS_PER_SEC  << endl;
}


/****测试C++取中位数的函数******
*
* 一次测试时间是0.04s，惊呆我了。
* 看来，如果我的kd-tree 改进的话，是不是有很大的改进空间。
* 构造KD-tree 的主要过程就是选取中位数，我的算法太慢了。
********************************/
void test_kdtree::test_nth(){
	vector<int> v;
	clock_t time;
	int k;
	
	long n = 1000000;
	for(int i = 1; i <= n; i++)
		v.push_back(i);
	std::random_shuffle(v.begin(), v.end());
	k = (1+n)/2;

	time = std::clock();
	std::nth_element(v.begin(), v.begin() + k, v.end());
	time = std::clock()-time;
	cout <<"nth_element() time = " << ((double)time) / CLOCKS_PER_SEC << endl;
}