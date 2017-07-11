
CXXFLAGS:=-std=c++14 -O3 -fno-omit-frame-pointer -fsanitize=thread -stdlib=libc++
#CXXFLAGS:=-std=c++14 -O3  -stdlib=libc++

all: main main.ll

HEADER_FILES = $(wildcard ./*.hpp)

main: main.o
	$(CXX) $(CXXFLAGS) -o main main.o -lpthread

main.ll: main.cpp $(HEADER_FILES) 
	$(CXX) $(CXXFLAGS) -S -emit-llvm -c main.cpp

main.o: main.cpp $(HEADER_FILES) 
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

clean:
	rm main *.o *.ll
