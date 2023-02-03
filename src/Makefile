
HEADER = BMgr.h DSMgr.h setting.h ReplaceAlgorithm.h
SRC = BMgr.cpp DSMgr.cpp Replace_LRU.cpp Replace_Clock.cpp Replace_Random.cpp
OBJ = BMgr.o DSMgr.o Replace_LRU.o Replace_Clock.o Replace_Random.o

libBMgr.a: $(OBJ)
	ar rcs $@ $(OBJ)

clean:
	rm libBMgr.a