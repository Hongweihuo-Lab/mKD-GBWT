CXX = g++
CFLAG = -O3

ALL: gen_sa_lcp sam_sa_lcp

gen_sa_lcp: gen_sa_lcp.cpp utils.cpp
	g++ -O3  gen_sa_lcp.cpp utils.cpp -o ../gen_sa_lcp

sam_sa_lcp: sam_sa_lcp.cpp
	g++ -O3 sam_sa_lcp.cpp -o ../sam_sa_lcp

clean:
	rm *.o ../gen_sa_lcp ../sam_sa_lcp