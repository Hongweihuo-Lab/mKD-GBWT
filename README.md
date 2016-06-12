# mKD-GBWT
mKD-GBWT(multiple KD-tree GBWT) is an external memory full-text index.

The basis of mKD-GBWT is Geometric Burrows-Wheeler Transform. The biggest improvement of mKD-GBWT is that it uses multiple KD-tree as its orthogonal range searching data structure, so it has a good I/O performance.

## Install
1. First you need install SAscan [1] and LCPscan [2].

    [1] https://www.cs.helsinki.fi/group/pads/SAscan.html
    
    [2] https://www.cs.helsinki.fi/group/pads/LCPscan.html

2. Download mKD-GBWT index from https://github.com/Hongweihuo-Lab/mKD-GBWT.

## Modify the location of index
2.1 Open file sbt_util.h, modify the `#define GBWT_INDEX_POSITION  "/media/软件/GBWT-Index-position/"` into location of yourself.

2.2 Open file tools/sam_sa_lcp.cpp, modify the `#define GBWT_INDEX_POSITION  "/media/软件/GBWT-Index-position/"` into location of yourself.

	$ cd mKD-GBWt
	$ make
	$ cd test
	$ make

## Generate index of data
1. 	$ cd SAscan-???
	$ cd src
	$ ./sascan <data>

2.	$ cd LCPscan-???
	$ cd build
	$ ./construct_lcp <data>

3.	$ cd mKD-GBWT
	$ ./gen_sa_lcp <data>

4.	$ cd mKD-GBWT
	$ ./sam_sa_lcp <data> <step>
	$ ./build	<data>	<disk-page-size-in-bytes> <step>

## Pattern-matching
	$ cd mKD-GBWT
	$ ./mygbwt <data> <disk-page-size-in-bytes> <step>

## Example
	the data file is `/data/english`, disk-page is 4096 bytes, step = 4

	First generate the index of data

		1. 	$ cd SAscan-???
			$ cd src
			$ ./sascan /data/english

		2.	$ cd LCPscan-???
			$ cd build
			$ ./construct_lcp /data/english

		3.	$ cd mKD-GBWT
			$ ./gen_sa_lcp /data/english

		4.	$ cd mKD-GBWT
			$ ./sam_sa_lcp /data/english 4
			$ ./build	/data/english	4096  	4

	Second, pattern mathcing
	
			$ cd mKD-GBWT
			$ ./mygbwt /data/english 4096 4

## Contributors
### Code
•	Yuhao Zhao

### Paper
mKD-GBWT is an implementation of the paper.

Hongwei Huo, Yuhao Zhao, et al., Efficient external memory compressed indexes, submitted.
