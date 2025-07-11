/**
 * @file		mem_trackify.h 
 * @brief 		A very simple, lightweight, header-only C++ library designed to
 *				track memory allocations and deallocations throughout the lifetime of a program.
 * @copyright	Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * @details		Released under MIT License
 * 
 */


 // ================================================================================
 // Macros definition for usage
 // ================================================================================

/**
 * NOTE: In order to use this MemTrack++ library or to enable/disable
 *		 specific modes, define these corresponding macros before including this header,
 *		 or in the Preprocessor Definitions section of your project settings.
 *		 Please read this note carefully before using any of these macros and
 *		 importing memory tracking feature for your C++ program.
 *
 * HOW TO USE:
 *
 *   _MTP_DEBUG
 *		- Enable debugging mode.
 *		- With this macro, you can track Debugging information (filename, line number)
 *		  for memory allocations/deallocations.
 *
 *   _MTP_THREADSAFETY
 *		- Ensure thread-safety when tracking memory allocations/deallocations.
 *		- If your program has no multi-threads, you can skip this macro.
 *
 *   _MTP_CONSOLE_REPORT_ON_TERMINATION
 *		- Display memory leak report and garbage collecting progress on program termination.
 *		- Only enable this macro if you're using a Console Application.
 *
 *   _MTP_NO_OVERRIDE_GLOBAL_OPERATORS
 *		- By default, this library has overriden the global new/delete operators.
 *      - In some cases, this may conflicts some of your other libraries, modules,
 *		  or maybe part of your program architecture, or may cause unexpected behaviors.
 *		- We also provided some methods as replacements for default new/delete operators,
 *		  use them instead if you want to ensure safety for your program.
 *		- Define this macros if you want to disable the default overidden global new/delete operators.
 *
 *	 track_new
 *   track_delete
 *		Use these macros to ensure using this libaray's overriden global tracking new/delete operators.
 *		Example:
 *			int* ptr = track_new int[100];
 *			track_delete ptr;
 *
 *	 debug_new
 *   debug_delete
 *		- Use these macros to ensure using debug version of this libaray's overriden global tracking new/delete operators.
 *		- These macros only work if _MTP_DEBUG is defined and enabled.
 *		Example:
 *			int* ptr = debug_new int[100];
 *			debug_delete ptr;
 *
 */

/**
 * Example:
 *	 #define _MTP_DEBUG
 *	 #define _MTP_THREADSAFETY
 *	 #define _MTP_CONSOLE_REPORT_ON_TERMINATION
 *	 #include "mem_trackify.h"
 *
 */


// ===========================================================
// C++ Memory Tracking Library
// ===========================================================


#ifndef _MEM_TRACKIFY_PLUS_INCLUDED_
#define _MEM_TRACKIFY_PLUS_INCLUDED_

#pragma once


// ===========================================================
// Include header file and dependencies
// ===========================================================

#include <iostream>
#include <sstream>

#include <new>			// for std::bad_alloc

#ifdef _MTP_THREADSAFETY
	#include <mutex>
#endif // _MTP_THREADSAFETY

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

// Memory allocations/deallocations tracking and leak detection
class MemTrackifyPlus 
{
public:
	struct AllocInfo {					// Struct to hold allocation information
		size_t		size;
		bool		isArray;
	};
	struct DebugInfo {					// Struct to hold debugging information
		const char* file = nullptr;
		int32_t		line = -1;
	};

private:
	using Address			= typename void*;
	using StringData		= typename std::string;
	using StringStreamData	= typename std::ostringstream;
	using AtomicFlag		= typename std::atomic<bool>;
	using AllocTrackObj		= typename std::pair<Address, AllocInfo>;
	using AllocTrackData	= typename std::unordered_map<Address, AllocInfo>;
	using DebugTrackObj		= typename std::pair<Address, DebugInfo>;
	using DebugTrackData	= typename std::unordered_map<Address, DebugInfo>;
	using TrackingReport	= typename std::vector<StringData>;

#ifdef _MTP_THREADSAFETY
	using MutexObj			= typename std::recursive_mutex;
	using MutexLockGuard	= typename std::lock_guard<MutexObj>;
#endif // _MTP_THREADSAFETY

public:
	// Constructor
	MemTrackifyPlus() {
		allocTrackData_.reserve(64);
		isTrackerInitialized_ = true;
	};

	// Destructor
	~MemTrackifyPlus() {
#ifdef _MTP_CONSOLE_REPORT_ON_TERMINATION
		this->printTrackingReport(std::cout);
#endif // _MTP_CONSOLE_REPORT_ON_TERMINATION

		// Automatically execute garbage collection at termination
		if (isMemoryLeak()) {
#ifdef _MTP_CONSOLE_REPORT_ON_TERMINATION
			std::cout << "\n--- Executing garbage collection ---\n";
#endif // _MTP_CONSOLE_REPORT_ON_TERMINATION
			for (const auto& info : allocTrackData_) {
				if (info.first) {
#ifdef _MTP_CONSOLE_REPORT_ON_TERMINATION
					std::cout << "  Freed " << info.second.size << " bytes at " << info.first << ".\n";
#endif // _MTP_CONSOLE_REPORT_ON_TERMINATION
					std::free(info.first);  // Clean up
				}
			}

			// Clean up the tracking data itself
			allocTrackData_.clear();
		}
	};

public:
	// Define static functions for smart allocation/deallocation
#ifndef _MTP_DEBUG
	_NODISCARD static inline void* smartAlloc(size_t size, bool isArray);
#else
	_NODISCARD static inline void* smartAlloc(size_t size, const char* file, int line, bool isArray);
#endif // !_MTP_DEBUG
	static inline void smartFree(void* ptr, bool isArray);
	static inline void smartDealloc(void* ptr, bool isArray) { smartFree(ptr, isArray); };

private:
	// Request memory allocation and store debug tracking info
	_NODISCARD void* reqTrackAlloc(size_t size, const char* file, int line, bool isArray) {
		// Invalid size
		if (size == 0) return nullptr;

		// Skip re-entry during tracker map initialization
		thread_local bool isInReqTrackAlloc = false;
		if (isInReqTrackAlloc) return std::malloc(size);

		// Ensure the flag is automatically reset
		AllocGuard allocGuard(isInReqTrackAlloc);

		// Allocate memory block
		void* ptr = std::malloc(size);
		if (!ptr) throw std::bad_alloc();

#ifdef _MTP_THREADSAFETY
		MutexLockGuard _lock(myMutex_);
#endif // _MTP_THREADSAFETY

		// Track allocation info
		if (ptr && (reinterpret_cast<uintptr_t>(ptr) > 0x10000)
			/* only track when the track map is initialized */
			&& isTrackerInitialized_.load(std::memory_order_acquire)) {
			allocTrackData_.insert(AllocTrackObj(ptr, { size, isArray }));
			debugTrackData_.insert(DebugTrackObj(ptr, { file, line }));
		}
		return ptr;
	};

	// Request memory deallocation and clear the pointer debug tracking info
	void reqTrackDealloc(void* ptr, bool isArray) {
		// Not a valid pointer
		if (!ptr) return;

#ifdef _MTP_THREADSAFETY
		MutexLockGuard lock(myMutex_);
#endif // _MTP_THREADSAFETY

		// Check the allocation info and free memory
		if (!isMemoryLeak()) return;
		auto it = allocTrackData_.find(ptr);
		if (it != allocTrackData_.end())
			if (it->first == ptr && it->second.isArray == isArray) {
				allocTrackData_.erase(it);		// Remove the entry
				std::free(ptr);					// Default: Free memory
			}
	};

	// Print memory tracking info
	void printTrackingInfo(const AllocTrackObj& allocTrackObj, std::ostream& os, bool newLine) const noexcept {
		os << "Leaked: " << allocTrackObj.second.size << " bytes "
			<< (allocTrackObj.second.isArray ? "of an array " : "") << "at " << allocTrackObj.first;
#ifdef _MTP_DEBUG
		auto debugInfo = debugTrackData_.get(allocTrackObj.first);
		if (debugInfo != nullptr) {
			os << " in " << ((debugInfo->file != nullptr) ? debugInfo->file : "unknown file");
			if (debugInfo->line != -1)
				os << " (line:" << debugInfo->line << ")";
			else
				os << " (line: unknown)";
		}
#endif // _MTP_DEBUG
		os << (newLine ? ".\n" : ".");
	};

public:
	// Get size of the allocation tracker (in bytes)
	_NODISCARD size_t getTrackerSize(void) const {
		size_t size = 0;
		if (isMemoryLeak())
			for (const auto& info : allocTrackData_) {
				size += sizeof(info.first);
				size += sizeof(info.second);
			}

		return size;
	};

	// Get total tracked allocated memory sizes (in bytes)
	_NODISCARD size_t getMemorySize(void) const {
		size_t size = 0;
		if (isMemoryLeak())
			for (const auto& info : allocTrackData_)
				size += info.second.size;

		return size;
	};

	// Get the number of tracking allocated memory blocks
	_NODISCARD size_t getPtrCount(void) const {
#ifdef _MTP_THREADSAFETY
		MutexLockGuard lock(myMutex_);
#endif // _MTP_THREADSAFETY
		return allocTrackData_.size();
	};

	// Check if there are any allocated memory blocks in use or not yet freed
	_NODISCARD bool isMemoryLeak(void) const {
#ifdef _MTP_THREADSAFETY
		MutexLockGuard lock(myMutex_);
#endif // _MTP_THREADSAFETY
		return (!allocTrackData_.empty());
	};

	// Get memory tracking report data (as an array of string)
	_NODISCARD TrackingReport getTrackingReport(void) const noexcept {
		if (isInReporting_.exchange(true)) { return {}; }
		TrackingReport report;
		if (isMemoryLeak()) {
			report.reserve(getPtrCount());
			for (const auto& info : allocTrackData_) {
				StringStreamData oss;
				printTrackingInfo(info, oss, false);
				report.push_back(oss.str());
			}
		}
		isInReporting_ = false;
		return report;
	};

	// Print memory tracking report data (to file/console, ...)
	void printTrackingReport(std::ostream& os) const noexcept {
		if (isMemoryLeak()) {
			os << "\n--- Memory Leaks Detected ---\n";
			for (const auto& info : allocTrackData_) {
				printTrackingInfo(info, os, true);
			}
		}
		else {
			os << "\nNo memory leaks detected.\n";
		}
	};

private:
	// No copyable
	MemTrackifyPlus(const MemTrackifyPlus&) = delete;
	MemTrackifyPlus& operator=(const MemTrackifyPlus&) = delete;

	// No movable
	MemTrackifyPlus(const MemTrackifyPlus&&) = delete;
	MemTrackifyPlus& operator=(const MemTrackifyPlus&&) = delete;

private:
	// Ensure trackAlloc() function run correctly
	class AllocGuard {
	public:
		// Construction
		AllocGuard(bool& flag) : myFlag_(flag) { myFlag_ = true; };
		~AllocGuard() { myFlag_ = false; };

	private:
		bool& myFlag_;
	};

	// Debug track data wrapper (maybe dummy)
	class DebugTracker {
	public:
#ifdef  _MTP_DEBUG
		// Operations
		void insert(const DebugTrackObj& obj) {	data_.insert(obj); };
		void insert(Address addr, const char* file, int line) {
			data_[addr] = { file, line };
		};
		_NODISCARD const DebugInfo* get(Address addr) const {
			auto it = data_.find(addr);
			if (it != data_.end()) return &it->second;
			return nullptr;
		};
#else
		// Dummy operations
		void insert(const DebugTrackObj&) {};
		void insert(Address, const char*, int) {};
		_NODISCARD const DebugInfo* get(Address) const { return nullptr; };
#endif // !_MTP_DEBUG

	private:
		// Debug data map
		DebugTrackData data_;
	};

private:
	// Attributes
	AllocTrackData		allocTrackData_;				// Stores all allocation info
	DebugTracker		debugTrackData_;				// Stores all debug tracking info
	AtomicFlag			isTrackerInitialized_ = false;	// Check if the tracker finished initializing
	mutable AtomicFlag	isInReporting_ = false;			// Check if the tracking report process is running
#ifdef _MTP_THREADSAFETY
	mutable MutexObj	myMutex_;						// Ensures thread-safety
#endif // _MTP_THREADSAFETY
};


// ================================================================================
// Declare global tracker to handle memory allocation tracking and leak detection
// ================================================================================

class GlobalMemTracker final /* non-inheritable */ {
	// To prevent from directly accessing
#if _HAS_CXX17
	static inline MemTrackifyPlus globalTracker_;
#else
	static MemTrackifyPlus globalTracker_;
#endif
	virtual void dummyFunc() = 0; /* non-instantiable */
public:
	_NODISCARD static MemTrackifyPlus* get(void) {
		return &globalTracker_;
	};
};

// Access the global Memory Tracker
_NODISCARD inline MemTrackifyPlus* getGlobalMemTracker(void) {
	return GlobalMemTracker::get();
};


// ================================================================================
// MemTrackifyPlus static inline function definitions
// ================================================================================

// Smart allocation
#ifndef _MTP_DEBUG
inline void* MemTrackifyPlus::smartAlloc(size_t size, bool isArray) {
	MemTrackifyPlus* allocTracker = getGlobalMemTracker();
	if (allocTracker) return allocTracker->reqTrackAlloc(size, "unknown", -1, isArray);
	return std::malloc(size);
};
#else
inline void* MemTrackifyPlus::smartAlloc(size_t size, const char* file, int line, bool isArray) {
	MemTrackifyPlus* allocTracker = getGlobalMemTracker();
	if (allocTracker) return allocTracker->reqTrackAlloc(size, file, line, isArray);
	return std::malloc(size);
};
#endif // !_MTP_DEBUG

// Smart deallocation
inline void MemTrackifyPlus::smartFree(void* ptr, bool isArray) {
	if (!ptr) return;
	MemTrackifyPlus* allocTracker = getGlobalMemTracker();
	if (allocTracker)
		allocTracker->reqTrackDealloc(ptr, isArray);
	else
		std::free(ptr);  // Default: Free memory
};


// ================================================================================
// Override global new/delete operators
// ================================================================================

#ifndef _MTP_NO_OVERRIDE_GLOBAL_OPERATORS
#ifndef _MTP_DEBUG

#ifndef __CRTDECL
	#define __CRTDECL __cdecl
#endif

// Scalar new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
	_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#else
	_NODISCARD
#endif // !_MSC_VER
inline void* __CRTDECL operator new(std::size_t size) {
	return MemTrackifyPlus::smartAlloc(size, false);
};

// Array new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
	_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#else
	_NODISCARD
#endif // !_MSC_VER
inline void* __CRTDECL operator new[](std::size_t size) {
	return MemTrackifyPlus::smartAlloc(size, true);
};

#else
// Scalar new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#else
	_NODISCARD
#endif // !_MSC_VER
_NODISCARD inline void* operator new(std::size_t size, const char* file, int line) {
	return MemTrackifyPlus::smartAlloc(size, file, line, false);
};

// Array new
#ifdef _MSC_VER
	#pragma warning(disable:4595)
	_VCRT_EXPORT_STD _NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
#else
	_NODISCARD
#endif // !_MSC_VER
_NODISCARD inline void* operator new[](std::size_t size, const char* file, int line) {
	return MemTrackifyPlus::smartAlloc(size, file, line, true);
};
#endif // !_MTP_DEBUG
#endif // !_MTP_NO_OVERRIDE_GLOBAL_OPERATORS

#ifndef _MTP_NO_OVERRIDE_GLOBAL_OPERATORS
// Scalar delete
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete(void* ptr) noexcept {
	MemTrackifyPlus::smartDealloc(ptr, false);
};

// Array delete
#ifdef _MSC_VER
	#pragma warning(disable:4595)
#endif // !_MSC_VER
inline void __CRTDECL operator delete[](void* ptr) noexcept {
	MemTrackifyPlus::smartDealloc(ptr, true);
};
#endif // !_MTP_NO_OVERRIDE_GLOBAL_OPERATORS


// ================================================================================
// Template replacements for new/delete, using when you don't want to override 
// global new/delete operators (_MTP_NOTOVERRIDE_GLOBAL_NEW/DELETE), 
// Note: These template functions do not support debugging mode (_MTP_DEBUG)
// ================================================================================

// Scalar new
template<typename _Ptr_type, typename... _Args>
_NODISCARD _Ptr_type* smartNew(_Args&&... args) {
#ifndef _MTP_DEBUG
	_Ptr_type* ptr = static_cast<_Ptr_type*>(MemTrackifyPlus::smartAlloc(sizeof(_Ptr_type), false));
#else
	_Ptr_type* ptr = static_cast<_Ptr_type*>(MemTrackifyPlus::smartAlloc(sizeof(_Ptr_type), "null", -1, false));
#endif
	if (ptr != nullptr)
		return new(ptr) _Ptr_type(std::forward<_Args>(args)...);
	return ptr;
};

// Array new
template<typename _Ptr_type, typename _Elem_count = std::size_t>
_NODISCARD _Ptr_type* smartNewArray(_Elem_count count) {
#ifndef _MTP_DEBUG
	_Ptr_type* ptr = static_cast<_Ptr_type*>(MemTrackifyPlus::gcSmartAlloc(sizeof(_Ptr_type) * count, false));
#else
	_Ptr_type* ptr = static_cast<_Ptr_type*>(MemTrackifyPlus::gcSmartAlloc(sizeof(_Ptr_type) * count, "null", -1, false));
#endif
	if (ptr != nullptr)
		for (_Elem_count i = 0; i < count; ++i)
			::new (&ptr[i]) _Ptr_type();
	return ptr;
};

// Scalar delete
template<typename _Ptr_type>
void smartDelete(_Ptr_type* ptr) {
	if (ptr) {
		ptr->~_Ptr_type();
		MemTrackifyPlus::smartDealloc(ptr, false);
	}
};

// Array delete
template<typename _Ptr_type, typename _Elem_count = std::size_t>
void smartDeleteArray(_Ptr_type* ptr, _Elem_count count) {
	if (ptr) {
		for (_Elem_count i = 0; i < count; ++i)
			ptr[i].~_Ptr_type();
		MemTrackifyPlus::smartDealloc(ptr, true);
	}
};


// ================================================================================
// Override global new/delete operators for debugging
// ================================================================================

// Mask default 'new/delete' operators with debug tracking info
// This will ensure the allocation is logged with file/line info
#ifndef _MTP_NO_OVERRIDE_GLOBAL_OPERATORS
	#define track_new					new
	#define track_delete				delete
	#ifdef _MTP_DEBUG
		#undef track_new
		#define new						new(__FILE__, __LINE__)
		#define track_new				new		// ^^^
		#define debug_new				new		// ^^^
		#define debug_delete			delete
	#endif // _MTP_DEBUG
#endif	// !_MTP_NO_OVERRIDE_GLOBAL_OPERATORS
#endif	// if !defined _MEM_TRACKIFY_PLUS_INCLUDED_
