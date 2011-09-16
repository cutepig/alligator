all: allocator_test

allocator_test: allocator_test.o
	g++ allocator_test.o -o allocator_test

allocator_test.o: allocator_test.cpp
#	g++ -c allocator_test.cpp
