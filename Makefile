CXXFLAGS = 
CXX = g++
VPATH = src:build

ifdef OS
# if we're on Windows :
build/choose.exe : choose.o
else
# if NOT on windows :
build/choose : choose.o
endif
	$(CXX) $(CXXFLAGS) $? -o $@

build/choose.o : choose.cpp
	mkdir build
	$(CXX) $(CXXFLAGS) -c $? -o $@

.PHONY = clean
clean :
	rm -rvf build

