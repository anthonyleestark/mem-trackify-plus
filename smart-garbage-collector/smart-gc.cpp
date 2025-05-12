/*
*
 * smart-gc.h
 * A very simple C++ Smart Garbage Collection Library
 * Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * Released under MIT License
 *
 */

#include "smart-gc.h"


// ================================================================================
// Global tracker definition
// ================================================================================

// Global tracker to handle memory allocations
SmartGarbageCollector __g_gcSmartGarbageCollector;


// ================================================================================
// Implementation
// ================================================================================

// Constructor
SmartGarbageCollector::SmartGarbageCollector()
{
	_allocTrackData.reserve(64);
	_isTrackerInitialized = true;
}

// Destructor
SmartGarbageCollector::~SmartGarbageCollector()
{
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
}

#ifndef SMART_GC_DEBUG
// Smart allocation
void* SmartGarbageCollector::gcSmartAlloc(size_t size, bool isArray)
{
	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker) return _allocTracker->gcAlloc(size, "null", -1, isArray);
	return std::malloc(size);
}
#else
// Smart allocation
void* SmartGarbageCollector::gcSmartAlloc(size_t size, const char* file, int line, bool isArray)
{
	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker) return _allocTracker->gcAlloc(size, file, line, isArray);
	return std::malloc(size);
}
#endif // !SMART_GC_DEBUG
// Smart deallocation
void SmartGarbageCollector::gcSmartFree(void* ptr, bool isArray) noexcept
{
	if (!ptr) return;

	SmartGarbageCollector* _allocTracker = gcGetSmartGarbageCollector();
	if (_allocTracker)
		_allocTracker->gcDealloc(ptr, isArray);
	else
		std::free(ptr);  // Default: Free memory
}

// Allocation
void* SmartGarbageCollector::gcAlloc(size_t size, const char* file, int line, bool isArray)
{
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
}

// Deallocation
void SmartGarbageCollector::gcDealloc(void* ptr, bool isArray) noexcept
{
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
}

// Get size of allocation tracker
size_t SmartGarbageCollector::gcGetTrackerSize(void) const
{
	size_t _size = 0;
	if (gcIsMemoryLeak())
		for (const auto& info : _allocTrackData) {
			_size += sizeof(info.first);
			_size += sizeof(info.second);
		}

	return _size;
}

// Get total tracked allocated memory sizes
size_t SmartGarbageCollector::gcGetMemorySize(void) const
{
	size_t _size = 0;
	if (gcIsMemoryLeak())
		for (const auto& info : _allocTrackData)
			_size += info.second._size;

	return _size;
}

// Get the number of tracking allocated memory blocks
size_t SmartGarbageCollector::gcGetPtrCount(void) const 
{
#ifdef SMART_GC_THREADSAFETY
	MutexLockGuard lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY

	return _allocTrackData.size(); 
}

// Check if there are any allocated memory blocks in use or not yet freed
bool SmartGarbageCollector::gcIsMemoryLeak(void) const 
{
#ifdef SMART_GC_THREADSAFETY
	MutexLockGuard lock(_Mymutex);
#endif // SMART_GC_THREADSAFETY

	return (!_allocTrackData.empty());
}

// Get list of tracking data (as an array of string)
SmartGarbageCollector::LeakReport
SmartGarbageCollector::gcGetLeakReport(void) const noexcept
{
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
}

// Print tracking data (to file/console, ...)
void SmartGarbageCollector::gcPrintLeakInfo(std::ostream& os) const noexcept
{
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
}
