
INCLUDE = -I../include
LIB = -L../src -lBMgr
CXXFLAGS = $(INCLUDE) $(LIB)

main.exe: libBMgr.a
	g++ main.cpp $(CXXFLAGS) -o $@

libBMgr.a:
	mingw32-make -C ../src/

clean:
	rm main.exe
	mingw32-make clean -C ../src/