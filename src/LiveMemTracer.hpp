/*
LiveMemTracer
Code and documentation https://github.com/cesarl/LiveMemTracer
*/

#pragma once


#if defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__)
#define LMT_PLATFORM_WINDOWS
#elif defined(__clang__)
#define LMT_PLATFORM_ORBIS
#endif

#if LMT_ENABLED == 0
#define LMT_ALLOC(size)::malloc(size)
#define LMT_ALLOC_ALIGNED(size, alignment)::malloc(size)
#define LMT_DEALLOC(ptr)::free(ptr)
#define LMT_DEALLOC_ALIGNED(ptr)::free(ptr)
#define LMT_REALLOC(ptr, size)::realloc(ptr, size)
#define LMT_REALLOC_ALIGNED(ptr, size, alignment)::realloc(ptr, size, alignment)
#define LMT_DISPLAY(dt)do{}while(0)
#define LMT_EXIT()do{}while(0)
#define LMT_INIT()do{}while(0)
#define LMT_FLUSH()do{}while(0)

#else //LMT_ENABLED

#define LMT_ALLOC(size)::LiveMemTracer::alloc(size)
#define LMT_ALLOC_ALIGNED(size, alignment)::LiveMemTracer::allocAligned(size, alignment)
#define LMT_DEALLOC(ptr)::LiveMemTracer::dealloc(ptr)
#define LMT_DEALLOC_ALIGNED(ptr)::LiveMemTracer::deallocAligned(ptr)
#define LMT_REALLOC(ptr, size)::LiveMemTracer::realloc(ptr, size)
#define LMT_REALLOC_ALIGNED(ptr, size, alignment)::LiveMemTracer::reallocAligned(ptr, size, alignment)
#define LMT_DISPLAY(dt)::LiveMemTracer::display(dt)
#define LMT_EXIT()::LiveMemTracer::exit()
#define LMT_INIT() ::LiveMemTracer::init()
#define LMT_FLUSH()::LiveMemTracer::getChunk(true)

#ifdef LMT_IMPL

#include <atomic>     //std::atomic
#include <cstdlib>    //malloc etc...
#include <algorithm>
#ifdef LMT_PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable:4265)
#endif
#include <mutex>
#ifdef LMT_PLATFORM_WINDOWS
#pragma warning(pop)
#endif

#ifndef LMT_ALLOC_NUMBER_PER_CHUNK
#define LMT_ALLOC_NUMBER_PER_CHUNK 1024 * 8
#endif

#ifndef LMT_STACK_SIZE_PER_ALLOC
#define LMT_STACK_SIZE_PER_ALLOC 50
#endif

#ifndef LMT_CHUNK_NUMBER_PER_THREAD
#define LMT_CHUNK_NUMBER_PER_THREAD 8
#endif

#ifndef LMT_CACHE_SIZE
#define LMT_CACHE_SIZE 16
#endif

#ifndef LMT_ALLOC_DICTIONARY_SIZE
#define LMT_ALLOC_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef LMT_STACK_DICTIONARY_SIZE
#define LMT_STACK_DICTIONARY_SIZE 1024 * 16
#endif

#ifndef LMT_TREE_DICTIONARY_SIZE
#define LMT_TREE_DICTIONARY_SIZE 1024 * 16 * 16
#endif

#ifndef LMT_IMGUI
#define LMT_IMGUI 1
#endif

#ifndef LMT_ASSERT
#define LMT_ASSERT(condition, message, ...) assert(condition)
#endif

#ifndef LMT_TREAT_CHUNK
#define LMT_TREAT_CHUNK(chunk) LiveMemTracer::treatChunk(chunk)
#endif

#ifndef LMT_DEBUG_DEV
#define LMT_DEBUG_ASSERT(condition, message, ...) do{}while(0)
#else
#define LMT_DEBUG_ASSERT(condition, message, ...) LMT_ASSERT(condition, message, ##__VA_ARGS__)
#endif

#ifndef LMT_IMPLEMENTED
#define LMT_IMPLEMENTED 1
#else
static_assert(false, "LMT is already implemented, do not define LMT_IMPL more than once.");
#endif

#if !defined(LMT_PLATFORM_WINDOWS) && !defined(LMT_PLATFORM_ORBIS)
static_assert(false, "You have to define platform. Only Orbis and Windows are supported for now.");
#endif

#if !defined(LMT_PLATFORM_WINDOWS) && !defined(LMT_PLATFORM_ORBIS)
static_assert(false, "You have to define platform. Only Orbis and Windows are supported for now.");
#endif

#if (!defined(LMT_x64) && !defined(LMT_x86)) || (defined(LMT_x64) && defined(LMT_x86))
static_assert(false, "You have to define if target is x64 or x86");
#endif

#endif

#if defined(LMT_PLATFORM_WINDOWS) 
#undef LMT_TLS
#undef LMT_INLINE
#define LMT_TLS __declspec(thread)
#define LMT_INLINE __forceinline
#define LMT_ATOMIC_INITIALIZER(value) value
#elif defined(LMT_PLATFORM_ORBIS)
#undef LMT_TLS
#undef LMT_INLINE
#define LMT_TLS __thread
#define LMT_INLINE __attribute__((__always_inline__))
#define LMT_ATOMIC_INITIALIZER(value) {value}
#else
#define LMT_TLS
#define LMT_INLINE
#define LMT_ATOMIC_INITIALIZER(value)()
#endif

#include <stdint.h>

namespace LiveMemTracer
{
	typedef uint32_t Hash;

	LMT_INLINE void *alloc(size_t size);
	LMT_INLINE void *allocAligned(size_t size, size_t alignment);
	LMT_INLINE void dealloc(void *ptr);
	LMT_INLINE void deallocAligned(void *ptr);
	LMT_INLINE void *realloc(void *ptr, size_t size);
	LMT_INLINE void *reallocAligned(void *ptr, size_t size, size_t alignment);
	void exit();
	void init();
	void display(float dt);
	struct Chunk;
	LMT_INLINE Chunk *getChunk(bool forceFlush = false);

	namespace SymbolGetter
	{
		void init();
	}
#ifdef LMT_IMPL
	static const size_t INTERNAL_MAX_STACK_DEPTH = 255;
	static const size_t INTERNAL_FRAME_TO_SKIP = 2;
	static const char  *TRUNCATED_STACK_NAME = "Truncated\0";
	static const char  *UNKNOWN_STACK_NAME = "Unknown\0";
	static const size_t HISTORY_FRAME_NUMBER = 120;
	template <class T> LMT_INLINE Hash combineHash(const T& val, const Hash baseHash = 2166136261U);
	static uint32_t getCallstack(uint32_t maxStackSize, void **stack, Hash *hash);
#endif
}

#if defined(LMT_PLATFORM_WINDOWS)
#include "LiveMemTracer_Windows.hpp"
#elif defined(LMT_PLATFORM_ORBIS)
#include "LiveMemTracer_Orbis.hpp"
#endif

#ifdef LMT_IMPL
namespace LiveMemTracer
{
#define LMT_IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define LMT_TO_UPPER(c) ((c) & 0xDF)

	LMT_INLINE char *LMT_STRSTRI(const char * str1, const char * str2)
	{
		char *copy = (char *)str1;
		char *s1, *s2;
		if (!*str2)
			return (char *)str1;

		while (*copy)
		{
			s1 = copy;
			s2 = (char *)str2;

			while (*s1 && *s2 && (LMT_IS_ALPHA(*s1) && LMT_IS_ALPHA(*s2)) ? !(LMT_TO_UPPER(*s1) - LMT_TO_UPPER(*s2)) : !(*s1 - *s2))
				++s1, ++s2;

			if (!*s2)
				return copy;

			++copy;
		}
		return nullptr;
	}

	template <typename T>
	class LMTVector
	{
	public:
		LMTVector() : _data(nullptr), _size(0), _capacity(0) {}
		~LMTVector()
		{
			LMT_DEALLOC(_data);
		}
		LMT_INLINE void push_back(const T &o)
		{
			// TODO log
			if (_size == _capacity)
			{
				reserve(_capacity ? _capacity * 2 : 4);
			}
			new (&_data[_size++]) T(o);
		}
		LMT_INLINE void pop_back()
		{
			if (_size == 0)
				return;
			_data[_size - 1].~T();
			--_size;
		}
		LMT_INLINE void resize(uint16_t size)
		{
			if (size <= _capacity && size >= _size)
			{
				_size = size;
				return;
			}
			reserve(size);
			_size = size;
		}
		LMT_INLINE void clear()
		{
			for (uint16_t i = 0; i < _size; ++i)
			{
				_data[i].~T();
			}
			_size = 0;
		}
		LMT_INLINE T *insert(T* iterator, const T &value)
		{
			if (_size == _capacity)
			{
				auto offset = iterator - _data;
				reserve(_capacity ? _capacity * 2 : 4);
				iterator = _data + offset;
			}
			new (end()) T;
			++_size;

			auto it = end() - 1;
			while (it != iterator)
			{
				*it = *(it - 1);
				it -= 1;
			}

			*iterator = value;
			return iterator;
		}
		LMT_INLINE T &operator[](size_t i) { return _data[i]; }
		LMT_INLINE const T &operator[](size_t i) const { return _data[i]; }
		LMT_INLINE T *begin() { return _data; }
		LMT_INLINE T *end() { return _data + _size; }
		LMT_INLINE const T *begin() const { return _data; }
		LMT_INLINE const T *end() const { return _data + _size; }
	private:
		LMT_INLINE void reserve(Hash capacity)
		{
			_capacity = capacity;
			T *newData = (T*)LMT_ALLOC(_capacity * sizeof(T));
			memcpy(newData, _data, _size * sizeof(T));
			LMT_DEALLOC(_data);
			_data = newData;
		}
		T        *_data;
		uint16_t _size;
		uint16_t _capacity;

		LMTVector(const LMTVector &o);
		LMTVector(LMTVector &&o);
		LMTVector &operator=(const LMTVector &o);
		LMTVector &operator=(LMTVector &&o);
	};

	struct Header
	{
		Hash      hash;
		uint64_t  size : 63;
		uint64_t  aligned : 1;
	};

	static const size_t HEADER_SIZE = sizeof(Header);
	static const size_t ALIGNED_HEADER_SIZE = sizeof(size_t) + sizeof(Header);

	enum class ChunkStatus : size_t
	{
		TREATED = 0,
		PENDING,
		TEMPORARY
	};

	struct Chunk
	{
		ptrdiff_t                allocSize[LMT_ALLOC_NUMBER_PER_CHUNK];
		Hash                     allocHash[LMT_ALLOC_NUMBER_PER_CHUNK];
		size_t                   allocStackIndex[LMT_ALLOC_NUMBER_PER_CHUNK];
		uint8_t                  allocStackSize[LMT_ALLOC_NUMBER_PER_CHUNK];
		void                     *stackBuffer[LMT_ALLOC_NUMBER_PER_CHUNK * LMT_STACK_SIZE_PER_ALLOC];
		size_t                   allocIndex;
		size_t                   stackIndex;
		Chunk                    *next;
		std::atomic<ChunkStatus> status;
	};

	struct Edge;

	struct Alloc
	{
		ptrdiff_t allocSize;
		ptrdiff_t allocSizeCache;
#ifdef LMT_CAPTURE_ACTIVATED
		ptrdiff_t allocSizeCapture;
#endif
		const char *str;
		Alloc *next;
		Alloc *shared;
		Edge  *edges;
		Alloc() : allocSize(0), allocSizeCache(0), str(nullptr), next(nullptr), shared(nullptr), edges(nullptr) {}
	};

	struct AllocStack
	{
		ptrdiff_t allocSize;
		LMTVector<Alloc*> stackAllocs;
		Hash hash;
		uint8_t stackSize;
		AllocStack() : hash(0), allocSize(0), stackSize(0) {}
	};

	struct Edge
	{
		ptrdiff_t allocSize;
		ptrdiff_t allocSizeCache;
#ifdef LMT_CAPTURE_ACTIVATED
		ptrdiff_t allocSizeCapture;
#endif
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
		ptrdiff_t instanceCount;
#endif
		Alloc *alloc;
		LMTVector<Edge*> to;
		Edge *from;
		Edge *same;
		uint8_t depth;
		Edge() : allocSize(0), alloc(nullptr), from(nullptr), same(nullptr)
		{
#ifdef LMT_CAPTURE_ACTIVATED
			allocSizeCapture = 0;
#endif
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
			instanceCount = 0;
#endif
		}
	};

	template <typename Key, typename Value, Hash Capacity>
	class Dictionary
	{
	public:
		static const size_t   HASH_EMPTY = Hash(-1);
		static const size_t   HASH_INVALID = Hash(-2);

		Dictionary(const char *name)
			:_name(name)
		{
		}

		class Pair
		{
		private:
			Hash  _hash;
			Key   _key;
			Value _value;
			Pair() : _hash(HASH_EMPTY) {}
		public:
			LMT_INLINE Value &getValue() { return _value; }
			friend class Dictionary;
		};

		Pair *update(const Key &key)
		{
			const Hash hash = getHash(key);
			if (hash == HASH_INVALID)
			{
				static Pair fakePair;
				return &fakePair;
			}

			Pair *pair = &_buffer[hash];
			if (pair->_hash == HASH_EMPTY)
			{
				pair->_hash = hash;
				pair->_key = key;
#ifdef LMT_STATS
				_size.fetch_add(1);
#endif
			}
			return pair;
		}

#ifdef LMT_STATS
		LMT_INLINE float getHitStats() const
		{
			return _hitTotal.load() / float(_hitCount.load());
		}

		LMT_INLINE float getRatio() const
		{
			size_t size = _size;
			return size / float(Capacity) * 100.f;
		}
#endif
	private:
		Pair _buffer[Capacity];
		const char *_name;
#ifdef LMT_STATS
		mutable std::atomic_size_t _hitCount;
		mutable std::atomic_size_t _hitTotal;
		mutable std::atomic_size_t _size;
#endif

		LMT_INLINE Hash getHash(const Key &key) const
		{
			for (Hash i = 0; i < Capacity; ++i)
			{
				const Hash realHash = (key + i) % Capacity;
				if (_buffer[realHash]._hash == HASH_EMPTY || _buffer[realHash]._key == key)
				{
#ifdef LMT_STATS
					_hitCount += 1;
					_hitTotal += i;
#endif
					return realHash;
				}
			}
#ifdef LMT_STATS
			_hitTotal += Capacity - 1;
#endif
			LMT_DEBUG_ASSERT(false, "LMT : Dictionary %s is full.", _name);
			return HASH_INVALID;
		}
	};

	enum RunningStatus : unsigned char
	{
		NOT_INITIALIZED,
		RUNNING,
		EXIT
	};

	LMT_TLS static Chunk                     *g_th_currentChunk = nullptr;
	LMT_TLS static Chunk                     g_th_chunks[LMT_CHUNK_NUMBER_PER_THREAD];
	LMT_TLS static uint8_t                   g_th_chunkIndex = 0;
	LMT_TLS static Hash                      g_th_cache[LMT_CACHE_SIZE];
	LMT_TLS static uint8_t                   g_th_cacheIndex = 0;
	LMT_TLS static bool                      g_th_initialized = false;
	LMT_TLS static uint8_t                   g_th_lmt_internal_scope = 0;


	static Alloc                             *g_allocList = nullptr;

	struct TreeKey
	{
		TreeKey()
			: hash(0), str(0){}

		TreeKey(Hash _hash, Hash _str)
			: hash(_hash), str(_str)
		{
		}
		TreeKey& operator=(const TreeKey &o)
		{
			this->hash = o.hash;
			this->str = o.str;
			return *this;
		}
		bool operator==(const TreeKey &o) const
		{
			return ((hash == o.hash)
				&& (str == o.str));
		}
		Hash operator+(Hash i) const { return (hash + i); }
		Hash hash;
		Hash str;
	};

	static Dictionary<Hash, AllocStack, LMT_STACK_DICTIONARY_SIZE>    g_stackDictionary("STACK_DICTIONARY");
	static Dictionary<Hash, Alloc, LMT_ALLOC_DICTIONARY_SIZE>         g_allocDictionary("ALLOC_DICTIONARY");
	static Dictionary<TreeKey, Edge, LMT_TREE_DICTIONARY_SIZE>        g_treeDictionary("TREE_DICTIONARY");

#ifdef LMT_STATS
	static std::atomic_size_t                                   g_userAllocations;
	static std::atomic_size_t                                   g_realUserAllocations;
	static std::atomic_size_t                                   g_internalAllocations;
#endif

	static LMTVector<Edge*>                                     g_allocStackRoots;
	static std::mutex                                           g_mutex;

	static std::atomic<RunningStatus>                           g_runningStatus = LMT_ATOMIC_INITIALIZER(RunningStatus::NOT_INITIALIZED);
	static std::atomic_size_t                                   g_temporaryChunkCounter = LMT_ATOMIC_INITIALIZER(0);

	static const size_t                                         g_internalPerThreadMemoryUsed =
		sizeof(g_th_currentChunk)
		+ sizeof(g_th_chunks)
		+ sizeof(g_th_chunkIndex)
		+ sizeof(g_th_cache)
		+ sizeof(g_th_cacheIndex)
		+ sizeof(g_th_initialized)
		+ sizeof(g_th_lmt_internal_scope);

	static const size_t                                         g_internalSharedMemoryUsed =
		sizeof(g_stackDictionary)
		+ sizeof(g_allocDictionary)
		+ sizeof(g_treeDictionary)
#ifdef LMT_STATS
		+ sizeof(g_userAllocations)
		+ sizeof(g_realUserAllocations)
		+ sizeof(g_internalAllocations)
#endif
		+ sizeof(g_allocList)
		+ sizeof(g_allocStackRoots)
		+ sizeof(g_mutex)
		+ sizeof(g_internalPerThreadMemoryUsed)
		+ sizeof(g_runningStatus)
		+ sizeof(g_temporaryChunkCounter)
		+ sizeof(size_t) /* itself */;

	static std::atomic_size_t                                   g_internalAllThreadsMemoryUsed = LMT_ATOMIC_INITIALIZER(g_internalSharedMemoryUsed);

	namespace Renderer
	{
		struct Histogram
		{
			Alloc *function;
			Edge  *call;
			const char *name;
			bool  isFunction;
			ptrdiff_t allocSize[HISTORY_FRAME_NUMBER];
			int  cursor;
			ptrdiff_t allocSizeCache;
			Histogram() : function(nullptr), call(nullptr), name(nullptr), isFunction(false), cursor(0), allocSizeCache(0)
			{
				memset(allocSize, 0, sizeof(allocSize));
			}
		};

		enum DisplayType : int
		{
			CALLEE = 0,
			FUNCTION,
			STACK,
			HISTOGRAMS,
			END
		};

		static const char *DisplayTypeStr[] =
		{
			"Callee",
			"Function",
			"Stack",
			"Histograms"
		};

		enum UpdateType : uint8_t
		{
			NONE = 0,
			CURRENT_FRAME,
			CURRENT_AND_NEXT_FRAME
		};

		struct GroupedEdge
		{
			Alloc     *alloc;
			ptrdiff_t  allocSize;
		};

		static bool                                g_refeshAuto = true;
		static UpdateType                          g_updateType = UpdateType::NONE;
		static bool                                g_updateSearch = false;
		static float                               g_updateRatio = 0.f;
		static const size_t                        g_search_str_length = 1024;
		static char                                g_searchStr[g_search_str_length];
		static DisplayType                         g_displayType = DisplayType::STACK;
		static LMTVector<Histogram>                g_histograms;
		static Alloc                              *g_searchResult;
		static Alloc                              *g_functionView;
		static LMTVector<GroupedEdge>              g_groupedEdges;
		static LMTVector<Edge*>                    g_sortedEdges;

		bool searchAlloc();
		void renderCallee(Edge *callee, bool callerTooltip);
		void renderCallees();
		void renderFunctionView();
		void renderMenu();
		void renderHistograms();
		void renderStack();
		void recursiveCacheData(Edge *edge);
		void cacheData();
#ifdef LMT_CAPTURE_ACTIVATED
		void capture();
#endif
		void createHistogram(Alloc *function);
		void createHistogram(Edge  *functionCall);
		void render(float dt);
	}

	static bool chunkIsNotFull(const Chunk *chunk);
	static Chunk *createTemporaryChunk();
	static Chunk *createPreallocatedChunk(const RunningStatus status);
	static uint8_t findInCache(Hash hash);
	static void logAllocInChunk(Header *header, size_t size);
	static void logFreeInChunk(Header *header);
	static void treatChunk(Chunk *chunk);
	static void updateTree(AllocStack &alloc, ptrdiff_t size, bool checkTree);
}
#endif

#ifdef LMT_IMPL
namespace LiveMemTracer
{
#define GET_HEADER(ptr) (Header*)((void*)((size_t)ptr - HEADER_SIZE))
#define GET_ALIGNED_PTR(ptr) (void*)(*(size_t*)((void*)(size_t(ptr) - ALIGNED_HEADER_SIZE)))
#define GET_ALIGNED_SIZE(size, alignment) size + --alignment + ALIGNED_HEADER_SIZE
#ifdef LMT_STATS
#define LOG_REAL_SIZE_ALLOC(header, ptr) g_realUserAllocations.fetch_add(size_t(uint64_t(ptr) - uint64_t(header) + header->size)); g_userAllocations.fetch_add(header->size)
#define LOG_REAL_SIZE_FREE(header, ptr)  g_realUserAllocations.fetch_sub(size_t(uint64_t(ptr) - uint64_t(header) + header->size)); g_userAllocations.fetch_sub(header->size)
#else
#define LOG_REAL_SIZE_ALLOC(header, ptr)
#define LOG_REAL_SIZE_FREE(header, ptr)
#endif
	static LMT_INLINE void *REGISTER_ALIGNED_PTR(void *ptr, size_t alignment)
	{
		size_t t = (size_t)ptr + ALIGNED_HEADER_SIZE;
		size_t o = (t + alignment) & ~alignment;
		size_t *addrPtr = (size_t*)((void*)(o - ALIGNED_HEADER_SIZE));
		*addrPtr = (size_t)ptr;
		return (void*)o;
	}
#define IS_ALIGNED(POINTER, BYTE_COUNT) \
	(((uintptr_t)(const void *)(POINTER)) % (BYTE_COUNT) == 0)

#ifdef LMT_STATS
	struct InternalScope
	{
		InternalScope()  { ++g_th_lmt_internal_scope; }
		~InternalScope() { --g_th_lmt_internal_scope; }
	};
#define INTERNAL_SCOPE ::LiveMemTracer::InternalScope scopeGuard
#define IS_IN_INTERNAL_SCOPE() g_th_lmt_internal_scope > 0
#else
#define INTERNAL_SCOPE;
#endif
}

void *LiveMemTracer::alloc(size_t size)
{
	void *ptr = LMT_USE_MALLOC(size + HEADER_SIZE);
	void *userPtr = (void*)(size_t(ptr) + HEADER_SIZE);
	LMT_ASSERT(ptr != nullptr, "Out of memory");
	if (!ptr)
		return nullptr;
	Header *header = (Header*)(ptr);
	logAllocInChunk(header, size);
	LOG_REAL_SIZE_ALLOC(header, userPtr);
	header->aligned = 0;
	return userPtr;
}

void *LiveMemTracer::allocAligned(size_t size, size_t alignment)
{
	if (alignment < 8)
	{
		alignment = 8;
	}

	size_t allocatedSize = GET_ALIGNED_SIZE(size, alignment);
	void* r = LMT_USE_MALLOC(allocatedSize);
	LMT_ASSERT(r != nullptr, "Out of memory");
	if (!r)
		return nullptr;
	void* o = REGISTER_ALIGNED_PTR(r, alignment);

	Header* header = GET_HEADER(o);
	logAllocInChunk(header, size);
	LOG_REAL_SIZE_ALLOC(header, o);
	header->aligned = 1;
	LMT_ASSERT(IS_ALIGNED(o, alignment + 1), "Not aligned");
	LMT_ASSERT(o != nullptr, "");
	return (void*)o;
}

void *LiveMemTracer::realloc(void *ptr, size_t size)
{
	if (ptr == nullptr)
	{
		return alloc(size);
	}

	Header *header = GET_HEADER(ptr);

	if (size == 0)
	{
		dealloc(ptr);
		return alloc(0);
	}

	if (size == header->size)
	{
		return ptr;
	}
	logFreeInChunk(header);
	LOG_REAL_SIZE_FREE(header, ptr);
	void *newPtr = LMT_USE_REALLOC((void*)header, size + HEADER_SIZE);
	LMT_ASSERT(newPtr != nullptr, "Out of memory");
	if (!newPtr)
		return newPtr;
	header = (Header*)(newPtr);
	logAllocInChunk(header, size);
	void *userPtr = (void*)(size_t(newPtr) + HEADER_SIZE);
	LOG_REAL_SIZE_ALLOC(header, userPtr);
	header->aligned = 0;
	return userPtr;
}

void *LiveMemTracer::reallocAligned(void *ptr, size_t size, size_t alignment)
{
	if (ptr == nullptr)
	{
		return allocAligned(size, alignment);
	}

	Header oldHeader = *GET_HEADER(ptr);

	if (size == 0)
	{
		deallocAligned(ptr);
		return allocAligned(0, alignment);
	}

	if (size == oldHeader.size)
	{
		return ptr;
	}
	LMT_ASSERT(oldHeader.aligned == 1, "");
	LMT_ASSERT(IS_ALIGNED(ptr, alignment), "");
	void *newPtr = allocAligned(size, alignment);
	if (!newPtr)
		return nullptr;
	memcpy(newPtr, ptr, size_t(oldHeader.size < size ? oldHeader.size : size));
	deallocAligned(ptr);
	LMT_ASSERT(IS_ALIGNED(newPtr, alignment), "");
	LMT_ASSERT(newPtr != nullptr, "");
	return newPtr;
	/*size_t reallocSize = GET_ALIGNED_SIZE(size, alignment);
	void *alignedPtr = GET_ALIGNED_PTR(ptr);
	void* r = LMT_USE_REALLOC(alignedPtr, reallocSize);
	if (!r) return NULL;
	void* o = REGISTER_ALIGNED_PTR(r, alignment);

	Header *newHeader = GET_HEADER(o);
	logAllocInChunk(newHeader, size);
	newHeader->aligned = 1;
	newHeader->size = size;
	logFreeInChunk(&oldHeader);
	return o;*/
}

void LiveMemTracer::dealloc(void *ptr)
{
	if (ptr == nullptr)
		return;
	Header *header = GET_HEADER(ptr);
	LMT_DEBUG_ASSERT(header->aligned == 0, "Trying to free an aligned ptr with a non-aligned free");
	logFreeInChunk(header);
	LOG_REAL_SIZE_FREE(header, ptr);
	LMT_USE_FREE((void*)header);
}

void LiveMemTracer::deallocAligned(void *ptr)
{
	if (ptr == nullptr)
		return;
	Header *header = GET_HEADER(ptr);
	LMT_DEBUG_ASSERT(header->aligned == 1, "Trying to free an non-aligned ptr with an aligned free");
	logFreeInChunk(header);
	LOG_REAL_SIZE_FREE(header, ptr);
	LMT_USE_FREE(GET_ALIGNED_PTR(ptr));
}

void LiveMemTracer::exit()
{
	g_runningStatus = EXIT;
}

void LiveMemTracer::init()
{
	INTERNAL_SCOPE;
	g_runningStatus = RUNNING;
	SymbolGetter::init();
}


//////////////////////////////////////////////////////////////////////////
// IMPL ONLY : 
//////////////////////////////////////////////////////////////////////////

#ifdef LMT_x64
#define LMT_HASH_FROM_PTR(ptr) combineHash(ptr)
#else
#define LMT_HASH_FROM_PTR(ptr) Hash(ptr)
#endif

bool LiveMemTracer::chunkIsNotFull(const Chunk *chunk)
{
	return (chunk
		&& chunk->allocIndex < LMT_ALLOC_NUMBER_PER_CHUNK
		&& chunk->stackIndex < LMT_ALLOC_NUMBER_PER_CHUNK * LMT_STACK_SIZE_PER_ALLOC);
}

LiveMemTracer::Chunk *LiveMemTracer::createTemporaryChunk()
{
	void *ptr = LMT_USE_MALLOC(sizeof(Chunk));
	LMT_ASSERT(ptr != nullptr, "Out of memory");
	if (ptr == nullptr)
		return nullptr;
#ifdef LMT_STATS
	g_internalAllocations.fetch_add(sizeof(Chunk));
#endif
	memset(ptr, 0, sizeof(Chunk));
	Chunk *tmpChunk = new(ptr)Chunk;
	memset(g_th_cache, 0, sizeof(g_th_cache));
	g_th_cacheIndex = 0;
	tmpChunk->allocIndex = 0;
	tmpChunk->stackIndex = 0;
	tmpChunk->status = ChunkStatus::TEMPORARY;
	g_temporaryChunkCounter.fetch_add(1);
	return tmpChunk;
}

LiveMemTracer::Chunk *LiveMemTracer::createPreallocatedChunk(const RunningStatus status)
{
	// If it's not running we do not cycle around pre-allocated
	// chunks, we just use them once.
	if (status != RunningStatus::RUNNING
		&& g_th_chunkIndex + 1 >= LMT_CHUNK_NUMBER_PER_THREAD)
	{
		return nullptr;
	}

	// We get the next preallocated chunk
	g_th_chunkIndex = (g_th_chunkIndex + 1) % LMT_CHUNK_NUMBER_PER_THREAD;
	Chunk *chunk = &g_th_chunks[g_th_chunkIndex];

	// If new chunk is pending for treatment we return nullptr
	if (chunk->status.load() == ChunkStatus::PENDING)
	{
		return nullptr;
	}

	// Else we init chunk and return it
	g_th_cacheIndex = 0;
	memset(g_th_cache, 0, sizeof(g_th_cache));
	chunk->allocIndex = 0;
	chunk->stackIndex = 0;
	return chunk;
}

LiveMemTracer::Chunk *LiveMemTracer::getChunk(bool forceFlush /*= false*/)
{
	const RunningStatus status = g_runningStatus;

	// We initialized TLS values
	if (!g_th_initialized)
	{
		memset(g_th_chunks, 0, sizeof(g_th_chunks));
		memset(g_th_cache, 0, sizeof(g_th_cache));
		g_internalAllThreadsMemoryUsed.fetch_add(g_internalPerThreadMemoryUsed);
		g_th_initialized = true;
	}

	// If LMT is not initialized
	if (status != RunningStatus::RUNNING)
	{
		// If current chunk is not full we use it
		if (chunkIsNotFull(g_th_currentChunk))
		{
			return g_th_currentChunk;
		}
		// Else we try to use TLS preallocated chunk
		g_th_currentChunk = createPreallocatedChunk(status);
		if (g_th_currentChunk != nullptr)
		{
			return g_th_currentChunk;
		}
		// Else we return a temporary chunk
		g_th_currentChunk = createTemporaryChunk();
		return g_th_currentChunk;
	}
	// Else if LMT is running
	else
	{
		// If current chunk is not full and treat current chunk time is not came
		if (forceFlush == false && chunkIsNotFull(g_th_currentChunk))
		{
			return g_th_currentChunk;
		}
		// Else we search for a new one
		// and set old chunk status as pending 
		Chunk *oldChunk = g_th_currentChunk;
		if (oldChunk && oldChunk->status != ChunkStatus::TEMPORARY)
		{
			oldChunk->status = ChunkStatus::PENDING;
		}
		g_th_currentChunk = createPreallocatedChunk(status);
		// And treat old chunk
		if (g_th_currentChunk && oldChunk)
		{
			LMT_TREAT_CHUNK(oldChunk);
		}
		// If there were a free preallocated chunk we return it
		if (g_th_currentChunk)
		{
			return g_th_currentChunk;
		}
		// Else we create a temporary one
		g_th_currentChunk = createTemporaryChunk();

		if (g_th_currentChunk && oldChunk)
		{
			LMT_TREAT_CHUNK(oldChunk);
		}

		return g_th_currentChunk;
	}
	return nullptr;
}

uint8_t LiveMemTracer::findInCache(LiveMemTracer::Hash hash)
{
	int i = int(g_th_cacheIndex - 1);
	if (i < 0)
		i = LMT_CACHE_SIZE - 1;
	uint8_t res = 1;
	while (true)
	{
		if (g_th_cache[i] == hash)
		{
			return res - 1;
		}
		i -= 1;
		if (i < 0)
			i = LMT_CACHE_SIZE - 1;
		if (++res == LMT_CACHE_SIZE)
			break;
	}
	return uint8_t(-1);
}

void LiveMemTracer::logAllocInChunk(LiveMemTracer::Header *header, size_t size)
{
#ifdef LMT_STATS
	if (IS_IN_INTERNAL_SCOPE())
	{
		g_internalAllocations.fetch_add(size);
	}
#endif
	INTERNAL_SCOPE;
	Chunk *chunk = getChunk();

	header->hash = 0;
	void **stack = &chunk->stackBuffer[chunk->stackIndex];
	uint32_t count = getCallstack(LMT_STACK_SIZE_PER_ALLOC, stack, &header->hash);

	header->size = size;

	size_t index = chunk->allocIndex;
	uint8_t found = findInCache(header->hash);
	if (found != uint8_t(-1))
	{
#ifndef LMT_INSTANCE_COUNT_ACTIVATED
		index = chunk->allocIndex - found - 1;
		chunk->allocSize[index] += size;
		return;
#else
		chunk->allocStackIndex[index] = chunk->allocStackIndex[chunk->allocIndex - found - 1];
		chunk->allocSize[index] = size;
		chunk->allocHash[index] = chunk->allocHash[chunk->allocIndex - found - 1];
		chunk->allocStackSize[index] = chunk->allocStackSize[chunk->allocIndex - found - 1];
		chunk->allocIndex += 1;
		g_th_cache[g_th_cacheIndex] = header->hash;
		g_th_cacheIndex = (g_th_cacheIndex + 1) % LMT_CACHE_SIZE;
		return;
#endif
	}

	chunk->allocStackIndex[index] = chunk->stackIndex;
	chunk->allocSize[index] = size;
	chunk->allocHash[index] = header->hash;
	chunk->allocStackSize[index] = count;
	g_th_cache[g_th_cacheIndex] = header->hash;
	g_th_cacheIndex = (g_th_cacheIndex + 1) % LMT_CACHE_SIZE;
	chunk->allocIndex += 1;
	chunk->stackIndex += count;
}

void LiveMemTracer::logFreeInChunk(LiveMemTracer::Header *header)
{
#ifdef LMT_STATS
	if (IS_IN_INTERNAL_SCOPE())
	{
		g_internalAllocations.fetch_sub(header->size);
	}
#endif
	INTERNAL_SCOPE;
	Chunk *chunk = getChunk();

	size_t index = chunk->allocIndex;
	uint8_t found = findInCache(header->hash);
	if (found != uint8_t(-1))
	{
#ifndef LMT_INSTANCE_COUNT_ACTIVATED
		index = chunk->allocIndex - found - 1;
		chunk->allocSize[index] -= ptrdiff_t(header->size);
		return;
#endif
	}

	g_th_cache[g_th_cacheIndex] = header->hash;
	g_th_cacheIndex = (g_th_cacheIndex + 1) % LMT_CACHE_SIZE;
	chunk->allocStackIndex[index] = size_t(-1);
	chunk->allocSize[index] = -ptrdiff_t(header->size);
	chunk->allocHash[index] = header->hash;
	chunk->allocStackSize[index] = 0;
	chunk->allocIndex += 1;
}

#ifdef LMT_INSTANCE_COUNT_ACTIVATED
#define LMT_INC_INSTANCE(instance, size) instance += size > 0 ? 1 : -1
#else
#define LMT_INC_INSTANCE(instance, size)
#endif

void LiveMemTracer::treatChunk(Chunk *chunk)
{
	INTERNAL_SCOPE;
	std::lock_guard<std::mutex> lock(g_mutex);
	for (size_t i = 0, iend = chunk->allocIndex; i < iend; ++i)
	{
		auto size = chunk->allocSize[i];
		if (size == 0)
			continue;
		auto it = g_stackDictionary.update(chunk->allocHash[i]);
		auto &allocStack = it->getValue();
		allocStack.allocSize += size;
		if (allocStack.stackSize != 0)
		{
			updateTree(allocStack, size, false);
			for (size_t j = 0; j < allocStack.stackSize; ++j)
			{
				allocStack.stackAllocs[j]->allocSize += size;
			}
			continue;
		}
		allocStack.hash = chunk->allocHash[i];
		allocStack.stackAllocs.resize(chunk->allocStackSize[i]);
		for (size_t j = 0, jend = chunk->allocStackSize[i]; j < jend; ++j)
		{
			void *addr = chunk->stackBuffer[chunk->allocStackIndex[i] + j];
			auto found = g_allocDictionary.update(LMT_HASH_FROM_PTR(addr));
			if (found->getValue().shared != nullptr)
			{
				auto shared = found->getValue().shared;
				shared->allocSize += size;
				allocStack.stackAllocs[j] = shared;
				continue;
			}
			if (found->getValue().str != nullptr)
			{
				allocStack.stackAllocs[j] = &found->getValue();
				found->getValue().allocSize += size;
				continue;
			}
			void *absoluteAddress = nullptr;
			const char *name = SymbolGetter::getSymbol(addr, absoluteAddress);

			auto shared = g_allocDictionary.update(LMT_HASH_FROM_PTR(absoluteAddress));
			if (shared->getValue().str != nullptr)
			{
				found->getValue().shared = &shared->getValue();
				allocStack.stackAllocs[j] = &shared->getValue();
				shared->getValue().allocSize += size;
#ifdef LMT_PLATFORM_WINDOWS
				if (name != TRUNCATED_STACK_NAME)
					LMT_USE_FREE((void*)name);
#endif
				continue;
			}

			found->getValue().shared = &shared->getValue();
			shared->getValue().str = name;
			shared->getValue().allocSize = size;

			allocStack.stackAllocs[j] = &shared->getValue();
			shared->getValue().next = g_allocList;
			g_allocList = &shared->getValue();
		}
		allocStack.stackSize = chunk->allocStackSize[i];
		updateTree(allocStack, size, true);
	}
	if (chunk->status == ChunkStatus::TEMPORARY)
	{
		chunk->~Chunk();
		LMT_USE_FREE(chunk);
#ifdef LMT_STATS
		g_internalAllocations.fetch_sub(sizeof(Chunk));
#endif
		g_temporaryChunkCounter.fetch_sub(1);
	}
	else
	{
		chunk->status.store(ChunkStatus::TREATED);
	}
}

void LiveMemTracer::updateTree(AllocStack &allocStack, ptrdiff_t size, bool checkTree)
{
	int stackSize = allocStack.stackSize;
	stackSize -= INTERNAL_FRAME_TO_SKIP;
	uint8_t depth = 0;
	Edge *previousPtr = nullptr;
	while (stackSize >= 0)
	{
		Hash currentHash = LMT_HASH_FROM_PTR(allocStack.stackAllocs[stackSize]);
		currentHash = combineHash((size_t(previousPtr)), currentHash);
		currentHash = combineHash(depth * depth, currentHash);

		TreeKey key(currentHash, LMT_HASH_FROM_PTR(allocStack.stackAllocs[stackSize]->str));

		auto pair = g_treeDictionary.update(key);
		Edge *currentPtr = &pair->getValue();
		currentPtr->allocSize += size;
		LMT_INC_INSTANCE(currentPtr->instanceCount, size);
		if (checkTree)
		{
			if (!currentPtr->alloc)
			{
				LMT_DEBUG_ASSERT(currentPtr->same == nullptr, "Edge already have a same pointer defined.");
				currentPtr->alloc = allocStack.stackAllocs[stackSize];
				currentPtr->same = allocStack.stackAllocs[stackSize]->edges;
				allocStack.stackAllocs[stackSize]->edges = currentPtr;
			}

			if (previousPtr != nullptr)
			{
				auto it = std::find(previousPtr->to.begin(), previousPtr->to.end(), currentPtr);
				if (it == previousPtr->to.end())
				{
					previousPtr->to.push_back(currentPtr);
				}
				currentPtr->from = previousPtr;
			}
			else
			{
				auto it = std::find(g_allocStackRoots.begin(), g_allocStackRoots.end(), currentPtr);
				if (it == g_allocStackRoots.end())
					g_allocStackRoots.push_back(currentPtr);
			}

			currentPtr->depth = depth;
		}
		LMT_DEBUG_ASSERT(strcmp(currentPtr->alloc->str, allocStack.stackAllocs[stackSize]->str) == 0, "Name collision.");

		previousPtr = currentPtr;
		++depth;
		--stackSize;
	}
}

template <class T>
LMT_INLINE LiveMemTracer::Hash LiveMemTracer::combineHash(const T& val, const LiveMemTracer::Hash baseHash)
{
	const uint8_t* bytes = (uint8_t*)&val;
	const size_t count = sizeof(val);
	const Hash FNV_offset_basis = baseHash;
	const Hash FNV_prime = 16777619U;

	Hash hash = FNV_offset_basis;
	for (size_t i = 0; i < count; ++i)
	{
		hash ^= (Hash)bytes[i];
		hash *= FNV_prime;
	}

	return hash;
}

#if LMT_IMGUI
#include LMT_IMGUI_INCLUDE_PATH

namespace LiveMemTracer
{
	namespace Renderer
	{
		static bool SortGroupedEdge(const GroupedEdge &a, const GroupedEdge &b)
		{
			return a.allocSize > b.allocSize;
		}

		static bool InsertSortedEdge(const GroupedEdge &a, const GroupedEdge &b)
		{
			return a.alloc < b.alloc;
		}

		float formatMemoryString(ptrdiff_t sizeInBytes, const char *&str)
		{
			float multiplier = 1.f;
			if (sizeInBytes < 0)
			{
				multiplier = -1.f;
				sizeInBytes *= -1;
			}
			if (sizeInBytes < 10 * 1024)
			{
				str = "B";
				return float(sizeInBytes) * multiplier;
			}
			else if (sizeInBytes < 10 * 1024 * 1024)
			{
				str = "KiB";
				return sizeInBytes / 1024.f * multiplier;
			}
			else
			{
				str = "Mb";
				return sizeInBytes / 1024 / 1024.f * multiplier;
			}
		}

		bool searchAlloc()
		{
			Alloc *alloc = g_allocList;

			Alloc **prevNext = &g_allocList;
			g_searchResult = nullptr;

			while (alloc != nullptr)
			{
				Alloc *next = alloc->next;
				if (LMT_STRSTRI(alloc->str, g_searchStr) != nullptr)
				{
					*prevNext = alloc->next;
					alloc->next = g_searchResult;
					g_searchResult = alloc;
				}
				else
				{
					prevNext = &alloc->next;
				}
				alloc = next;
			}

			bool hasChanged = true;
			while (g_searchResult && hasChanged)
			{
				hasChanged = false;
				Alloc **pv = &g_searchResult;
				Alloc *nd = g_searchResult;
				nd->allocSizeCache = nd->allocSize;
				Alloc *nx = g_searchResult->next;

				while (nx)
				{
					nx->allocSizeCache = nx->allocSize;
					if (nd->allocSizeCache < nx->allocSizeCache)
					{
						nd->next = nx->next;
						nx->next = nd;
						*pv = nx;

						hasChanged = true;
					}
					pv = &nd->next;
					nd = nx;
					nx = nx->next;
				}
			}
			*prevNext = g_searchResult;

			return g_searchResult != nullptr;
		}

		void displayCallerTooltip(Edge *from, size_t &depth)
		{
			if (!from)
				return;
			size_t depthCopy = depth;
			displayCallerTooltip(from->from, ++depth);
			if (from->from)
				ImGui::Indent();
			ImGui::Text(from->alloc->str);
			if (depthCopy == 0)
			{
				while (depth - 1 > 0)
				{
					ImGui::Unindent();
					--depth;
				}
			}
		}

#define PAD_AND_SET_IMGUI_CURSOR(cursor, pad) cursor.x += pad; ImGui::SetCursorPos(cursor)

		void renderCallee(Edge *callee, bool callerTooltip)
		{
			if (!callee)
				return;

			ImGui::PushID(callee);
			const char *suffix;
			if (g_updateType != UpdateType::NONE)
			{
				callee->allocSizeCache = callee->allocSize;
			}
			float size = formatMemoryString(callee->allocSizeCache, suffix);
			auto cursorPos = ImGui::GetCursorPos();
			const bool opened = ImGui::TreeNode(callee, "%4.0f %s", size, suffix);
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("%4u", callee->instanceCount);
#endif
#ifdef LMT_CAPTURE_ACTIVATED
			ptrdiff_t diff = callee->allocSizeCache - callee->allocSizeCapture;
			size = formatMemoryString(diff, suffix);
			const char *capturedSuffix;
			float capturedSize = formatMemoryString(callee->allocSizeCapture, capturedSuffix);
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			if (diff > 0)
			{
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%4.0f %s", size, suffix);
			}
			else
			{
				ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "%4.0f %s", size, suffix);
			}
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("[%4.0f %s]", capturedSize, capturedSuffix);
#endif
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("%s", callee->alloc->str);
			if (callerTooltip && ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				size_t depth;
				displayCallerTooltip(callee->from, depth);
				ImGui::EndTooltip();
			}
			if (ImGui::BeginPopupContextItem("Options"))
			{
				if (ImGui::Selectable("Watch call"))
				{
					createHistogram(callee);
				}
				if (ImGui::Selectable("Watch function"))
				{
					createHistogram(callee->alloc);
					g_displayType = DisplayType::HISTOGRAMS;
				}
				if (ImGui::Selectable("Function view"))
				{
					g_functionView = callee->alloc;
					g_displayType = DisplayType::FUNCTION;
					g_updateType = UpdateType::CURRENT_AND_NEXT_FRAME;
				}
				ImGui::EndPopup();
			}
			if (opened)
			{
				if (g_updateType != UpdateType::NONE || g_refeshAuto == false)
				{
					std::stable_sort(callee->to.begin(), callee->to.end(), [](Edge *a, Edge *b){ return a->allocSizeCache > b->allocSizeCache; });
				}
				for (auto &to : callee->to)
				{
					renderCallee(to, callerTooltip);
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		void renderCallees()
		{
			ImGui::Separator();
			ImVec2 cursorPos = ImGui::GetCursorPos();
			ImGui::Text("Size");
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Count");
#endif
#ifdef LMT_CAPTURE_ACTIVATED
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Diff");
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Captured");
#endif
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Callee");
			ImGui::Separator();
			ImGui::BeginChild("Content", ImGui::GetWindowContentRegionMax(), false, ImGuiWindowFlags_HorizontalScrollbar);
			Alloc *callee = g_searchResult;
			int i = 0;
			while (callee)
			{
				if (!callee->edges)
				{
					callee = callee->next;
					continue;
				}

				ImGui::PushID(callee);
				++i;
				auto cursorPos = ImGui::GetCursorPos();
				if (i % 2)
				{
					ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.78f, 0.45f));
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.78f, 0.65f));
					ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.4f, 0.4f, 0.78f, 0.85f));
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.57f, 0.78f, 0.45f));
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.57f, 0.78f, 0.65f));
					ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.4f, 0.57f, 0.78f, 0.85f));
				}
				bool opened = ImGui::CollapsingHeader("##dummy", "NoName", true, false);
				if (ImGui::BeginPopupContextItem("Options"))
				{
					if (ImGui::Selectable("Watch function"))
					{
						createHistogram(callee);
						g_displayType = DisplayType::HISTOGRAMS;
					}
					if (ImGui::Selectable("Function view"))
					{
						g_functionView = callee;
						g_displayType = DisplayType::FUNCTION;
						g_updateType = UpdateType::CURRENT_AND_NEXT_FRAME;
					}
					ImGui::EndPopup();
				}

				cursorPos.x += 20;
				cursorPos.y += 3;
				PAD_AND_SET_IMGUI_CURSOR(cursorPos, 0);
				const char *suffix;
				float size = formatMemoryString(callee->allocSizeCache, suffix);
				ImGui::Text("%4.0f %s", size, suffix);
				float nextPad = 80.f;
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
				PAD_AND_SET_IMGUI_CURSOR(cursorPos, nextPad);
				nextPad = 100.f;
#endif
#ifdef LMT_CAPTURE_ACTIVATED
				ptrdiff_t diff = callee->allocSizeCache - callee->allocSizeCapture;
				size = formatMemoryString(diff, suffix);
				const char *capturedSuffix;
				float capturedSize = formatMemoryString(callee->allocSizeCapture, capturedSuffix);
				PAD_AND_SET_IMGUI_CURSOR(cursorPos, nextPad);
				if (diff > 0)
				{
					ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "%4.0f %s", size, suffix);
				}
				else
				{
					ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "%4.0f %s", size, suffix);
				}
				PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
				ImGui::Text("[%4.0f %s]", capturedSize, capturedSuffix);
#endif
				PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
				ImGui::Text("%s", callee->str);
				if (opened)
				{
					g_sortedEdges.clear();
					Edge *edge = callee->edges;
					while (edge)
					{
						auto it = std::lower_bound(g_sortedEdges.begin(), g_sortedEdges.end(), edge, [](const Edge *a, const Edge *b){ return a->allocSizeCache > b->allocSizeCache; });
						if (it == g_sortedEdges.end() || *it != edge)
						{
							g_sortedEdges.insert(it, edge);
						}
						edge = edge->same;
					}
					for (auto &e : g_sortedEdges)
					{
						renderCallee(e, true);
					}
				}
				ImGui::PopID();
				ImGui::PopStyleColor(3);
				callee = callee->next;
			}
			ImGui::EndChild();
		}

		void renderFunctionView()
		{
			if (!g_functionView)
				return;
			ImGui::Columns(3);

			//////////////////////////////////////////////////////////////////////////
			// CALLERS
			Edge *edge = g_functionView->edges;
			size_t total = 0;
			g_groupedEdges.clear();

			while (edge)
			{
				if (edge->from)
				{
					GroupedEdge group;
					if (g_updateType != UpdateType::NONE)
					{
						edge->allocSizeCache = edge->allocSize;
					}
					group.alloc = edge->from->alloc;
					group.allocSize = 0;

					auto it = std::lower_bound(g_groupedEdges.begin(), g_groupedEdges.end(), group, InsertSortedEdge);
					if (it == g_groupedEdges.end() || it->alloc != group.alloc)
					{
						it = g_groupedEdges.insert(it, group);
					}
					total += edge->allocSizeCache;
					it->allocSize += edge->allocSizeCache;
				}
				edge = edge->same;
			}

			std::stable_sort(g_groupedEdges.begin(), g_groupedEdges.end(), SortGroupedEdge);

			for (auto &caller : g_groupedEdges)
			{
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				const char *suffix;
				float size = formatMemoryString(caller.allocSize, suffix);
				ImGui::TextWrapped("%s\n%4.0f%s", caller.alloc->str, size, suffix);
				ImVec2 cursorPosNext = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos(cursorPos);
				ImGui::PushID(caller.alloc);
				if (ImGui::InvisibleButton("##Finvisible", ImVec2(ImGui::GetColumnWidth(), cursorPosNext.y - cursorPos.y)))
				{
					g_functionView = caller.alloc;
					g_updateType = UpdateType::CURRENT_AND_NEXT_FRAME;
				}
				if (ImGui::BeginPopupContextItem("Options"))
				{
					if (ImGui::Selectable("Watch function"))
					{
						createHistogram(caller.alloc);
						g_displayType = DisplayType::HISTOGRAMS;
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
				ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2(cursorPos.x + float(caller.allocSize) / total * ImGui::GetColumnWidth(), ImGui::GetCursorScreenPos().y), 0x3F025CAB);
			}

			//////////////////////////////////////////////////////////////////////////
			// FUNCTION
			ImGui::NextColumn();

			const char *suffix;
			if (g_updateType != UpdateType::NONE)
			{
				g_functionView->allocSizeCache = g_functionView->allocSize;
			}
			float size = formatMemoryString(g_functionView->allocSizeCache, suffix);
			ImGui::TextWrapped("%s\n%4.0f%s", g_functionView->str, size, suffix);
			ImGui::PushID(g_functionView);
			if (ImGui::BeginPopupContextItem("Options"))
			{
				if (ImGui::Selectable("Watch function"))
				{
					createHistogram(g_functionView);
					g_displayType = DisplayType::HISTOGRAMS;
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();

			//////////////////////////////////////////////////////////////////////////
			// CALLEE
			ImGui::NextColumn();

			edge = g_functionView->edges;
			g_groupedEdges.clear();
			total = 0;

			while (edge)
			{
				for (auto &to : edge->to)
				{
					GroupedEdge group;
					if (g_updateType != UpdateType::NONE)
					{
						to->allocSizeCache = to->allocSize;
					}
					group.alloc = to->alloc;
					group.allocSize = 0;

					auto it = std::lower_bound(g_groupedEdges.begin(), g_groupedEdges.end(), group, InsertSortedEdge);
					if (it == g_groupedEdges.end() || it->alloc != group.alloc)
					{
						it = g_groupedEdges.insert(it, group);
					}

					it->allocSize += to->allocSizeCache;
					total += to->allocSizeCache;
				}
				edge = edge->same;
			}

			std::stable_sort(g_groupedEdges.begin(), g_groupedEdges.end(), SortGroupedEdge);

			for (auto &callee : g_groupedEdges)
			{
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				const char *suffix;
				float size = formatMemoryString(callee.allocSize, suffix);
				ImGui::TextWrapped("%s\n%4.0f%s", callee.alloc->str, size, suffix);
				ImVec2 cursorPosNext = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos(cursorPos);
				ImGui::PushID(callee.alloc);
				if (ImGui::InvisibleButton("##invisible", ImVec2(ImGui::GetColumnWidth(), cursorPosNext.y - cursorPos.y)))
				{
					g_functionView = callee.alloc;
					g_updateType = UpdateType::CURRENT_AND_NEXT_FRAME;
				}
				if (ImGui::BeginPopupContextItem("Options"))
				{
					if (ImGui::Selectable("Watch function"))
					{
						createHistogram(callee.alloc);
						g_displayType = DisplayType::HISTOGRAMS;
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
				ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2(cursorPos.x + float(callee.allocSize) / total * ImGui::GetColumnWidth(), ImGui::GetCursorScreenPos().y), 0x3F025CAB);
			}

			ImGui::Columns(1);
		}

		void renderHistograms()
		{
			const int columnNumber = 3;
			int i = 0;
			float histoW = ImGui::GetWindowContentRegionWidth() / columnNumber;
			ImVec2 graphSize(histoW, histoW * 0.4f);
			auto it = g_histograms.begin();
			while (it != g_histograms.end())
			{
				auto &h = *it;

				bool toDelete = false;
				ImGui::PushID(i);
				ImGui::PushItemWidth(histoW);
				ImGui::BeginGroup();
				ImGui::BeginGroup();
				ImGui::TextColored(h.isFunction ? ImColor(217, 164, 22) : ImColor(209, 6, 145), "%s", h.isFunction ? "Function" : "Call"); ImGui::SameLine();
				const char *suffix;
				float size = formatMemoryString(h.allocSizeCache, suffix);
				ImGui::Text("%4.0f %s", size, suffix);
				if (h.isFunction == false)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						size_t depth;
						displayCallerTooltip(h.call->from, depth);
						ImGui::EndTooltip();
					}
				}
				ImGui::Text("%.*s", int(histoW * 0.145f), h.name);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(h.name);
				}
				ImVec2 cursorPos = ImGui::GetCursorPos();
				ImGui::PlotLines("##NoName", (const float*)h.allocSize, HISTORY_FRAME_NUMBER, h.cursor, nullptr, 0, FLT_MAX, graphSize, sizeof(ptrdiff_t));
				ImGui::SetCursorPos(cursorPos);
				ImGui::InvisibleButton("##invisibleButton", graphSize);
				if (ImGui::IsItemHovered())
				{
					ImVec2 itemMin = ImGui::GetItemRectMin();
					ImGuiIO& g = ImGui::GetIO();
					const float x = (graphSize.x - (g.MousePos.x - itemMin.x)) / graphSize.x;
					const int itemCount = HISTORY_FRAME_NUMBER;
					const int id = itemCount - (int)(x * itemCount);
					const int itemIndex = (id + h.cursor) % itemCount;

					ImGui::SetTooltip("");
					ImGui::BeginTooltip();
					const char *ttSuffix;
					float ttSize = formatMemoryString(h.allocSize[itemIndex], ttSuffix);
					ImGui::Text("%4.0f %s", ttSize, ttSuffix);
					ImGui::EndTooltip();
				}
				ImGui::EndGroup();
				if (ImGui::BeginPopupContextItem("Options"))
				{
					toDelete = ImGui::Selectable("Delete");
					ImGui::EndPopup();
				}
				ImGui::EndGroup();
				ImGui::PopItemWidth();
				ImGui::PopID();
				if ((i % columnNumber) < columnNumber - 1)
					ImGui::SameLine();
				else
					ImGui::Separator();
				if (toDelete)
				{
					Histogram *last = g_histograms.end();
					*it = *(last - 1);
					g_histograms.pop_back();
				}
				else
				{
					++it;
					++i;
				}
			}
			ImGui::Columns(1);
		}

		void createHistogram(Alloc *function)
		{
			for (auto &h : g_histograms)
			{
				if (h.function == function)
					return;
			}
			Histogram histogram;
			histogram.function = function;
			histogram.name = function->str;
			histogram.isFunction = true;
			g_histograms.push_back(histogram);
		}

		void createHistogram(Edge *functionCall)
		{
			for (auto &h : g_histograms)
			{
				if (h.call == functionCall)
					return;
			}
			Histogram histogram;
			histogram.call = functionCall;
			histogram.name = functionCall->alloc->str;
			histogram.isFunction = false;
			g_histograms.push_back(histogram);
		}

		void renderStack()
		{
			ImGui::Separator();
			ImVec2 cursorPos = ImGui::GetCursorPos();
			ImGui::Text("Size");
#ifdef LMT_INSTANCE_COUNT_ACTIVATED
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Count");
#endif
#ifdef LMT_CAPTURE_ACTIVATED
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Diff");
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Captured");
#endif
			PAD_AND_SET_IMGUI_CURSOR(cursorPos, 100);
			ImGui::Text("Callee");
			ImGui::Separator();
			ImGui::BeginChild("Content", ImGui::GetWindowContentRegionMax(), false, ImGuiWindowFlags_HorizontalScrollbar);
			for (auto &root : g_allocStackRoots)
			{
				renderCallee(root, false);
			}
			ImGui::EndChild();
		}

		void recursiveCacheData(Edge *edge)
		{
			edge->alloc->allocSizeCache = edge->alloc->allocSize;
			for (auto &n : edge->to)
			{
				n->allocSizeCache = n->allocSize;
				recursiveCacheData(n);
			}
		}

		void cacheData()
		{
			for (auto &r : g_allocStackRoots)
			{
				r->allocSizeCache = r->allocSize;
				recursiveCacheData(r);
			}
		}

#ifdef LMT_CAPTURE_ACTIVATED
		void recursiveCapture(Edge *edge)
		{
			edge->alloc->allocSizeCapture = edge->alloc->allocSize;
			edge->allocSizeCapture = edge->allocSize;
			for (auto &n : edge->to)
			{
				n->allocSizeCapture = n->allocSize;
				recursiveCapture(n);
			}
		}

		void capture()
		{
			for (auto &r : g_allocStackRoots)
			{
				recursiveCapture(r);
			}
		}
#endif

		void renderMenu()
		{
			if (ImGui::BeginMenuBar())
			{
				ImGui::PushItemWidth(110.f);
				ImGui::Combo("##ComboMode", (int*)&g_displayType, DisplayTypeStr, DisplayType::END); ImGui::SameLine();
				if (ImGui::Checkbox("Refresh auto", &g_refeshAuto))
				{
					if (!g_refeshAuto)
					{
						cacheData();
					}
				}
				if (!g_refeshAuto)
				{
					ImGui::SameLine();
					if (ImGui::Button("Refresh"))
					{
						cacheData();
					}
				}
#ifdef LMT_CAPTURE_ACTIVATED
				ImGui::SameLine();
				if (ImGui::Button("Capture"))
				{
					capture();
				}
#endif
				ImGui::SameLine();
				if (ImGui::InputText("Search", g_searchStr, g_search_str_length))
				{
					g_displayType = DisplayType::CALLEE;
					g_updateSearch = true;
				}
				size_t temporaryChunk = g_temporaryChunkCounter;
				if (temporaryChunk > 0)
				{
					ImGui::SameLine();
					ImGui::TextColored(ImColor(1.f, 0.f, 0.f), "Temporary chunks : %i", int(temporaryChunk));
				}
				ImGui::SameLine();
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("LMT's static allocations : %0.2f Mo\n", float(g_internalAllThreadsMemoryUsed.load()) / 1024.f / 1024.f);
					ImGui::Text("LMT's dynamic allocations : %0.2f Mo", float(g_internalAllocations.load()) / 1024.f / 1024.f);
					ImGui::Separator();
#ifdef LMT_STATS
					ImGui::Text("Total allocation asked : %0.2f Mo | Real allocation done : %0.2f Mo", float(g_userAllocations.load()) / 1024.f / 1024.f, float(g_realUserAllocations.load()) / 1024.f / 1024.f);
					ImGui::Separator();
					ImGui::Text("Stack dictionary : %0.2f iterations per search | filled : %0.2f%% (LMT_STACK_DICTIONARY_SIZE) | %0.2f Mo", g_stackDictionary.getHitStats(), g_stackDictionary.getRatio(), sizeof(g_stackDictionary) / 1024.f / 1024.f);
					ImGui::Text("Alloc dictionary : %0.2f iterations per search | filled : %0.2f%% (LMT_ALLOC_DICTIONARY_SIZE) | %0.2f Mo", g_allocDictionary.getHitStats(), g_allocDictionary.getRatio(), sizeof(g_allocDictionary) / 1024.f / 1024.f);
					ImGui::Text("Tree dictionary  : %0.2f iterations per search | filled : %0.2f%% (LMT_TREE_DICTIONARY_SIZE)  | %0.2f Mo", g_treeDictionary.getHitStats(), g_treeDictionary.getRatio(), sizeof(g_treeDictionary) / 1024.f / 1024.f);
					ImGui::Separator();
#endif
					ImGui::TextWrapped("Note that dictionaries are allocated at init and are not resizable. If you enable LMT_DEBUG_DEV a full dictionary will trigger an assert, if not execution will continue but statistics can be corrupted.\nThe more the dictionary is full, the more the number of iterations increase when searching into dictionary, a 90%% full dictionary is a bad idea.");
					ImGui::EndTooltip();
				}
				ImGui::PopItemWidth();
				ImGui::EndMenuBar();
			}
		}

		void render(float dt)
		{
			g_updateSearch = false;
			g_updateRatio += dt;
			bool second = false;
			if (ImGui::Begin("LiveMemoryProfiler", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar))
			{
				renderMenu();
				if (g_updateRatio >= 0.333f)
				{
					if (g_refeshAuto)
					{
						cacheData();
						g_updateType = UpdateType::CURRENT_FRAME;
					}
					g_updateRatio = 0.f;
					second = true;
				}
				ImGui::Separator();

				std::lock_guard<std::mutex> lock(g_mutex);
				if (g_updateSearch)
				{
					if (strlen(g_searchStr) > 0)
					{
						searchAlloc();
					}
				}

				if (g_displayType == DisplayType::CALLEE)
				{
					renderCallees();
				}
				else if (g_displayType == DisplayType::HISTOGRAMS)
				{
					renderHistograms();
				}
				else if (g_displayType == DisplayType::STACK)
				{
					renderStack();
				}
				else if (g_displayType == DisplayType::FUNCTION)
				{
					renderFunctionView();
				}
			}
			ImGui::End();

			if (second)
			{
				std::lock_guard<std::mutex> lock(g_mutex);
				for (auto &h : g_histograms)
				{
					if (h.isFunction)
					{
						h.allocSizeCache = h.function->allocSize;
						h.allocSize[h.cursor] = h.allocSizeCache;
					}
					else
					{
						h.allocSizeCache = h.call->allocSize;
						h.allocSize[h.cursor] = h.allocSizeCache;
					}
					h.cursor = (h.cursor + 1) % HISTORY_FRAME_NUMBER;
				}
			}

			if (g_updateType == UpdateType::CURRENT_AND_NEXT_FRAME)
				g_updateType = UpdateType::CURRENT_FRAME;
			else if (g_updateType == UpdateType::CURRENT_FRAME)
				g_updateType = UpdateType::NONE;
		}
	}
}

void LiveMemTracer::display(float dt)
{
	INTERNAL_SCOPE;
	Renderer::render(dt);
}
#else
void LiveMemTracer::display(float dt) {}
#endif

#endif // LMT_IMPL
#endif // LMT_ENABLED