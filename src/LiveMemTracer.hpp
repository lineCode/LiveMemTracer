#pragma once

#if defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__)
#define LMT_PLATFORM_WINDOWS
#elif defined(__clang__)
#define LMT_PLATFORM_ORBIS
#endif

#ifndef LMT_ENABLED
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

#else // LMT_ENABLED

#define LMT_ALLOC(size)::LiveMemTracer::alloc(size)
#define LMT_ALLOC_ALIGNED(size, alignment)::LiveMemTracer::allocAligned(size, alignment)
#define LMT_DEALLOC(ptr)::LiveMemTracer::dealloc(ptr)
#define LMT_DEALLOC_ALIGNED(ptr)::LiveMemTracer::deallocAligned(ptr)
#define LMT_REALLOC(ptr, size)::LiveMemTracer::realloc(ptr, size)
#define LMT_REALLOC_ALIGNED(ptr, size, alignment)::LiveMemTracer::reallocAligned(ptr, size, alignment)
#define LMT_DISPLAY(dt)::LiveMemTracer::display(dt)
#define LMT_EXIT()::LiveMemTracer::exit()
#define LMT_INIT()::LiveMemTracer::SymbolGetter::init(); ::LiveMemTracer::init()
#define LMT_FLUSH()::LiveMemTracer::getChunk(true)

#ifdef LMT_IMPL

#include <atomic>     //std::atomic
#include <cstdlib>    //malloc etc...
#include <vector>
#include <algorithm>
#include <mutex>

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

#ifndef LMT_ASSERT
#define LMT_ASSERT(condition, message) assert(condition)
#endif

#ifndef LMT_TREAT_CHUNK
#define LMT_TREAT_CHUNK(chunk) LiveMemTracer::treatChunk(chunk)
#endif

#ifndef LMT_DEBUG_DEV
#define LMT_DEBUG_ASSERT(condition, message) do{}while(0)
#else
#define LMT_DEBUG_ASSERT(condition, message) assert(condition && message)
#endif

#ifndef LMT_IMPLEMENTED
#define LMT_IMPLEMENTED 1
#else
static_assert(false, "LMT is already implemented, do not define LMT_IMPL more than once.");
#endif

#if !defined(LMT_PLATFORM_WINDOWS) && !defined(LMT_PLATFORM_ORBIS)
static_assert(false, "You have to define platform. Only Orbis and Windows are supported for now.");
#endif

#endif

#if defined(LMT_PLATFORM_WINDOWS) 
#undef LMT_TLS
#undef LMT_INLINE
#define LMT_TLS __declspec(thread)
#define LMT_INLINE __forceinline
#elif defined(LMT_PLATFORM_ORBIS)
#undef LMT_TLS
#undef LMT_INLINE
#define LMT_TLS __thread
#define LMT_INLINE __attribute__((__always_inline__))
#else
#define LMT_TLS
#define LMT_INLINE
#endif

#include <stdint.h>

namespace LiveMemTracer
{
	typedef size_t Hash;

	void *alloc(size_t size);
	void *allocAligned(size_t size, size_t alignment);
	void dealloc(void *ptr);
	void deallocAligned(void *ptr);
	void *realloc(void *ptr, size_t size);
	void *reallocAligned(void *ptr, size_t size, size_t alignment);
	void exit();
	void init();
	void display(float dt);
	struct Chunk;
	Chunk *getChunk(bool forceFlush = false);

	namespace SymbolGetter
	{
		void init();
	}
#ifdef LMT_IMPL
	static const size_t ALLOC_NUMBER_PER_CHUNK = LMT_ALLOC_NUMBER_PER_CHUNK;
	static const size_t STACK_SIZE_PER_ALLOC = LMT_STACK_SIZE_PER_ALLOC;
	static const size_t CHUNK_NUMBER = LMT_CHUNK_NUMBER_PER_THREAD;
	static const size_t CACHE_SIZE = LMT_CACHE_SIZE;
	static const size_t ALLOC_DICTIONARY_SIZE = LMT_ALLOC_DICTIONARY_SIZE;
	static const size_t STACK_DICTIONARY_SIZE = LMT_STACK_DICTIONARY_SIZE;
	static const size_t TREE_DICTIONARY_SIZE = LMT_TREE_DICTIONARY_SIZE;
	static const size_t INTERNAL_MAX_STACK_DEPTH = 255;
	static const size_t INTERNAL_FRAME_TO_SKIP = 3;
	static const char  *TRUNCATED_STACK_NAME = "Truncated\0";
	static const char  *UNKNOWN_STACK_NAME = "Unknown\0";
	static const size_t HISTORY_FRAME_NUMBER = 120;
	template <class T> LMT_INLINE size_t combineHash(const T& val, const size_t baseHash = 2166136261U);
	static uint32_t getCallstack(size_t maxStackSize, void **stack, Hash *hash);
#endif
}

#if defined(LMT_PLATFORM_WINDOWS)
#include "LiveMemTracer_Windows.hpp"
#elif defined(LMT_PLATFORM_ORBIS)
#include "LiveMemTracer_Orbis/LiveMemTracer_Orbis.hpp"
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
		int64_t       allocSize[ALLOC_NUMBER_PER_CHUNK];
		Hash          allocHash[ALLOC_NUMBER_PER_CHUNK];
		size_t        allocStackIndex[ALLOC_NUMBER_PER_CHUNK];
		uint8_t       allocStackSize[ALLOC_NUMBER_PER_CHUNK];
		void         *stackBuffer[ALLOC_NUMBER_PER_CHUNK * STACK_SIZE_PER_ALLOC];
		size_t        allocIndex;
		size_t        stackIndex;
		Chunk         *next;
		std::atomic<ChunkStatus> status;
	};

	struct Edge;

	struct Alloc
	{
		int64_t counter;
		int64_t lastCount;
		const char *str;
		Alloc *next;
		Alloc *shared;
		Edge  *edges;
		Alloc() : counter(0), lastCount(0), str(nullptr), next(nullptr), shared(nullptr), edges(nullptr) {}
	};

	struct AllocStack
	{
		Hash hash;
		int64_t counter;
		Alloc *stackAllocs[STACK_SIZE_PER_ALLOC];
		uint8_t stackSize;
		AllocStack() : hash(0), counter(0), stackSize(0) {}
	};

	struct Edge
	{
		int64_t count;
		Alloc *alloc;
		std::vector<Edge*> to;
		Edge *from;
		Edge *same;
		int64_t lastCount;
		Edge() : count(0), alloc(nullptr), from(nullptr), same(nullptr), lastCount(0) {}
	};

	template <typename Key, typename Value, size_t Capacity>
	class Dictionary
	{
	public:
		static const size_t   HASH_EMPTY = Hash(-1);
		static const size_t   HASH_INVALID = Hash(-2);

		Dictionary()
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
			const size_t hash = getHash(key);
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
#ifdef LMT_DICTIONARY_STATS
				_size.fetch_add(1);
#endif
			}
			return pair;
		}

#ifdef LMT_DICTIONARY_STATS
		LMT_INLINE float getHitStats() const
		{
			size_t total = _hitTotal;
			size_t count = _hitCount;
			return _hitTotal / float(_hitCount);
		}

		LMT_INLINE float getRatio() const
		{
			size_t size = _size;
			return size / float(Capacity) * 100.f;
		}
#endif
	private:
		Pair _buffer[Capacity];
		static const size_t HASH_MODIFIER = 7;
#ifdef LMT_DICTIONARY_STATS
		mutable std::atomic_size_t _hitCount;
		mutable std::atomic_size_t _hitTotal;
		mutable std::atomic_size_t _size;
#endif

		LMT_INLINE size_t getHash(Key key) const
		{
			for (size_t i = 0; i < Capacity; ++i)
			{
				const size_t realHash = (key + i) % Capacity;
				if (_buffer[realHash]._hash == HASH_EMPTY || _buffer[realHash]._key == key)
				{
#ifdef LMT_DICTIONARY_STATS
					_hitCount += 1;
					_hitTotal += i;
#endif
					return realHash;
				}
			}
#ifdef LMT_DICTIONARY_STATS
			_hitTotal += Capacity - 1;
#endif
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
	LMT_TLS static Chunk                     g_th_chunks[CHUNK_NUMBER];
	LMT_TLS static uint8_t                   g_th_chunkIndex = 0;
	LMT_TLS static Hash                      g_th_cache[CACHE_SIZE];
	LMT_TLS static uint8_t                   g_th_cacheIndex = 0;
	LMT_TLS static bool                      g_th_initialized = false;


	static Alloc                                               *g_allocList = nullptr;

	static Dictionary<Hash, AllocStack, STACK_DICTIONARY_SIZE>  g_allocStackRefTable;
	static Dictionary<Hash, Alloc, ALLOC_DICTIONARY_SIZE>       g_allocRefTable;
	static Dictionary<size_t, Edge, TREE_DICTIONARY_SIZE>       g_tree;

	static std::vector<Edge*>                                   g_allocStackRoots;
	static std::mutex                                           g_mutex;

	static const size_t                                         g_internalPerThreadMemoryUsed =
		sizeof(g_th_chunks)
		+ sizeof(g_th_chunkIndex)
		+ sizeof(g_th_cache)
		+ sizeof(g_th_initialized)
		+ sizeof(g_th_cacheIndex);

	static const size_t                                         g_internalSharedMemoryUsed =
		sizeof(g_allocStackRefTable)
		+ sizeof(g_allocRefTable)
		+ sizeof(g_allocList)
		+ sizeof(g_tree)
		+ sizeof(g_allocStackRoots)
		+ sizeof(g_mutex)
		+ sizeof(g_internalPerThreadMemoryUsed)
		+ sizeof(size_t) /* itself */;

	static std::atomic_size_t                                   g_internalAllThreadsMemoryUsed =
#if defined(LMT_PLATFORM_WINDOWS)
		g_internalSharedMemoryUsed;
#elif defined(LMT_PLATFORM_ORBIS)
	{g_internalSharedMemoryUsed};
#endif

	static std::atomic<RunningStatus>                           g_runningStatus =
#if defined(LMT_PLATFORM_WINDOWS)
		RunningStatus::NOT_INITIALIZED;
#elif defined(LMT_PLATFORM_ORBIS)
	{RunningStatus::NOT_INITIALIZED};
#endif

	static std::atomic_size_t                                   g_temporaryChunkCounter =
#if defined(LMT_PLATFORM_WINDOWS)
		0;
#elif defined(LMT_PLATFORM_ORBIS)
	{0};
#endif


	namespace Renderer
	{
		struct Histogram
		{
			Alloc *function;
			Edge  *call;
			const char *name;
			bool  isFunction;
			int64_t count[HISTORY_FRAME_NUMBER];
			size_t  cursor;
			int64_t currentCount;
			Histogram() : function(nullptr), call(nullptr), name(nullptr), isFunction(false), cursor(0), currentCount(0)
			{
				memset(count, 0, sizeof(count));
			}
		};

		enum DisplayType : int
		{
			//CALLER = 0,
			CALLEE = 0,
			STACK,
			HISTOGRAMS,
			END
		};

		static const char *DisplayTypeStr[] =
		{
			//"Caller",
			"Callee",
			"Stack",
			"Histograms"
		};

		static bool                                g_refeshAuto = true;
		static bool                                g_refresh = false;
		static bool                                g_updateSearch = false;
		static float                               g_updateRatio = 0.f;
		static const size_t                        g_search_str_length = 1024;
		static char                                g_searchStr[g_search_str_length];
		static DisplayType                         g_displayType = DisplayType::CALLEE;
		static std::vector<Histogram>              g_histograms;
		static Alloc                              *g_searchResult;

		bool searchAlloc();
		void displayCallee(Edge *callee, bool callerTooltip);
		void renderCallees();
		void renderMenu();
		void renderHistograms();
		void renderStack();
		void createHistogram(Alloc *function);
		void createHistogram(Edge  *functionCall);
		void display(float dt);
	}

	static bool chunkIsNotFull(const Chunk *chunk);
	static Chunk *createTemporaryChunk();
	static Chunk *createPreallocatedChunk(const RunningStatus status);
	static uint8_t findInCache(Hash hash);
	static void logAllocInChunk(Header *header, size_t size);
	static void logFreeInChunk(Header *header);
	static void treatChunk(Chunk *chunk);
	static void updateTree(AllocStack &alloc, int64_t size, bool checkTree);
}
#endif

#ifdef LMT_IMPL
namespace LiveMemTracer
{
#define GET_HEADER(ptr) (Header*)((void*)((size_t)ptr - HEADER_SIZE))
#define GET_ALIGNED_PTR(ptr) (void*)(*(size_t*)((void*)(size_t(ptr) - ALIGNED_HEADER_SIZE)))
#define GET_ALIGNED_SIZE(size, alignment) size + --alignment + ALIGNED_HEADER_SIZE
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
}

void *LiveMemTracer::alloc(size_t size)
{
	void *ptr = LMT_USE_MALLOC(size + HEADER_SIZE);
	LMT_ASSERT(ptr != nullptr, "Out of memory");
	if (!ptr)
		return nullptr;
	Header *header = (Header*)(ptr);
	logAllocInChunk(header, size);
	header->aligned = 0;
	return (void*)(size_t(ptr) + HEADER_SIZE);
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
	void *newPtr = LMT_USE_REALLOC((void*)header, size + HEADER_SIZE);
	LMT_ASSERT(newPtr != nullptr, "Out of memory");
	if (!newPtr)
		return newPtr;
	header = (Header*)(newPtr);
	logAllocInChunk(header, size);
	header->aligned = 0;
	return (void*)(size_t(newPtr) + HEADER_SIZE);
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
	memcpy(newPtr, ptr, oldHeader.size < size ? oldHeader.size : size);
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
	LMT_USE_FREE((void*)header);
}

void LiveMemTracer::deallocAligned(void *ptr)
{
	if (ptr == nullptr)
		return;
	Header *header = GET_HEADER(ptr);
	LMT_DEBUG_ASSERT(header->aligned == 1, "Trying to free an non-aligned ptr with an aligned free");
	logFreeInChunk(header);
	LMT_USE_FREE(GET_ALIGNED_PTR(ptr));
}

void LiveMemTracer::exit()
{
	g_runningStatus = EXIT;
}

void LiveMemTracer::init()
{
	g_runningStatus = RUNNING;
}


//////////////////////////////////////////////////////////////////////////
// IMPL ONLY : 
//////////////////////////////////////////////////////////////////////////



bool LiveMemTracer::chunkIsNotFull(const Chunk *chunk)
{
	return (chunk
		&& chunk->allocIndex < ALLOC_NUMBER_PER_CHUNK
		&& chunk->stackIndex < ALLOC_NUMBER_PER_CHUNK * STACK_SIZE_PER_ALLOC);
}

LiveMemTracer::Chunk *LiveMemTracer::createTemporaryChunk()
{
	void *ptr = LMT_USE_MALLOC(sizeof(Chunk));
	LMT_ASSERT(ptr != nullptr, "Out of memory");
	if (ptr == nullptr)
		return nullptr;
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
		&& g_th_chunkIndex + 1 >= CHUNK_NUMBER)
	{
		return nullptr;
	}

	// We get the next preallocated chunk
	g_th_chunkIndex = (g_th_chunkIndex + 1) % CHUNK_NUMBER;
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
	Chunk *currentChunk = nullptr;
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
		i = CACHE_SIZE - 1;
	uint8_t res = 1;
	while (true)
	{
		if (g_th_cache[i] == hash)
		{
			return res - 1;
		}
		i -= 1;
		if (i < 0)
			i = CACHE_SIZE - 1;
		if (++res == CACHE_SIZE)
			break;
	}
	return uint8_t(-1);
}

void LiveMemTracer::logAllocInChunk(LiveMemTracer::Header *header, size_t size)
{
	Chunk *chunk = getChunk();

	header->hash = 0;
	void **stack = &chunk->stackBuffer[chunk->stackIndex];
	uint32_t count = getCallstack(STACK_SIZE_PER_ALLOC, stack, &header->hash);

	header->size = size;

	size_t index = chunk->allocIndex;
	uint8_t found = findInCache(header->hash);
	if (found != uint8_t(-1))
	{
		index = chunk->allocIndex - found - 1;
		chunk->allocSize[index] += size;
		return;
	}

	chunk->allocStackIndex[index] = chunk->stackIndex;
	chunk->allocSize[index] = size;
	chunk->allocHash[index] = header->hash;
	chunk->allocStackSize[index] = count;
	g_th_cache[g_th_cacheIndex] = header->hash;
	g_th_cacheIndex = (g_th_cacheIndex + 1) % CACHE_SIZE;
	chunk->allocIndex += 1;
	chunk->stackIndex += count;
}

void LiveMemTracer::logFreeInChunk(LiveMemTracer::Header *header)
{
	Chunk *chunk = getChunk();

	size_t index = chunk->allocIndex;
	uint8_t found = findInCache(header->hash);
	if (found != uint8_t(-1))
	{
		index = chunk->allocIndex - found - 1;
		chunk->allocSize[index] -= header->size;
		return;
	}

	g_th_cache[g_th_cacheIndex] = header->hash;
	g_th_cacheIndex = (g_th_cacheIndex + 1) % CACHE_SIZE;
	chunk->allocStackIndex[index] = -1;
	chunk->allocSize[index] = -int64_t(header->size);
	chunk->allocHash[index] = header->hash;
	chunk->allocStackSize[index] = 0;
	chunk->allocIndex += 1;
}

void LiveMemTracer::treatChunk(Chunk *chunk)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	for (size_t i = 0, iend = chunk->allocIndex; i < iend; ++i)
	{
		auto size = chunk->allocSize[i];
		if (size == 0)
			continue;
		auto it = g_allocStackRefTable.update(chunk->allocHash[i]);
		auto &allocStack = it->getValue();
		if (allocStack.stackSize != 0)
		{
			allocStack.counter += size;
			updateTree(allocStack, size, false);
			for (size_t j = 0; j < allocStack.stackSize; ++j)
			{
				allocStack.stackAllocs[j]->counter += size;
			}
			continue;
		}
		allocStack.counter = size;
		allocStack.hash = chunk->allocHash[i];

		for (size_t j = 0, jend = chunk->allocStackSize[i]; j < jend; ++j)
		{
			void *addr = chunk->stackBuffer[chunk->allocStackIndex[i] + j];
			auto found = g_allocRefTable.update(size_t(addr));
			if (found->getValue().shared != nullptr)
			{
				auto shared = found->getValue().shared;
				shared->counter += size;
				allocStack.stackAllocs[j] = shared;
				continue;
			}
			if (found->getValue().str != nullptr)
			{
				allocStack.stackAllocs[j] = &found->getValue();
				found->getValue().counter += size;
				continue;
			}
			void *absoluteAddress = nullptr;
			const char *name = SymbolGetter::getSymbol(addr, absoluteAddress);

			auto shared = g_allocRefTable.update(size_t(absoluteAddress));
			if (shared->getValue().str != nullptr)
			{
				found->getValue().shared = &shared->getValue();
				allocStack.stackAllocs[j] = &shared->getValue();
				shared->getValue().counter += size;
#ifdef LMT_PLATFORM_WINDOWS
				if (name != TRUNCATED_STACK_NAME)
					LMT_USE_FREE((void*)name);
#endif
				continue;
			}

			found->getValue().shared = &shared->getValue();
			shared->getValue().str = name;
			shared->getValue().counter = size;
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
		g_temporaryChunkCounter.fetch_sub(1);
	}
	else
	{
		chunk->status.store(ChunkStatus::TREATED);
	}
}

void LiveMemTracer::updateTree(AllocStack &allocStack, int64_t size, bool checkTree)
{
	int stackSize = allocStack.stackSize;
	stackSize -= INTERNAL_FRAME_TO_SKIP;
	uint8_t depth = 0;
	Edge *previousPtr = nullptr;
	while (stackSize >= 0)
	{
		Hash currentHash = size_t(allocStack.stackAllocs[stackSize]);
		currentHash = combineHash((size_t(previousPtr)), currentHash);
		currentHash = combineHash(depth * depth, currentHash);

		Edge *currentPtr = &g_tree.update(currentHash)->getValue();
		currentPtr->count += size;
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
				auto it = std::find(std::begin(previousPtr->to), std::end(previousPtr->to), currentPtr);
				if (it == std::end(previousPtr->to))
				{
					previousPtr->to.push_back(currentPtr);
				}
				currentPtr->from = previousPtr;
			}
			else
			{
				auto it = find(std::begin(g_allocStackRoots), std::end(g_allocStackRoots), currentPtr);
				if (it == std::end(g_allocStackRoots))
					g_allocStackRoots.push_back(currentPtr);
			}
		}
		LMT_DEBUG_ASSERT(strcmp(currentPtr->alloc->str, allocStack.stackAllocs[stackSize]->str) == 0, "Name collision.");

		previousPtr = currentPtr;
		++depth;
		--stackSize;
	}
}

template <class T>
LMT_INLINE size_t LiveMemTracer::combineHash(const T& val, const size_t baseHash)
{
	const uint8_t* bytes = (uint8_t*)&val;
	const size_t count = sizeof(val);
	const size_t FNV_offset_basis = baseHash;
	const size_t FNV_prime = 16777619U;

	size_t hash = FNV_offset_basis;
	for (size_t i = 0; i < count; ++i)
	{
		hash ^= (size_t)bytes[i];
		hash *= FNV_prime;
	}

	return hash;
}

#ifdef LMT_IMGUI
#include LMT_IMGUI_INCLUDE_PATH

namespace LiveMemTracer
{
	namespace Renderer
	{
		float formatMemoryString(int64_t sizeInBytes, const char *&str)
		{
			if (sizeInBytes < 0)
			{
				str = "b";
				return float(sizeInBytes);
			}
			if (sizeInBytes < 10 * 1024)
			{
				str = "b";
				return float(sizeInBytes);
			}
			else if (sizeInBytes < 10 * 1024 * 1024)
			{
				str = "Kb";
				return sizeInBytes / 1024.f;
			}
			else
			{
				str = "Mb";
				return sizeInBytes / 1024 / 1024.f;
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

		void displayCallee(Edge *callee, bool callerTooltip)
		{
			if (!callee)
				return;

			if (g_refresh)
			{
				callee->lastCount = callee->count;
			}

			ImGui::PushID(callee);
			const char *suffix;
			float size = formatMemoryString(callee->lastCount, suffix);
			auto cursorPos = ImGui::GetCursorPos();
			const bool opened = ImGui::TreeNode(callee, "%f %s", size, suffix);
			cursorPos.x += 150;
			ImGui::SetCursorPos(cursorPos);
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
				}
				ImGui::EndPopup();
			}
			if (opened)
			{
				if (g_refresh)
				{
					std::sort(std::begin(callee->to), std::end(callee->to), [](Edge *a, Edge *b){ return a->lastCount > b->lastCount; });
				}
				for (auto &to : callee->to)
					displayCallee(to, callerTooltip);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		void renderCallees()
		{
			ImGui::Separator();
			ImGui::Text("Size"); ImGui::SameLine(150);
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
				if (g_refresh)
				{
					callee->lastCount = callee->counter;
				}
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
				bool opened = ImGui::CollapsingHeader("##dummy", "Coucou", true, false);
				if (ImGui::BeginPopupContextItem("Options"))
				{
					if (ImGui::Selectable("Watch function"))
					{
						createHistogram(callee);
					}
					ImGui::EndPopup();
				}

				cursorPos.x += 25;
				cursorPos.y += 3;
				const char *suffix;
				float size = formatMemoryString(callee->lastCount, suffix);
				ImGui::SetCursorPos(cursorPos);
				ImGui::Text("%f %s", size, suffix);
				cursorPos.x += 150 - 25;
				ImGui::SetCursorPos(cursorPos);
				ImGui::Text("%s", callee->str);
				if (opened)
				{
					Edge *edge = callee->edges;
					while (edge)
					{
						displayCallee(edge, true);
						edge = edge->same;
					}
				}
				ImGui::PopID();
				ImGui::PopStyleColor(3);
				callee = callee->next;
			}
			ImGui::EndChild();
		}

		void renderHistograms()
		{
			const int columnNumber = 3;
			int i = 0;
			float histoW = ImGui::GetWindowContentRegionWidth() / columnNumber;
			ImVec2 graphSize(histoW, histoW * 0.4f);
			auto it = std::begin(g_histograms);
			while (it != std::end(g_histograms))
			{
				auto &h = *it;
				if (g_searchStr[0] && LMT_STRSTRI(h.name, g_searchStr) == nullptr)
				{
					++it;
					continue;
				}

				bool toDelete = false;
				ImGui::PushID(i);
				ImGui::PushItemWidth(histoW);
				ImGui::BeginGroup();
				ImGui::BeginGroup();
				ImGui::TextColored(h.isFunction ? ImColor(217, 164, 22) : ImColor(209, 6, 145), "%s", h.isFunction ? "Function" : "Call"); ImGui::SameLine();
				const char *suffix;
				float size = formatMemoryString(h.currentCount, suffix);
				ImGui::Text("%f %s", size, suffix);
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
				ImGui::PlotLines("##NoName", (const float*)h.count, HISTORY_FRAME_NUMBER, h.cursor, nullptr, 0, FLT_MAX, graphSize, sizeof(int64_t));
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
					float ttSize = formatMemoryString(h.count[itemIndex], ttSuffix);
					ImGui::Text("%f %s", ttSize, ttSuffix);
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
					it = g_histograms.erase(it);
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
			g_histograms.resize(g_histograms.size() + 1);
			auto &last = g_histograms.back();
			last.function = function;
			last.name = function->str;
			last.isFunction = true;
		}

		void createHistogram(Edge *functionCall)
		{
			for (auto &h : g_histograms)
			{
				if (h.call == functionCall)
					return;
			}
			g_histograms.resize(g_histograms.size() + 1);
			auto &last = g_histograms.back();
			last.call = functionCall;
			last.name = functionCall->alloc->str;
			last.isFunction = false;
		}

		void renderStack()
		{
			ImGui::Separator();
			ImGui::Text("Size"); ImGui::SameLine(150);
			ImGui::Text("Callee");
			ImGui::Separator();
			ImGui::BeginChild("Content", ImGui::GetWindowContentRegionMax(), false, ImGuiWindowFlags_HorizontalScrollbar);
			for (auto &root : g_allocStackRoots)
			{
				displayCallee(root, false);
			}
			ImGui::EndChild();
		}

		void renderMenu()
		{
			if (ImGui::BeginMenuBar())
			{
				ImGui::PushItemWidth(110.f);
				ImGui::Combo("##ComboMode", (int*)&g_displayType, DisplayTypeStr, DisplayType::END); ImGui::SameLine();
				ImGui::Checkbox("Refresh auto", &g_refeshAuto);
				if (!g_refeshAuto)
				{
					ImGui::SameLine();
					g_refresh = ImGui::Button("Refresh");
				}
				if (g_displayType != DisplayType::STACK)
				{
					ImGui::SameLine();
					if (ImGui::InputText("Search", g_searchStr, g_search_str_length))
					{
						g_updateSearch = true;
					}
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
					ImGui::Text("Memory used by LMT : %06f Mo\n", float(g_internalAllThreadsMemoryUsed.load()) / 1024.f / 1024.f);
#ifdef LMT_DICTIONARY_STATS
					ImGui::Text("allocStackRefTable %f | %f%%", g_allocStackRefTable.getHitStats(), g_allocStackRefTable.getRatio());
					ImGui::Text("allocRefTable %f | %f%%", g_allocRefTable.getHitStats(), g_allocRefTable.getRatio());
					ImGui::Text("tree %f | %f%%", g_tree.getHitStats(), g_tree.getRatio());
#endif
					ImGui::EndTooltip();
				}
				ImGui::PopItemWidth();
				ImGui::EndMenuBar();
			}
		}

		void render(float dt)
		{
			g_refresh = false;
			g_updateSearch = false;
			g_updateRatio += dt;
			if (ImGui::Begin("LiveMemoryProfiler", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar))
			{
				ImGui::CaptureMouseFromApp();
				ImGui::CaptureKeyboardFromApp();

				renderMenu();
				if (g_refeshAuto && g_updateRatio >= 1.f)
				{
					g_refresh = true;
					g_updateRatio = 0.f;
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
			}
			ImGui::End();

			if (g_refresh)
			{
				std::lock_guard<std::mutex> lock(g_mutex);
				for (auto &h : g_histograms)
				{
					if (h.isFunction)
					{
						h.currentCount = h.function->counter;
						h.count[h.cursor] = h.currentCount;
					}
					else
					{
						h.currentCount = h.call->count;
						h.count[h.cursor] = h.currentCount;
					}
					h.cursor = (h.cursor + 1) % HISTORY_FRAME_NUMBER;
				}
			}
		}
	}
}

void LiveMemTracer::display(float dt)
{
	Renderer::render(dt);
}
#else
void LiveMemTracer::display(float dt) {}
#endif

#endif // LMT_IMPL
#endif // LMT_ENABLED