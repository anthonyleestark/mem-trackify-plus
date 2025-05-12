/*
 * smart-gc.h 
 * A very simple C++ Smart Garbage Collection Library
 * Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * Released under MIT License
 * 
 */

#ifndef _SMART_GARBAGE_COLLECTION_CPP_INCLUDED_
#define _SMART_GARBAGE_COLLECTION_CPP_INCLUDED_

#pragma once


// ===========================================================
// Include header file and dependencies
// ===========================================================

#include <iostream>
#include <sstream>

#include <new>			// for std::bad_alloc

#ifdef SMART_GC_THREADSAFETY
#include <mutex>
#endif // SMART_GC_THREADSAFETY

#include <atomic>
#include <vector>
#include <unordered_map>


// ================================================================================
// Macros definition for usage
// ================================================================================

/*
 * NOTE: In order to use Smart Garbage Collection library or to enable/disable
 *		 specific modes, define these corresponding macros somewhere in your code.
 *		 Please read this carefully before using any of these macros and
 *		 importing Smart Garbage Collection feature for your C++ program.
 * 
 * HOW TO USE:
 *
 *   SMART_GC_DEBUG
 *		- Enable debugging mode.
 *		- With this macro, you can track Debugging information (filename, line number) 
 *		  for allocation/deallocation.
 *
 *   SMART_GC_THREADSAFETY
 *		- Ensure thread-safety when tracking allocation/deallocation.
 *		- If your program has no multi-threads, you can skip this macro.
 *
 *   SMART_GC_CONSOLE_REPORT_ON_TERMINATION
 *		- Display memory leak report and garbage collecting progress on program termination.
 *		- Only enable this macro if you're using a Console Application.
 *
 *   SMART_GC_NOTOVERRIDE_GLOBAL_NEW
 *   SMART_GC_NOTOVERRIDE_GLOBAL_DELETE
 *		- By default, this library has overriden the global new/delete operators.
 *      - In some cases, this may conflicts some of your other libraries, modules,
 *		  or maybe part of your program architecture, or may cause unexpected behaviors.
 *		- We also provided some methods as replacements for default new/delete operators,
 *		  use them instead if you want to ensure safety for your program.
 *		- Define this macros if you want to disable the default overidden global new/delete operators.
 * 
 *	 smart_new
 *   smart_delete
 *		Use these macros to ensure using this libaray's overriden smart new/delete operators.
 *		Example:
 *			int* ptr = smart_new int[100];
 *			smart_delete ptr;
 * 
 *	 debug_new
 *   debug_delete
 *		- Use these macros to ensure using debug version of this libaray's overriden smart new/delete operators.
 *		- These macros only work if SMART_GC_DEBUG is defined and enabled.
 *		Example:
 *			int* ptr = debug_new int[100];
 *			debug_delete ptr;
 *   
 */

/*
 * Example:
 *	 #define SMART_GC_ENABLED
 *	 #define SMART_GC_DEBUG
 *	 #define SMART_GC_THREADSAFETY
 *	 #define SMART_GC_CONSOLE_REPORT_ON_TERMINATION
 * 
 */


// ================================================================================
// Class/struct declaration
// ================================================================================

// Allocation tracking and leak detection
class SmartGarbageCollector 
{
public:
	struct GCDebugInfo {
		const char* _file;
		int			_line;
	};
	struct AllocInfo {			// Struct to hold allocation information
		size_t		_size;
		bool		_is_array;
#ifdef SMART_GC_DEBUG
		GCDebugInfo _debug_info;
#endif // SMART_GC_DEBUG
	};

private:
	using Address			= typename void*;
	using StringData		= typename std::string;
	using StringStreamData	= typename std::ostringstream;
	using AtomicFlag		= typename std::atomic<bool>;
	using AllocTrackObj		= typename std::pair<Address, AllocInfo>;
	using AllocTrackData	= typename std::unordered_map<Address, AllocInfo>;
	using LeakReport		= typename std::vector<StringData>;

#ifdef SMART_GC_THREADSAFETY
	using MutexObj			= typename std::recursive_mutex;
	using MutexLockGuard	= typename std::lock_guard<MutexObj>;
#endif // SMART_GC_THREADSAFETY

public:
	SmartGarbageCollector();
	~SmartGarbageCollector();

public:
	// Define static functions for smart allocation/deallocation
#ifndef SMART_GC_DEBUG
	static void* gcSmartAlloc(size_t size, bool isArray);
#else
	static void* gcSmartAlloc(size_t size, const char* file, int line, bool isArray);
#endif // !SMART_GC_DEBUG
	static void gcSmartFree(void* ptr, bool isArray) noexcept;
	static inline void gcSmartDealloc(void* ptr, bool isArray) noexcept { gcSmartFree(ptr, isArray); };

private:
	// Request memory allocation and store debug tracking info
	void* gcAlloc(size_t size, const char* file, int line, bool isArray);

	// Request memory deallocation and clear the pointer debug tracking info
	void gcDealloc(void* ptr, bool isArray) noexcept;

public:
	// Get tracking info
	size_t gcGetTrackerSize(void) const;		// size in bytes
	size_t gcGetMemorySize(void) const;		// size in bytes
	size_t gcGetPtrCount(void) const;

	// Report leak memory tracking data
	bool gcIsMemoryLeak(void) const;
	LeakReport gcGetLeakReport(void) const noexcept;
	void gcPrintLeakInfo(std::ostream& os) const noexcept;

private:
	// No copyable
	SmartGarbageCollector(const SmartGarbageCollector&) = delete;
	SmartGarbageCollector& operator=(const SmartGarbageCollector&) = delete;

	// No movable
	SmartGarbageCollector(const SmartGarbageCollector&&) = delete;
	SmartGarbageCollector& operator=(const SmartGarbageCollector&&) = delete;

private:
	// Attributes
	AllocTrackData		_allocTrackData;				// Stores all allocation info
	AtomicFlag			_isTrackerInitialized = false;	// Check if the tracker finished initializing
#ifdef SMART_GC_THREADSAFETY
	mutable MutexObj	_Mymutex;						// Ensures thread-safety
#endif // SMART_GC_THREADSAFETY

private:
	// Ensure gcAlloc() function run correctly
	class GCAllocGuard {
	public:
		// Construction
		GCAllocGuard(bool& flag) : _Myflag(flag) { _Myflag = true; }
		~GCAllocGuard() { _Myflag = false; }

	private:
		bool& _Myflag;
	};
};


// ================================================================================
// Global tracker declaration
// ================================================================================

// Global object to handle memory allocation tracking and leak detection
extern SmartGarbageCollector __g_gcSmartGarbageCollector;
inline SmartGarbageCollector* gcGetSmartGarbageCollector(void) { return &__g_gcSmartGarbageCollector; }


// ================================================================================
// Override global new/delete operators
// ================================================================================

#ifndef SMART_GC_NOTOVERRIDE_GLOBAL_NEW
#ifndef SMART_GC_DEBUG

#ifndef __CRTDECL
#define __CRTDECL	__cdecl
#endif

// Scalar new
#ifdef _MSC_VER
#pragma warning(disable:4595)
_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#endif // !_MSC_VER
inline void* __CRTDECL operator new(std::size_t size) {
	return SmartGarbageCollector::gcSmartAlloc(size, false);
}

// Array new
#ifdef _MSC_VER
#pragma warning(disable:4595)
_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#endif // !_MSC_VER
inline void* __CRTDECL operator new[](std::size_t size) {
	return SmartGarbageCollector::gcSmartAlloc(size, true);
}

#else
// Scalar new
#ifdef _MSC_VER
#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void* operator new(size_t size, const char* file, int line) {
	return SmartGarbageCollector::gcSmartAlloc(size, file, line, false);
}

// Array new
#ifdef _MSC_VER
#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void* operator new[](size_t size, const char* file, int line) {
	return SmartGarbageCollector::gcSmartAlloc(size, file, line, true);
}
#endif // !SMART_GC_DEBUG
#endif // !SMART_GC_NOTOVERRIDE_GLOBAL_NEW

#ifndef SMART_GC_NOTOVERRIDE_GLOBAL_DELETE
// Scalar delete
#ifdef _MSC_VER
#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete(void* ptr) noexcept {
	SmartGarbageCollector::gcSmartFree(ptr, false);
}

// Array delete
#ifdef _MSC_VER
#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete[](void* ptr) noexcept {
	SmartGarbageCollector::gcSmartFree(ptr, true);
}
#endif // !SMART_GC_NOTOVERRIDE_GLOBAL_DELETE


// ================================================================================
// Template replacements for new/delete, using when you don't want to override 
// global new/delete operators (SMART_GC_NOTOVERRIDE_GLOBAL_NEW/DELETE), 
// Note: These template functions do not support debugging mode (SMART_GC_DEBUG)
// ================================================================================

template<typename _Ptr_type, typename... _Args>
_Ptr_type* gcNew(_Args&&... args) {
#ifndef SMART_GC_DEBUG
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type), false));
#else
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type), "null", -1, false));
#endif
	if (_ptr != nullptr)
		return new(_ptr) _Ptr_type(std::forward<_Args>(args)...);
	return _ptr;
}

template<typename _Ptr_type, typename _Elem_count = std::size_t>
_Ptr_type* gcNewArray(_Elem_count count) {
#ifndef SMART_GC_DEBUG
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type) * count, false));
#else
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type) * count, "null", -1, false));
#endif
	if (_ptr != nullptr)
		for (_Elem_count i = 0; i < count; ++i)
			::new (&_ptr[i]) _Ptr_type();
	return _ptr;
}

template<typename _Ptr_type>
void gcDelete(_Ptr_type* ptr) {
	if (ptr) {
		ptr->~_Ptr_type();
		SmartGarbageCollector::gcSmartFree(ptr, false);
	}
}

template<typename _Ptr_type, typename _Elem_count = std::size_t>
void gcDeleteArray(_Ptr_type* ptr, _Elem_count count) {
	if (ptr) {
		for (_Elem_count i = 0; i < count; ++i)
			ptr[i].~_Ptr_type();
		SmartGarbageCollector::gcSmartFree(ptr, true);
	}
}


// ================================================================================
// Override global new/delete operators for debugging
// ================================================================================

// Mask default 'new/delete' operators with debug tracking info
// This will ensure the allocation is logged with file/line info
#ifndef SMART_GC_NOTOVERRIDE_GLOBAL_NEW
	#define smart_new						new
	#define smart_delete					delete
	#ifdef SMART_GC_DEBUG
		#undef smart_new
		#define smart_new				new(__FILE__, __LINE__)
		#define debug_new				new(__FILE__, __LINE__)
		#define debug_delete			delete
	#endif // SMART_GC_DEBUG
#endif	// !SMART_GC_NOTOVERRIDE_GLOBAL_NEW
#endif	// if !defined _SMART_GARBAGE_COLLECTION_CPP_INCLUDED_
