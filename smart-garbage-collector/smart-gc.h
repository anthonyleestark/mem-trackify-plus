/**
 * @file		smart-gc.h 
 * @description A very simple C++ Smart Garbage Collection Library
 * @copyright	Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * @license		Released under MIT License
 * 
 */


 // ================================================================================
 // Macros definition for usage
 // ================================================================================

/**
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

/**
 * Example:
 *	 #define SMART_GC_DEBUG
 *	 #define SMART_GC_THREADSAFETY
 *	 #define SMART_GC_CONSOLE_REPORT_ON_TERMINATION
 *	 #include "smart-gc.h"
 *
 */


// ===========================================================
// C++ Smart Garbage Collection Library
// ===========================================================


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


// ===========================================================
// C++ Version and preprocessing macro definition check
// ===========================================================

#ifdef __cplusplus
	#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
		#define _STL_LANG _MSVC_LANG
	#else  // ^^^ language mode is _MSVC_LANG / language mode is __cplusplus vvv
		#define _STL_LANG __cplusplus
	#endif // ^^^ language mode is larger of _MSVC_LANG and __cplusplus ^^^
#else  // ^^^ determine compiler's C++ mode / no C++ support vvv
	#define _STL_LANG 0L
#endif // ^^^ no C++ support ^^^

#ifndef _HAS_CXX17
	#if _STL_LANG > 201402L
		#define _HAS_CXX17 1
	#else
		#define _HAS_CXX17 0
	#endif
#endif // _HAS_CXX17

#ifndef _HAS_CXX20
	#if _HAS_CXX17 && _STL_LANG > 201703L
		#define _HAS_CXX20 1
	#else
		#define _HAS_CXX20 0
	#endif
#endif // _HAS_CXX20

#ifndef _HAS_CXX23
	#if _HAS_CXX20 && _STL_LANG > 202002L
		#define _HAS_CXX23 1
	#else
		#define _HAS_CXX23 0
	#endif
#endif // _HAS_CXX23

#ifndef _HAS_CXX26
	#if _HAS_CXX23 && _STL_LANG > 202302L
		#define _HAS_CXX26 1
	#else
		#define _HAS_CXX26 0
	#endif
#endif // _HAS_CXX26

#undef _STL_LANG

#if _HAS_CXX20 && !_HAS_CXX17
	#error _HAS_CXX20 must imply _HAS_CXX17.
#endif

#if _HAS_CXX23 && !_HAS_CXX20
	#error _HAS_CXX23 must imply _HAS_CXX20.
#endif

#if _HAS_CXX26 && !_HAS_CXX23
	#error _HAS_CXX26 must imply _HAS_CXX23.
#endif

// [[nodiscard]] attributes on STL functions
#ifndef _NODISCARD
	#ifndef _HAS_NODISCARD
		#ifndef __has_cpp_attribute
			#define _HAS_NODISCARD 0
		#elif __has_cpp_attribute(nodiscard) >= 201603L // TRANSITION, VSO#939899 (need toolset update)
			#define _HAS_NODISCARD 1
		#else
			#define _HAS_NODISCARD 0
		#endif
	#endif // _HAS_NODISCARD

	#if _HAS_NODISCARD
		#define _NODISCARD [[nodiscard]]
	#else // ^^^ CAN HAZ [[nodiscard]] / NO CAN HAZ [[nodiscard]] vvv
		#define _NODISCARD
	#endif // _HAS_NODISCARD
#endif _NODISCARD


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
	// Constructor
	SmartGarbageCollector() {
		_allocTrackData.reserve(64);
		_isTrackerInitialized = true;
	};

	// Destructor
	~SmartGarbageCollector() {
#ifdef SMART_GC_CONSOLE_REPORT_ON_TERMINATION
		this->gcPrintLeakInfo(std::cout);
#endif // SMART_GC_CONSOLE_REPORT_ON_TERMINATION

		// Automatically execute garbage collection at termination
		if (gcIsMemoryLeak()) {
#ifdef SMART_GC_CONSOLE_REPORT_ON_TERMINATION
			std::cout << "\n--- Executing garbage collection ---\n";
#endif // SMART_GC_CONSOLE_REPORT_ON_TERMINATION
			for (const auto& info : _allocTrackData) {
				if (info.first) {
#ifdef SMART_GC_CONSOLE_REPORT_ON_TERMINATION
					std::cout << "  Freed " << info.second._size << " bytes at " << info.first << ".\n";
#endif // SMART_GC_CONSOLE_REPORT_ON_TERMINATION
					std::free(info.first);  // Clean up
				}
			}

			// Clean up the tracking data itself
			_allocTrackData.clear();
		}
	};

public:
	// Define static functions for smart allocation/deallocation
#ifndef SMART_GC_DEBUG
	_NODISCARD static inline void* gcSmartAlloc(size_t size, bool isArray);
#else
	_NODISCARD static inline void* gcSmartAlloc(size_t size, const char* file, int line, bool isArray);
#endif // !SMART_GC_DEBUG
	static inline void gcSmartFree(void* ptr, bool isArray) noexcept;
	static inline void gcSmartDealloc(void* ptr, bool isArray) noexcept { gcSmartFree(ptr, isArray); };

private:
	// Request memory allocation and store debug tracking info
	_NODISCARD void* gcAlloc(size_t size, const char* file, int line, bool isArray) {
		// Invalid size
		if (size == 0) return nullptr;

		// Skip re-entry during tracker map initialization
		thread_local bool _in_gcAlloc = false;
		if (_in_gcAlloc) return std::malloc(size);

		// Ensure the flag is automatically reset
		GCAllocGuard guard(_in_gcAlloc);

		// Allocate memory block
		void* ptr = std::malloc(size);
		if (!ptr) throw std::bad_alloc();

#ifdef SMART_GC_THREADSAFETY
		MutexLockGuard _lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY

		// Track allocation info
		if (ptr && (reinterpret_cast<uintptr_t>(ptr) > 0x10000)
			/* only track when the track map is initialized */
			&& _isTrackerInitialized.load(std::memory_order_acquire)) {

#ifndef SMART_GC_DEBUG
			_allocTrackData.insert(AllocTrackObj(ptr, { size, isArray }));
#else
			_allocTrackData.insert(AllocTrackObj(ptr, { size, isArray, { file, line } }));
#endif // !SMART_GC_DEBUG
		}
		return ptr;
	};

	// Request memory deallocation and clear the pointer debug tracking info
	void gcDealloc(void* ptr, bool isArray) noexcept {
		// Not a valid pointer
		if (!ptr) return;

#ifdef SMART_GC_THREADSAFETY
		MutexLockGuard lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY

		// Check the allocation info and free memory
		if (!gcIsMemoryLeak()) return;
		auto it = _allocTrackData.find(ptr);
		if (it != _allocTrackData.end())
			if (it->first == ptr && it->second._is_array == isArray) {
				_allocTrackData.erase(it);		// Remove the entry
				std::free(ptr);					// Default: Free memory
			}
	};

public:
	// Get size of the allocation tracker (in bytes)
	_NODISCARD size_t gcGetTrackerSize(void) const {
		size_t _size = 0;
		if (gcIsMemoryLeak())
			for (const auto& info : _allocTrackData) {
				_size += sizeof(info.first);
				_size += sizeof(info.second);
			}

		return _size;
	};

	// Get total tracked allocated memory sizes (in bytes)
	_NODISCARD size_t gcGetMemorySize(void) const {
		size_t _size = 0;
		if (gcIsMemoryLeak())
			for (const auto& info : _allocTrackData)
				_size += info.second._size;

		return _size;
	};

	// Get the number of tracking allocated memory blocks
	_NODISCARD size_t gcGetPtrCount(void) const {
#ifdef SMART_GC_THREADSAFETY
		MutexLockGuard lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY
		return _allocTrackData.size();
	};

	// Check if there are any allocated memory blocks in use or not yet freed
	_NODISCARD bool gcIsMemoryLeak(void) const {
#ifdef SMART_GC_THREADSAFETY
		MutexLockGuard lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY
		return (!_allocTrackData.empty());
	};

	// Get list of tracking data (as an array of string)
	_NODISCARD LeakReport gcGetLeakReport(void) const noexcept {
		LeakReport _report;
		if (gcIsMemoryLeak()) {
			for (const auto& info : _allocTrackData) {
				StringStreamData oss;
				oss << "Memory leaked: " << info.second._size << " bytes "
					<< (info.second._is_array ? "of an array " : "")
					<< "at " << info.first
#ifdef SMART_GC_DEBUG
					<< " in " << info.second._debug_info._file << " (line:" << info.second._debug_info._line << ")"
#endif // SMART_GC_DEBUG
					<< ".";
				_report.push_back(oss.str());
			}
		}
		return _report;
	};

	// Print tracking data (to file/console, ...)
	void gcPrintLeakInfo(std::ostream& os) const noexcept {
		if (gcIsMemoryLeak()) {
			os << "\n--- Memory Leaks Detected ---\n";
			for (const auto& info : _allocTrackData) {
				os << "Memory leaked: " << info.second._size << " bytes "
					<< (info.second._is_array ? "of an array " : "")
					<< "at " << info.first
#ifdef SMART_GC_DEBUG
					<< " in " << info.second._debug_info._file << " (line:" << info.second._debug_info._line << ")"
#endif // SMART_GC_DEBUG
					<< ".\n";
			}
		}
		else {
			os << "\nNo memory leaks detected.\n";
		}
	};

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
		GCAllocGuard(bool& flag) : _Myflag(flag) { _Myflag = true; };
		~GCAllocGuard() { _Myflag = false; };

	private:
		bool& _Myflag;
	};
};


// ================================================================================
// Declare global tracker to handle memory allocation tracking and leak detection
// ================================================================================

class __smart_gc_global final /* non-inheritable */ {
	// To prevent from directly accessing
#if _HAS_CXX17
	static inline SmartGarbageCollector __g_gcSmartGarbageCollector;
#else
	static SmartGarbageCollector __g_gcSmartGarbageCollector;
#endif
	virtual void __smart_gc_global_dummy_func() = 0; /* non-instantiable */
public:
	_NODISCARD static SmartGarbageCollector* __get(void) {
		return &__g_gcSmartGarbageCollector;
	};
};

// Access the global Smart Garbage Collector
_NODISCARD inline SmartGarbageCollector* gcGetSmartGarbageCollector(void) {
	return __smart_gc_global::__get();
};


// ================================================================================
// SmartGarbageCollector static inline function definitions
// ================================================================================

// Smart allocation
#ifndef SMART_GC_DEBUG
inline void* SmartGarbageCollector::gcSmartAlloc(size_t size, bool isArray) {
	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker) return _allocTracker->gcAlloc(size, "null", -1, isArray);
	return std::malloc(size);
};
#else
inline void* SmartGarbageCollector::gcSmartAlloc(size_t size, const char* file, int line, bool isArray) {
	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker) return _allocTracker->gcAlloc(size, file, line, isArray);
	return std::malloc(size);
};
#endif // !SMART_GC_DEBUG

// Smart deallocation
inline void SmartGarbageCollector::gcSmartFree(void* ptr, bool isArray) noexcept {
	if (!ptr) return;
	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker)
		_allocTracker->gcDealloc(ptr, isArray);
	else
		std::free(ptr);  // Default: Free memory
};


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
};

// Array new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
	_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#endif // !_MSC_VER
inline void* __CRTDECL operator new[](std::size_t size) {
	return SmartGarbageCollector::gcSmartAlloc(size, true);
};

#else
// Scalar new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
_NODISCARD inline void* operator new(size_t size, const char* file, int line) {
	return SmartGarbageCollector::gcSmartAlloc(size, file, line, false);
};

// Array new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
_NODISCARD inline void* operator new[](size_t size, const char* file, int line) {
	return SmartGarbageCollector::gcSmartAlloc(size, file, line, true);
};
#endif // !SMART_GC_DEBUG
#endif // !SMART_GC_NOTOVERRIDE_GLOBAL_NEW

#ifndef SMART_GC_NOTOVERRIDE_GLOBAL_DELETE
// Scalar delete
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete(void* ptr) noexcept {
	SmartGarbageCollector::gcSmartFree(ptr, false);
};

// Array delete
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete[](void* ptr) noexcept {
	SmartGarbageCollector::gcSmartFree(ptr, true);
};
#endif // !SMART_GC_NOTOVERRIDE_GLOBAL_DELETE


// ================================================================================
// Template replacements for new/delete, using when you don't want to override 
// global new/delete operators (SMART_GC_NOTOVERRIDE_GLOBAL_NEW/DELETE), 
// Note: These template functions do not support debugging mode (SMART_GC_DEBUG)
// ================================================================================

// Scalar new
template<typename _Ptr_type, typename... _Args>
_NODISCARD _Ptr_type* gcNew(_Args&&... args) {
#ifndef SMART_GC_DEBUG
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type), false));
#else
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type), "null", -1, false));
#endif
	if (_ptr != nullptr)
		return new(_ptr) _Ptr_type(std::forward<_Args>(args)...);
	return _ptr;
};

// Array new
template<typename _Ptr_type, typename _Elem_count = std::size_t>
_NODISCARD _Ptr_type* gcNewArray(_Elem_count count) {
#ifndef SMART_GC_DEBUG
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type) * count, false));
#else
	_Ptr_type* _ptr = static_cast<_Ptr_type*>(SmartGarbageCollector::gcSmartAlloc(sizeof(_Ptr_type) * count, "null", -1, false));
#endif
	if (_ptr != nullptr)
		for (_Elem_count i = 0; i < count; ++i)
			::new (&_ptr[i]) _Ptr_type();
	return _ptr;
};

// Scalar delete
template<typename _Ptr_type>
void gcDelete(_Ptr_type* ptr) {
	if (ptr) {
		ptr->~_Ptr_type();
		SmartGarbageCollector::gcSmartFree(ptr, false);
	}
};

// Array delete
template<typename _Ptr_type, typename _Elem_count = std::size_t>
void gcDeleteArray(_Ptr_type* ptr, _Elem_count count) {
	if (ptr) {
		for (_Elem_count i = 0; i < count; ++i)
			ptr[i].~_Ptr_type();
		SmartGarbageCollector::gcSmartFree(ptr, true);
	}
};


// ================================================================================
// Override global new/delete operators for debugging
// ================================================================================

// Mask default 'new/delete' operators with debug tracking info
// This will ensure the allocation is logged with file/line info
#ifndef SMART_GC_NOTOVERRIDE_GLOBAL_NEW
	#define smart_new					new
	#define smart_new_array				new[]
	#define smart_delete				delete
	#define smart_delete_array			delete[]
	#ifdef SMART_GC_DEBUG
		#undef smart_new
		#undef smart_new_array
		#define smart_new				new(__FILE__, __LINE__)
		#define smart_new_array			new[](__FILE__, __LINE__)
		#define debug_new				new(__FILE__, __LINE__)
		#define debug_new_array			new[](__FILE__, __LINE__)
		#define debug_delete			delete
		#define debug_delete_array		delete[]
	#endif // SMART_GC_DEBUG
#else
	#define smart_new					gcNew
	#define smart_new_array				gcNewArray
	#define smart_delete				gcDelete
	#define smart_delete_array			gcDeleteArray
#endif	// !SMART_GC_NOTOVERRIDE_GLOBAL_NEW
#endif	// if !defined _SMART_GARBAGE_COLLECTION_CPP_INCLUDED_
