/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */
 
#ifndef __BX_ALLOCATOR_H__
#define __BX_ALLOCATOR_H__

#include "bx.h"

#include <memory.h>
#include <new>

#if BX_CONFIG_ALLOCATOR_CRT
#	include <malloc.h>
#endif // BX_CONFIG_ALLOCATOR_CRT

#if BX_CONFIG_ALLOCATOR_DEBUG
#	define BX_ALLOC(_allocator, _size)                         bx::alloc(_allocator, _size, __FILE__, __LINE__)
#	define BX_REALLOC(_allocator, _ptr, _size)                 bx::realloc(_allocator, _ptr, _size, __FILE__, __LINE__)
#	define BX_FREE(_allocator, _ptr)                           bx::free(_allocator, _ptr, __FILE__, __LINE__)
#	define BX_ALIGNED_ALLOC(_allocator, _size, _align)         bx::alignedAlloc(_allocator, _size, _align, __FILE__, __LINE__)
#	define BX_ALIGNED_REALLOC(_allocator, _ptr, _size, _align) bx::alignedRealloc(_allocator, _ptr, _size, _align, __FILE__, __LINE__)
#	define BX_ALIGNED_FREE(_allocator, _ptr)                   bx::alignedFree(_allocator, _ptr, __FILE__, __LINE__)
#	define BX_NEW(_allocator, _type)                           new(BX_ALLOC(_allocator, sizeof(_type) ) ) _type
#	define BX_DELETE(_allocator, _ptr)                         bx::deleteObject(_allocator, _ptr, __FILE__, __LINE__)
#else
#	define BX_ALLOC(_allocator, _size)                         bx::alloc(_allocator, _size)
#	define BX_REALLOC(_allocator, _ptr, _size)                 bx::realloc(_allocator, _ptr, _size)
#	define BX_FREE(_allocator, _ptr)                           bx::free(_allocator, _ptr)
#	define BX_ALIGNED_ALLOC(_allocator, _size, _align)         bx::alignedAlloc(_allocator, _size, _align)
#	define BX_ALIGNED_REALLOC(_allocator, _ptr, _size, _align) bx::alignedRealloc(_allocator, _ptr, _size, _align)
#	define BX_ALIGNED_FREE(_allocator, _ptr)                   bx::alignedFree(_allocator, _ptr)
#	define BX_NEW(_allocator, _type)                           ::new(BX_ALLOC(_allocator, sizeof(_type) ) ) _type
#	define BX_DELETE(_allocator, _ptr)                         bx::deleteObject(_allocator, _ptr)
#endif // BX_CONFIG_DEBUG_ALLOC

#ifndef BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT
#	define BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT 8
#endif // BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT

namespace bx
{
	/// Aligns pointer to nearest next aligned address. _align must be power of two.
	inline void* alignPtr(void* _ptr, size_t _extra, size_t _align = BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT)
	{
		union { void* ptr; size_t addr; } un;
		un.ptr = _ptr;
		size_t unaligned = un.addr + _extra; // space for header
		size_t mask = _align-1;
		size_t aligned = BX_ALIGN_MASK(unaligned, mask);
		un.addr = aligned;
		return un.ptr;
	}

	/// Check if pointer is aligned. _align must be power of two.
	inline bool isPtrAligned(const void* _ptr, size_t _align = BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT)
	{
		union { const void* ptr; size_t addr; } un;
		un.ptr = _ptr;
		return 0 == (un.addr & (_align-1) );
	}

	struct BX_NO_VTABLE AllocatorI
	{
		virtual ~AllocatorI() = 0;
		virtual void* alloc(size_t _size, const char* _file, uint32_t _line) = 0;
		virtual void free(void* _ptr, const char* _file, uint32_t _line) = 0;
	};

	inline AllocatorI::~AllocatorI()
	{
	}

	struct BX_NO_VTABLE ReallocatorI : public AllocatorI
	{
		virtual void* realloc(void* _ptr, size_t _size, const char* _file, uint32_t _line) = 0;
	};

	struct BX_NO_VTABLE AlignedAllocatorI
	{
		virtual void* alignedAlloc(size_t _size, size_t _align, const char* _file, uint32_t _line) = 0;
		virtual void alignedFree(void* _ptr, const char* _file, uint32_t _line) = 0;
	};

	struct BX_NO_VTABLE AlignedReallocatorI : public AlignedAllocatorI
	{
		virtual void* alignedRealloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) = 0;
	};

	inline void* alloc(AllocatorI* _allocator, size_t _size, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->alloc(_size, _file, _line);
	}

	inline void free(AllocatorI* _allocator, void* _ptr, const char* _file = NULL, uint32_t _line = 0)
	{
		_allocator->free(_ptr, _file, _line);
	}

	inline void* alloc(AlignedAllocatorI* _allocator, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->alignedAlloc(_size, _align, _file, _line);
	}

	inline void free(AlignedAllocatorI* _allocator, void* _ptr, const char* _file = NULL, uint32_t _line = 0)
	{
		_allocator->alignedFree(_ptr, _file, _line);
	}

	inline void* realloc(ReallocatorI* _allocator, void* _ptr, size_t _size, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->realloc(_ptr, _size, _file, _line);
	}

	inline void* realloc(AlignedReallocatorI* _allocator, void* _ptr, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->alignedRealloc(_ptr, _size, _align, _file, _line);
	}

	static inline void* alignedAlloc(AllocatorI* _allocator, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		size_t total = _size + _align;
		uint8_t* ptr = (uint8_t*)alloc(_allocator, total, _file, _line);
		uint8_t* aligned = (uint8_t*)alignPtr(ptr, sizeof(uint32_t), _align);
		uint32_t* header = (uint32_t*)aligned - 1;
		*header = uint32_t(aligned - ptr);
		return aligned;
	}

	static inline void alignedFree(AllocatorI* _allocator, void* _ptr, const char* _file = NULL, uint32_t _line = 0)
	{
		uint8_t* aligned = (uint8_t*)_ptr;
		uint32_t* header = (uint32_t*)aligned - 1;
		uint8_t* ptr = aligned - *header;
		free(_allocator, ptr, _file, _line);
	}

	static inline void* alignedRealloc(ReallocatorI* _allocator, void* _ptr, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		if (NULL == _ptr)
		{
			return alignedAlloc(_allocator, _size, _align, _file, _line);
		}

		uint8_t* aligned = (uint8_t*)_ptr;
		uint32_t offset = *( (uint32_t*)aligned - 1);
		uint8_t* ptr = aligned - offset;
		size_t total = _size + _align;
		ptr = (uint8_t*)realloc(_allocator, ptr, total, _file, _line);
		uint8_t* newAligned = (uint8_t*)alignPtr(ptr, sizeof(uint32_t), _align);

		if (newAligned == aligned)
		{
			return aligned;
		}

		aligned = ptr + offset;
		::memmove(newAligned, aligned, _size);
		uint32_t* header = (uint32_t*)newAligned - 1;
		*header = uint32_t(newAligned - ptr);
		return newAligned;
	}

	inline void* alignedAlloc(AlignedAllocatorI* _allocator, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->alignedAlloc(_size, _align, _file, _line);
	}

	inline void alignedFree(AlignedAllocatorI* _allocator, void* _ptr, const char* _file = NULL, uint32_t _line = 0)
	{
		_allocator->alignedFree(_ptr, _file, _line);
	}

	inline void* alignedRealloc(AlignedReallocatorI* _allocator, void* _ptr, size_t _size, size_t _align, const char* _file = NULL, uint32_t _line = 0)
	{
		return _allocator->alignedRealloc(_ptr, _size, _align, _file, _line);
	}

	template <typename ObjectT>
	inline void deleteObject(AllocatorI* _allocator, ObjectT* _object, const char* _file = NULL, uint32_t _line = 0)
	{
		if (NULL != _object)
		{
			_object->~ObjectT();
			free(_allocator, _object, _file, _line);
		}
	}

#if BX_CONFIG_ALLOCATOR_CRT
	class CrtAllocator : public ReallocatorI, public AlignedReallocatorI
	{
	public:
		CrtAllocator()
		{
		}

		virtual ~CrtAllocator()
		{
		}

		virtual void* alloc(size_t _size, const char* _file, uint32_t _line) BX_OVERRIDE
		{
			BX_UNUSED(_file, _line);
			return ::malloc(_size);
		}

		virtual void free(void* _ptr, const char* _file, uint32_t _line) BX_OVERRIDE
		{
			BX_UNUSED(_file, _line);
			::free(_ptr);
		}

		virtual void* realloc(void* _ptr, size_t _size, const char* _file, uint32_t _line) BX_OVERRIDE
		{
			BX_UNUSED(_file, _line);
			return ::realloc(_ptr, _size);
		}

		virtual void* alignedAlloc(size_t _size, size_t _align, const char* _file, uint32_t _line) BX_OVERRIDE
		{
#	if BX_COMPILER_MSVC
			BX_UNUSED(_file, _line);
			return _aligned_malloc(_size, _align);
#	else
			return bx::alignedAlloc(static_cast<AlignedReallocatorI*>(this), _size, _align, _file, _line);
#	endif // BX_
		}

		virtual void alignedFree(void* _ptr, const char* _file, uint32_t _line) BX_OVERRIDE
		{
#	if BX_COMPILER_MSVC
			BX_UNUSED(_file, _line);
			return _aligned_free(_ptr);
#	else
			return bx::alignedFree(static_cast<AlignedReallocatorI*>(this), _ptr, _file, _line);
#	endif // BX_
		}

		virtual void* alignedRealloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) BX_OVERRIDE
		{
#	if BX_COMPILER_MSVC
			BX_UNUSED(_file, _line);
			return _aligned_realloc(_ptr, _size, _align);
#	else
			return bx::alignedRealloc(static_cast<AlignedReallocatorI*>(this), _ptr, _size, _align, _file, _line);
#	endif // BX_
		}
	};
#endif // BX_CONFIG_ALLOCATOR_CRT

} // namespace bx

#endif // __BX_ALLOCATOR_H__
