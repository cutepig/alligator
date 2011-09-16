#include <iostream>
#include "allocator.h"

#include <list>
#include <vector>
#include <map>

#include <cstdlib>
#include <cstring>

template<typename T>
void print_container(const T &container) {
	for(typename T::const_iterator it = container.begin(); it != container.end(); it++ )
		std::cout << *it << " ";
}

int main()
{
	int i, j;	// predeclare some looping variables
	const int COUNT_I = 5, COUNT_J = 10;

	//===================================

	std::cout << "list block_allocator test" << std::endl;
	std::list<int, cutepig::block_allocator<int> > iblist;

	for(i=0; i<COUNT_I; i++) {
		std::cout << "inserting 10 values into list" << std::endl;
		for(j=0; j<COUNT_J; j++) {
			iblist.push_back(i*COUNT_J+j);
		}
	}
	for(i=0; i<COUNT_I; i++) {
		std::cout << "removing 10 values from list" << std::endl;
		for(j=0; j<COUNT_J; j++) {
			// little confusing trick
			if(j&1)
				iblist.pop_front();
			else
				iblist.pop_back();
			print_container(iblist);
			std::cout << std::endl;
		}
	}

	//===================================

	// on gcc, list allocates elements at a time
	std::cout << "list test" << std::endl;

	std::list<int, cutepig::malloc_allocator<int> > ilist;
	for(i=0; i<COUNT_I; i++) {
		std::cout << "inserting 10 values into list" << std::endl;
		for(j=0; j<COUNT_J; j++) {
			ilist.push_back(i);
		}
	}
	for(i=0; i<COUNT_I; i++) {
		std::cout << "removing 10 values from list" << std::endl;
		for(j=0; j<COUNT_J; j++) {
			// little confusing trick
			if(i&1)
				ilist.pop_front();
			else
				ilist.pop_back();
		}
	}

	//====================================

	// test what happens when we "copy" a sequence of elements
	std::cout << "list test 2" << std::endl;

	for(i=0; i<COUNT_I; i++) {
    	std::cout << "inserting 10 values into list" << std::endl;
    	for(j=0; j<COUNT_J; j++) {
			ilist.push_back(i);
    	}
    }

	std::list<int, cutepig::malloc_allocator<int> > ilist2;

	std::cout << "list::assign( begin, end)" << std::endl;
	// on gcc, this copies elements 1 by 1
	ilist2.assign( ilist.begin(), ilist.end() );

	std::cout << "list copy ctor" << std::endl;
	// on gcc, this copies elements 1 by 1
	std::list<int, cutepig::malloc_allocator<int> > ilist3( ilist2 );

	return 0;

    //====================================

	// on gcc, vector allocation policy seems to be N rounded to next 2^x
    std::cout << "vector test" << std::endl;

    std::vector<float, cutepig::malloc_allocator<float> > ivector;
    for(i=0; i<COUNT_I; i++) {
    	std::cout << "inserting 10 values into vector" << std::endl;
    	for(j=0; j<COUNT_J; j++) {
			ivector.push_back(i);
    	}
    }

    for(i=0; i<COUNT_I; i++) {
    	std::cout << "removing 10 values from vector" << std::endl;
    	for(j=0; j<COUNT_J; j++) {
    		// HMM, random remove?
			ivector.pop_back();
    	}
    }

    //====================================
#if 0
    // vector test 2, initialize with many values
    std::cout << "vector test 2" << std::endl;

	const int LARGE_SIZE = 100000000;
	const int LARGE_CHUNKS = 16;
    std::vector<int, cutepig::malloc_allocator<int> > ivector2( LARGE_SIZE, 12345678 );
    for(i=0; i < LARGE_CHUNKS; i++) {
    	std::size_t offset = (((LARGE_CHUNKS - 1) - i ) * (LARGE_SIZE / LARGE_CHUNKS));
    	std::cout << "removing from vector range " << std::dec << offset << " " << ivector2.size() << std::endl;

    	// on gcc, this doesnt free any memory
		// ivector2.erase( ivector2.begin() + offset, ivector2.end());

		// on gcc, this doesnt free any memory
		ivector2.resize( offset );
    }
    // on gcc, even this doesnt free memory!
    ivector2.resize(0);
#endif
    //====================================

    // lets see what map does for allocation
    // on gcc, map allocates elements at a time
    std::cout << "map test" << std::endl;
    std::map< int, int, std::less<int>, cutepig::malloc_allocator<std::pair<int, int> > > imap;

	for(i = 0; i < COUNT_I; i++ ) {
		std::cout << "inserting 10 values to map" << std::endl;
		for(j = 0; j < COUNT_J; j++ ) {
			imap[ rand() & 0xffff ] = rand();
		}
	}

	//====================================

	// gcc string seems to keep minimum 30 elements and then allocates more when needed
	// doesnt seem to release memory when shortened (or even cleared)
	// basically similar allocation strategy than with vector

	// string test.. override stl string with this
	std::cout << "string test" << std::endl;
	typedef std::basic_string<char, std::char_traits<char>, cutepig::malloc_allocator<char> > string;
	string s;

	// just some random stuff
	std::cout << "appending.. " << s << " " << s.size() << std::endl;
	s += "jees jees dfadfadf khhk";
	std::cout << "appending.. " << s << " " << s.size() << std::endl;
	s += "kdhafhadfhkadfh lkjkljdf";
	std::cout << "replacing.. " << s << " " << s.size() << std::endl;
	s = "dafljdhfhfd";
	std::cout << "appending.. " << s << " " << s.size() << std::endl;
	s += "jkldfljdflkjdafadfadfadfadffg f gfg fasgsfgf fg";
	std::cout << "appending.. " << s << " " << s.size() << std::endl;
	s += "adfkjdfllkjkjdfdfdff gsfg sfg sfg sfgfsgsfgfsgsfg fg fgfgfg";
	std::cout << "erasing.. " << s << " " << s.size() << std::endl;
	s.erase( s.size() / 2 );
	std::cout << "erasing.. " << s << " " << s.size() << std::endl;
	s.erase( s.size() / 2 );
	std::cout << "erasing.. " << s << " " << s.size() << std::endl;
	s.erase( s.size() / 2 );
	std::cout << "erasing.. " << s << " " << s.size() << std::endl;
	s.erase( s.size() / 2 );
	std::cout << "erasing.. " << s << " " << s.size() << std::endl;
	s.erase( s.size() / 2 );
	std::cout << "clearing out.. " << s << " " << s.size() << std::endl;
	s.clear();
	std::cout << "ok..?" << std::endl;
	s = "dippidappa";
	std::cout << "ok.." << std::endl;
}
