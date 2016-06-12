CXX = g++ -O3
CC =  g++ -O3
 

FLAGS = -std=c++11 -c

ALL : BUILD MYGBWT
MAIN_BUILD := main_BUILD.o

MAIN_MYGBWT := main_MYGBWT.o

MAIN_AVE := main_AVE.o

OBJECTS := gbwt.o test_gbwt.o critbit_tree.o sbtmpfile.o  sb_tree.o test_critbit_tree.o  \
		DiskSBT.o DiskGBWT.o test_diskgbwt.o Suffix.o

OBJECTS_KDTREE := KDNode.o KDTree.o rect.o KDDisk.o KDPointDisk.o test_kdtree.o DiskFile.o KDTreeDiskHeader.o \
		DiskKDNode.o DiskKDTree.o	KDVirtualDisk.o point.o

BUILD: $(OBJECTS) $(OBJECTS_KDTREE)	$(MAIN_BUILD)
	$(CXX)  -std=c++11 -o build -pthread   $(OBJECTS) $(OBJECTS_KDTREE) $(MAIN_BUILD) libsdsl.a libdivsufsort64.a libdivsufsort.a

MYGBWT: $(OBJECTS) $(OBJECTS_KDTREE) $(MAIN_MYGBWT)
	$(CXX) -std=c++11 -o mygbwt -pthread   $(OBJECTS) $(OBJECTS_KDTREE)  $(MAIN_MYGBWT) libsdsl.a libdivsufsort64.a libdivsufsort.a

AVE: $(OBJECTS) $(OBJECTS_KDTREE) $(MAIN_AVE)
	$(CXX) -std=c++11 -o ave -pthread   $(OBJECTS) $(OBJECTS_KDTREE)  $(MAIN_AVE) libsdsl.a libdivsufsort64.a libdivsufsort.a

critbit_tree.o: critbit_tree.cpp  critbit_tree.h  sbt_util.h
	$(CXX) $(FLAGS) -o $@  $<

sbtmpfile.o: sbtmpfile.cpp sbtmpfile.h sbt_util.h
	$(CXX) $(FLAGS) -o $@ $<

sb_tree.o: sb_tree.cpp sb_tree.h sbt_util.h critbit_tree.h sbtmpfile.h point.h
	$(CXX) $(FLAGS) -o $@ $<

test_critbit_tree.o: test_critbit_tree.cpp test_critbit_tree.h sbt_util.h
	$(CXX) $(FLAGS) -o $@ $<

test_sbt.o: test_sbt.cpp test_sbt.h sb_tree.h sbt_util.h critbit_tree.h sbtmpfile.h
	$(CXX) $(FLAGS) -o $@ $<

DiskSBT.o: DiskSBT.cpp DiskSBT.h
	$(CXX) $(FLAGS) -o $@ $<

test_gbwt.o: test_gbwt.cpp test_gbwt.h
	$(CXX) $(FLAGS) -o $@ $<

gbwt.o: gbwt.cpp gbwt.h sbt_util.h point.h
	$(CXX) $(FLAGS) -o $@ $<

DiskGBWT.o:	DiskGBWT.cpp DiskGBWT.h sbt_util.h
	$(CXX) $(FLAGS) -o $@ $<

Suffix.o: Suffix.cpp Suffix.h
	$(CXX) $(FLAGS) -o $@ $<

test_diskgbwt.o: test_diskgbwt.cpp test_diskgbwt.h
	$(CXX) $(FLAGS) -o $@ $<	

$(MAIN_BUILD): main_BUILD.cpp critbit_tree.h sbt_util.h
	$(CXX) $(FLAGS) -o $@ $<

$(MAIN_MYGBWT): main_MYGBWT.cpp 
	$(CXX) $(FLAGS) -o $@ $< 			

$(MAIN_AVE): main_AVE.cpp
	$(CXX) $(FLAGS) -o $@ $<

#----------------------------KDTree-----------------------------

KDNode.o: KDNode.cpp KDNode.h rect.h point.h
	$(CXX) $(FLAGS) -o $@ $<

KDTree.o: KDTree.cpp KDNode.h rect.h point.h
	$(CXX) $(FLAGS) -o $@ $< 

rect.o: rect.cpp rect.h
	$(CXX) $(FLAGS) -o $@ $< 

KDDisk.o: KDDisk.cpp KDDisk.h rect.h DiskFile.h KDVirtualDisk.h
	$(CXX) $(FLAGS) -o $@ $<

KDPointDisk.o: KDPointDisk.cpp KDPointDisk.h rect.h DiskFile.h KDVirtualDisk.h
	$(CXX) $(FLAGS) -o $@ $<

DiskFile.o: DiskFile.cpp DiskFile.h
	$(CXX) $(FLAGS) -o $@ $<

KDTreeDiskHeader.o: KDTreeDiskHeader.cpp KDTreeDiskHeader.h DiskFile.h
	$(CXX) $(FLAGS) -o $@ $<

DiskKDNode.o: DiskKDNode.cpp DiskKDNode.h rect.h
	$(CXX) $(FLAGS) -o $@ $<

DiskKDTree.o: DiskKDTree.cpp DiskKDTree.h
	$(CXX) $(FLAGS) -o $@ $<

KDVirtualDisk.o: KDVirtualDisk.cpp KDVirtualDisk.h
	$(CXX) $(FLAGS) -o $@ $<

test_kdtree.o: test_kdtree.cpp KDTree.h test_kdtree.h
	$(CXX) $(FLAGS) -o $@ $<	

point.o: point.cpp point.h
	$(CXX) $(FLAGS) -o $@ $<	

clean:
	-rm $(OBJECTS) $(OBJECTS_KDTREE) $(MAIN_MYGBWT) $(MAIN_BUILD) build mygbwt