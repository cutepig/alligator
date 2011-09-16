/*
allocator.h - cutepig stl allocators
Copyright (C) 2011  Christian Holmberg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef ALLOCATOR_H_INCLUDED
#define ALLOCATOR_H_INCLUDED

#define __REPORT_ALLOCS__

#include <utility>	// pair
#include <algorithm>	// max
#include <new>	// bad_alloc
#include <stdlib.h>	// malloc/free

#ifdef __REPORT_ALLOCS__
	#include <iostream>
	#include <cmath>
#endif

namespace cutepig {

	// helper to get either 32 or 64 depending on the platform
	// determined by length of void* pointer
	template<int __ptr_size> struct __int_32_64 {};
	template<> struct __int_32_64<4> { typedef __uint32_t type; };
	template<> struct __int_32_64<8> { typedef __uint64_t type; };
	struct int_32_64 { typedef typename __int_32_64<sizeof(int*)>::type type; };

	//=============================================

	// for copy/paste reference purposes only. unusable

	// allocator class
	template <class T> class __base_allocator;
	// specialize for void:
	template <> class __base_allocator<void> {
	public:
		typedef void*       pointer;
		typedef const void* const_pointer;
		//  reference-to-void members are impossible.
		typedef void  value_type;
		template <class U> struct rebind { typedef __base_allocator<U> other; };
	};

	template <class T> class __base_allocator {
	public:
		typedef size_t    size_type;
		typedef std::ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;
		template <class U> struct rebind { typedef __base_allocator<U> other; };

		__base_allocator() throw() {}
		__base_allocator(const __base_allocator&) throw() {}
		template <class U> __base_allocator(const __base_allocator<U>&) throw() {}

		~__base_allocator() throw() {};

		pointer address(reference x) const
		{ return &x; }
		const_pointer address(const_reference x) const
		{ return &x; }

		pointer allocate(size_type, typename __base_allocator<void>::const_pointer hint = 0)
		{ return 0; };
		void deallocate(pointer p, size_type n)
		{}

		size_type max_size() const throw()
		{ return 0x8000000; }

		void construct(pointer p, const T& val)
		{ new(p) T(val); }
		void destroy(pointer p)
		{ p->~T(); }

		// to copy ctor or compare with private data, do this
		template<typename U>
		friend class __base_allocator;
	};

	//===================================================

	// so lets implement some allocator

	// allocator class
	template <class T> class malloc_allocator;
	// specialize for void:
	template <> class malloc_allocator<void> {
	public:
		typedef void*       pointer;
		typedef const void* const_pointer;
		//  reference-to-void members are impossible.
		typedef void  value_type;
		template <class U> struct rebind { typedef malloc_allocator<U> other; };
	};

	template<typename T>
	class malloc_allocator {
	public:
		typedef size_t    size_type;
		typedef std::ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;
		template <class U> struct rebind { typedef malloc_allocator<U> other; };

		malloc_allocator() throw() {}
		malloc_allocator(const malloc_allocator &/*other*/) throw() {}
		template<typename U> malloc_allocator(const malloc_allocator<U>&) throw() {}

		~malloc_allocator() throw() {}

		// nothing to see here
		pointer address(reference x) const
		{ return &x; }
		const_pointer address(const_reference x) const
		{ return &x; }
		size_type max_size() const throw()
		{ return 0x8000000; }

		void construct(pointer p, const T& val)
		{ new(p) T(val); }
		void destroy(pointer p)
		{ p->~T(); }

		// to copy ctor or compare with private data, do this
		template<typename U>
		friend class __base_allocator;

		//=======================================

		// the "special" kind

		// allocation function, just use malloc here
		pointer allocate(size_type n, typename malloc_allocator<void>::const_pointer hint=0)
		{
			pointer p = static_cast<pointer>( malloc(n * sizeof(T)) );
			if(!p)
				throw std::bad_alloc();
			#ifdef __REPORT_ALLOCS__
				std::cout << "malloc_allocator::allocate: " << n << " * " << sizeof(T)
					<< ", hint: 0x" << std::hex << hint
					<< " returning: 0x" << static_cast<malloc_allocator<void>::const_pointer>(p) << std::endl;
			#endif

			return p;
		}

		// deallocation, use free()
		void deallocate(pointer p, size_type n)
		{
			#ifdef __REPORT_ALLOCS__
				std::cout << "malloc_allocator::deallocate: " << std::hex << static_cast<malloc_allocator<void>::const_pointer>(p)
						<< ", size " << std::dec << n << " * " << sizeof(T) << std::endl;
			#endif

			if(p)
				free(p);
		}
	};

	// then ofc these
	template<typename T1, typename T2>
	bool operator==(const malloc_allocator<T1> &a, const malloc_allocator<T2> &b) throw()
	{ return true; }

	template<typename T1, typename T2>
	bool operator!=(const malloc_allocator<T1> &a, const malloc_allocator<T2> &b) throw()
	{ return false; }

	//===================================================

	// another more advanced allocator, suitable for lists and maps
	// that has the allocation policy of one at a time
	// (for platforms that actually do that for these containers!)

	// this is basically a bitmap allocator, just renamed to block_allocator
	// because it uses list of blocks where each block has N slots

	// block-size policy, 32-bit and smaller objects have block-size of 32
	// larger objects have 8 (optimize further? utilize 64-bits on 64-bit archs?)
	template <typename T, int number_of_slots=(sizeof(T) > 4 ? 8 : 32)>
	class block_allocator;

	// specialize for void:
	template <int number_of_slots> class block_allocator<void,number_of_slots> {
	public:
		typedef void*       pointer;
		typedef const void* const_pointer;
		//  reference-to-void members are impossible.
		typedef void  value_type;
		template <class U> struct rebind { typedef block_allocator<U> other; };
	};

	template <class T, int _number_of_slots> class block_allocator {
	public:
		typedef size_t    size_type;
		typedef std::ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;

		template <class U>
		struct rebind { typedef block_allocator<U> other; };

		//===========================

		/*
			This allocator keeps a shared list of blocks to allocate from.
			Each block has number of slots to allocate from. Amount of slots is static,
			while each block is dynamically allocated/free'd as necessary.
		*/

		// invidual block
		struct block_block {
			typedef int_32_64::type bitmask_t;
			block_block *prev;
			block_block *next;
			bitmask_t slots;
			T ptr[_number_of_slots];
			static const bitmask_t fullmask = (1<<_number_of_slots) - 1;

			// very simple constructor
			block_block() {
				prev = next = 0;
				slots = 0;
			}

			// returns true if this block has slots available
			bool hasroom() { return (slots & fullmask) != fullmask; }
			// returns true if block is all empty
			bool isempty() { return (slots == 0); }

			// allocate a slot (excepts that the block has been checked with hasroom)
			pointer allocate() {
				// unoptimized scan of non-free elements
				for(int i = 0; i < _number_of_slots; i++) {
					if( !(slots & (1<<i)) ) {
						// mark allocated
						slots |= (1<<i);
						#ifdef __REPORT_ALLOCS__
							std::cout << "block_block::allocate found slot " << i << " @ " << (void*)(&ptr[i]) << std::endl << std::flush;
						#endif
						return &ptr[i];
					}
				}
				#ifdef __REPORT_ALLOCS__
					std::cout << "block_block::allocate couldnt find slot " << std::endl;
				#endif
				return 0;
			}

			// deallocate a slot
			void deallocate(pointer p) {
				bitmask_t bit = (1 << (bitmask_t)(p - ptr));
				#ifdef __REPORT_ALLOCS__
					std::cout << "block_block::deallocate freeing slot " << int(log2(float(bit))) << " @ " << (void*)(&ptr[bit]) << std::endl << std::flush;
				#endif
				slots &= ~bit;
			}

			// tell me if given pointer is inside this block
			bool inblock(pointer p) {
				return (p >= ptr && p < &(ptr[_number_of_slots]));
			}
		};

		// container of blocks (refcounted so that you can share this)
		struct block_list {
			int_32_64::type refcount;		// keep same size as pointers for padding
			block_block block;		// we always keep this one block here
			block_block *head;
			block_block *tail;

			block_list()
				: refcount(1), block(), head(&block), tail(&block)
			{}

			// internal system allocation functions
			void *_alloc( size_t s ) {
				return malloc(s);
			}
			void _free( void *p ) {
				free( p );
			}

			/*
				so, algorithm goes like this:
				if 'head' has slots available, call
					r = head->allocate();
					if 'head' has no slots available and 'head' != 'tail'
						move 'head' to 'tail'
					(alt. approach is to 'float' 'head' towards 'tail')

				else
					allocate new 'head'
					return head->allocate()		(this surely will have slots avail)
			*/
			pointer allocate() {
				// blocks with free space are always in the beginning
				if(head->hasroom()) {
					pointer r = head->allocate();

					// if this block doesnt have anymore space, move it to tail
					// (alt. float up)
					if(!head->hasroom() && head != tail) {
						tail->next = head;
						head->prev = tail;
						tail = head;
						head = head->next;
						// tail and head are now updated
						head->prev = tail->next = 0;
					}

					return r;
				}
				else {
					// allocate new head with space
					block_block *block = new(_alloc(sizeof(*block))) block_block();
					#ifdef __REPORT_ALLOCS__
						std::cout << "block_list::allocate allocated new block @ " << (void*)block << std::endl << std::flush;
					#endif
					block->next = head;
					head->prev = block;
					head = block;
					return head->allocate();
				}

				return 0;
			}

			/*
				algo to deallocate goes like this:
				find the block that contains p
				b->deallocate()
				if 'b' is empty and 'b' is not 'block',
					remove it from the list and 'delete'
				otherwise move to 'head'
				(alt. approach is to 'float' 'b' towards 'head')
			*/
			void deallocate(pointer p) {
				bool wasfull;
				block_block *iter;
				for(iter = head; iter != 0; iter = iter->next) {
					if(iter->inblock(p))
						break;
				}
				// exceptionally throw from here (other functions just return 0)
				if(!iter)
					throw std::bad_alloc();

				wasfull = !iter->hasroom();
				iter->deallocate(p);
				// if we just emptied this block, we can deallocate it
				if(iter->isempty() && iter != &block) {
					// detach (TODO: function)
					if(iter->prev)
						iter->prev->next = iter->next;
					if(iter->next)
						iter->next->prev = iter->prev;
					if(iter == head)
						head = iter->next;
					if(iter == tail)
						tail = iter->prev;

					_free( iter );
				}
				else if(wasfull && iter != head) {
					// move block back to head (alt. float downwards)
					if(iter->prev)
						iter->prev->next = iter->next;
					if(iter->next)
						iter->next->prev = iter->prev;
					iter->next = head;
					iter->prev = 0;
					head->prev = iter;
					head = iter;
				}
			}
		};

		//===========================


		block_allocator() throw() {
			blocks = &blocks_static;
		}
		block_allocator(const block_allocator &other) throw() {
			blocks = other.blocks;
			blocks->refcount++;
		}
		template <class U, int __number_of_slots>
		block_allocator(const block_allocator<U, __number_of_slots> &other) throw() {
			/*
				we can only use the blocks from the other allocator if its the
				same templation type, any wa
			*/
			blocks = &blocks_static;
		}

		~block_allocator() throw() {
			blocks->refcount--;
			/*
			if(blocks->refcount <= 0) {
				blocks->~block_list();
				_free( blocks );
			}
			*/
		};

		pointer address(reference x) const
		{ return &x; }
		const_pointer address(const_reference x) const
		{ return &x; }

		size_type max_size() const throw()
		{ return 0x1; }

		void construct(pointer p, const T& val)
		{ new(p) T(val); }
		void destroy(pointer p)
		{ p->~T(); }

		pointer allocate(size_type, typename block_allocator<void, _number_of_slots>::const_pointer hint = 0) {
			pointer r = blocks->allocate();
			if(!r)
				throw std::bad_alloc();
			return r;
		}
		void deallocate(pointer p, size_type n) {
			if(p)
				blocks->deallocate(p);
		}

	private:
		friend class block_block;
		friend class block_list;

		// to copy ctor or compare with private data, do this
		template<typename U, int __number_of_slots>
		friend class block_allocator;

		block_list *blocks;
		// static instantiation of blocks of same type
		// TODO: find a way to instantiate blocks of same size
		static block_list blocks_static;

		static const int number_of_slots = _number_of_slots;
	};

	// static instantiation of blocks of same type
	template<typename T, int _number_of_slots>
	typename block_allocator<T, _number_of_slots>::block_list
		block_allocator<T, _number_of_slots>::blocks_static;

	// then ofc these
	template<typename T1, typename T2>
	bool operator==(const block_allocator<T1> &a, const block_allocator<T2> &b) throw()
	{ return a.blocks == b.blocks; }

	template<typename T1, typename T2>
	bool operator!=(const block_allocator<T1> &a, const block_allocator<T2> &b) throw()
	{ return a.blocks != b.blocks; }
}

#endif // ALLOCATOR_H_INCLUDED
