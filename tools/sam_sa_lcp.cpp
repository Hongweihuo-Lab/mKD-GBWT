#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sys/types.h>
#include <inttypes.h>
#include <assert.h>

using namespace std;

#define SCANBUFSIZE 100000
#define GBWT_INDEX_POSITION "/media/软件/GBWT-Index-Position/"

/*********
* 根据文本文件fileName，得到存储串B-树的文件。
* 1. 串B-树的文件存储在 文件夹 /raw_data/中，所以在当前文件夹下面一定要有 raw_data。
* 2. 文件后缀为_<setp>.sbt
* 3. 当文件名不用的时候，要记得释放返回的文件名。
**********/
static  char* getSBTFile(const char* filename, uint64_t step){
	char file1[256];
	char* outfile = new char[256];

	strcpy(file1, filename);
	int i , j;
	int len = strlen(filename);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}
	/* now, the file1 saves the filename */
	strcpy(outfile, GBWT_INDEX_POSITION);
	strcat(outfile, file1);
	strcat(outfile, "._");
	file1[0] = (step + '0');
	file1[1] = '\0';
	strcat(outfile, file1);
	strcat(outfile, ".sbt");

	return outfile;	
}


/*******
* 根据文件名 fileName和step(采样步长)得到存储磁盘上KD-Tree的文件名。
* 1. 磁盘上的KD-tree是存储在./raw_data/文件中的，所以工作目录下面一定要有 raw_data 文件加。
* 2. 文件名后缀为: _<step>.kdt
* 3. 当返回的文件名不用的时候，要记得释放内存。
********/
static char* getKDTFile(const char* filename, uint64_t step){
	char file1[256];
	char* outfile = new char[256];	//申请内存，作为结果返回。

	strcpy(file1, filename);
	int i , j;
	int len = strlen(filename);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}
	/* now, the file1 saves the filename */
	strcpy(outfile, GBWT_INDEX_POSITION);
	strcat(outfile, file1);
	strcat(outfile, "._");
	file1[0] = (step + '0');
	file1[1] = '\0';
	strcat(outfile, file1);
	strcat(outfile, ".kdt");	//文件名的后缀。

	return outfile;
}

static char* getSAFile(const char* filename, uint64_t step){
	char* sbtFile = getSBTFile(filename, step);
	char* outFile = new char[1024];
	strcpy(outFile, sbtFile);
	strcat(outFile, ".saraw");

	delete []sbtFile;
	return outFile;
}

static char* getLCPFile(const char* filename, uint64_t step){
	char* sbtFile = getSBTFile(filename, step);
	char* outFile = new char[1024];
	strcpy(outFile, sbtFile);
	strcat(outFile, ".lcp");

	delete []sbtFile;
	return outFile;
}


int main(int argc, char* argv[]){

	if(argc != 3){
		fprintf(stderr, "Usage: ./sam_sa_lcp <data> <step>\n");
		exit(EXIT_FAILURE);
	}

	string textFile = string(argv[1]);
	int32_t step = atoi(argv[2]);

	string sa64File = textFile + ".sa64";
	string lcp64File = textFile + ".lcp64";

	char* saFile = getSAFile(textFile.c_str(), step);
	char* lcpFile = getLCPFile(textFile.c_str(), step);

	FILE* fsa64 = fopen(sa64File.c_str(), "r");
	FILE* flcp64 = fopen(lcp64File.c_str(), "r");

	FILE* fsa = fopen(saFile, "w");
	FILE* flcp = fopen(lcpFile, "w");

	if(!fsa64){
		fprintf(stderr, "Open file %s error!\n", sa64File.c_str());
		exit(EXIT_FAILURE);
	}

	if(!flcp64){
		fprintf(stderr, "Open file %s error!\n", lcp64File.c_str());
		exit(EXIT_FAILURE);
	}

	if(!fsa){
		fprintf(stderr, "Open file %s error!\n", saFile);
		exit(EXIT_FAILURE);		
	}

	if(!flcp){
		fprintf(stderr, "Open file %s error!\n", lcpFile);
		exit(EXIT_FAILURE);		
	}	


	uint64_t* rsaBuf = new uint64_t[SCANBUFSIZE];
	uint64_t* wsaBuf = new uint64_t[SCANBUFSIZE];

	uint64_t* rlcpBuf = new uint64_t[SCANBUFSIZE];
	uint64_t* wlcpBuf = new uint64_t[SCANBUFSIZE];

	int64_t rsaSize = 0, wsaIdx = 0, rlcpSize = 0, wlcpIdx = 0;

	uint64_t prev_lcp_val = 0XFFFFFFFFFFFFFFFF;
	bool firstSamLcp = false;

	while((rsaSize = fread(rsaBuf, sizeof(uint64_t), SCANBUFSIZE, fsa64)) != 0){

		rlcpSize = fread(rlcpBuf, sizeof(uint64_t), SCANBUFSIZE, flcp64);
		if(rlcpSize != rsaSize){
			assert(rlcpSize == rsaSize);
		}

		int64_t rsize = rsaSize;

		for(int i = 0; i < rsize; i++){
			if(rsaBuf[i] % step == 0){
				wsaBuf[wsaIdx++] = rsaBuf[i];
				if(wsaIdx == SCANBUFSIZE){
					if(fwrite(wsaBuf, sizeof(uint64_t), wsaIdx, fsa) != wsaIdx) {
						fprintf(stderr, "sam_sa_lcp: write file %s error!\n", saFile);
						exit(EXIT_FAILURE);
					}
					wsaIdx = 0;
				}
				wlcpBuf[wlcpIdx++] = min(rlcpBuf[i], prev_lcp_val);
				if(wlcpIdx == SCANBUFSIZE){
					if(fwrite(wlcpBuf, sizeof(uint64_t), wlcpIdx, flcp) != wlcpIdx) {
						fprintf(stderr, "sam_sa_lcp: write file %s error!\n", lcpFile);
						exit(EXIT_FAILURE);
					}
					wlcpIdx = 0;
				}
				/****** should be careful about sample lcp *********************/
				prev_lcp_val = 0XFFFFFFFFFFFFFFFF;
				if(firstSamLcp == false)
					firstSamLcp = true;

			} else {
				if(firstSamLcp == true){
					prev_lcp_val = min(rlcpBuf[i], prev_lcp_val);
				}
			}
		}
	}

	if(wsaIdx != 0){
		if(fwrite(wsaBuf, sizeof(uint64_t), wsaIdx, fsa) != wsaIdx) {
			fprintf(stderr, "sam_sa_lcp: write file %s error!\n", saFile);
			exit(EXIT_FAILURE);
		}
		if(fwrite(wlcpBuf, sizeof(uint64_t), wlcpIdx, flcp) != wlcpIdx) {
			fprintf(stderr, "sam_sa_lcp: write file %s error!\n", lcpFile);
			exit(EXIT_FAILURE);
		}				
	}

	fclose(fsa64);
	fclose(flcp64);
	fclose(fsa);
	fclose(flcp);
	delete []rsaBuf;
	delete []wsaBuf;
	delete []rlcpBuf;
	delete []wlcpBuf;

	return 0;
}