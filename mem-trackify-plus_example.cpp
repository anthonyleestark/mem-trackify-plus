/**
 * @file		mem-trackify-plus_example.cpp
 * @brief 		Example usage of MemTrackify++ Library
 * @copyright	Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * @details		Released under MIT License
 *
 */

#include <iostream>
#include <vector>

// #define _MTP_CONSOLE_REPORT_ON_TERMINATION			// uncomment this to view console report result
#include "mem_trackify.h"

//// default testing
#define DEFAULT_TEST

//// stress testing
//#define STRESS_TEST
#define VIEW_EACH_ELEMENT_DELETION						// uncomment this to view console result on deletion of each element

#if defined(DEFAULT_TEST)

class MyClass {
	int data[100];
};

int main() 
{
	// Entry information
	std::cout << "Normal allocation test. \n";

	MyClass* ptr = track_new MyClass;	// 400 bytes
	int* nums = new int[100000];		// 100000 * 4 = 400,000 bytes

	/*
	std::cout << "Template new allocation test. \n";

	int* i_ptr = smartNew<int>(15);
	int* arr_ptr = smartNewArray<int>(30);
	*/

	/*
	char* a = new char[15];
	MyClass* obj2 = new MyClass;
	MyClass* objArray = new MyClass[20];
	int* b = new int[3];
	float* c = new float[7];
	*/

	MemTrackifyPlus* pTracker = getGlobalMemTracker();
	if (pTracker) {
		std::cout << "\n--- Checking tracker and allocated memory size ---\n";
		size_t trackerSize = pTracker->getTrackerSize();
		std::cout << "Memory tracker size: " << trackerSize << " bytes.\n";
		size_t ptrCount = pTracker->getPtrCount();
		std::cout << "Number of allocated pointers: " << ptrCount << ".\n";
		size_t allocMemSize = pTracker->getMemorySize();
		std::cout << "Memory allocated size: " << allocMemSize << " bytes.\n";
	}

	delete ptr;			// Just an example, you actually don't need this
	// delete nums;		// It's okay

#if !defined(_DEBUG) || defined(_MTP_CONSOLE_REPORT_ON_TERMINATION)
	system("pause");
#endif

	return 0;
}

#elif defined(STRESS_TEST)

#include <random>
#include <chrono>

#define MAX_VECTOR_SIZE		1000			// max size of the allocation test vector (replace with any number you want)
#define VECTOR_DEL_COUNT	100				// number of elements for deletion test (replace with any number you want)

void vector_ptr_del(std::vector<uint64_t*>& _vec, uint64_t _del_count)
{
	// Can not delete
	if (_vec.empty() || _vec.size() < _del_count)
		return;

	// Entry information
	std::cout << "Function 'vector_ptr_del' start: will delete " << _del_count << " elements.\n";

	// Create a random number generator with a random seed (based on system time)
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister pseudo-random generator

	// Define the range for random numbers (e.g., between 1 and MAX_VECTOR_SIZE)
	std::uniform_int_distribution<> dis(1, MAX_VECTOR_SIZE);  // [1, MAX_VECTOR_SIZE]

	// Record the start time
	auto start = std::chrono::high_resolution_clock::now();

	uint64_t idx = 0;
	uint64_t _real_del_count = 0;
	while (_real_del_count < _del_count) {

		// Get random position
		do {
			idx = dis(gen);
			if (idx < _vec.size()) break;
		} while (true);

		uint64_t* ptr = _vec[idx];
		if (ptr != nullptr) {
#ifdef VIEW_EACH_ELEMENT_DELETION
			std::cout << "  Delete element at index: " << idx << ". Count: " << _real_del_count << "/" << _del_count << "\n";
#endif

			delete ptr;
			ptr = nullptr;

			_vec[idx] = nullptr;

			--idx;
			++_real_del_count;
		}
	}

	// Record the end time
	auto end = std::chrono::high_resolution_clock::now();

	// Calculate the duration
	std::chrono::duration<double> duration = end - start;

	// Output the runtime in seconds
	std::cout << "Function 'vector_ptr_del()' done. Real delete count: " << _real_del_count << ", total runtime: " << duration.count() << " secs.\n";
}

int main()
{
	// Entry information
	std::cout << "Stress test: Allocate an integer vector of " << MAX_VECTOR_SIZE << " elements and delete " << VECTOR_DEL_COUNT << " random elements.\n";

	// Super large vector of pointers
	std::vector<uint64_t*> my_vector(MAX_VECTOR_SIZE);
	for (uint64_t idx = 0; idx < MAX_VECTOR_SIZE; idx++) {
		my_vector[idx] = new uint64_t(idx);
	}

	vector_ptr_del(my_vector, VECTOR_DEL_COUNT);

	uint64_t _not_null_remain = 0;
	for (uint64_t idx = 0; idx < my_vector.size(); idx++) {
		uint64_t* ptr = my_vector[idx];
		if (ptr != nullptr) {
			_not_null_remain++;
		}
	}
	std::cout << "\nValid pointers (nulless) remaining: " << _not_null_remain << "\n";
	std::cout << "\nClean up remaining...\n";

	for (uint64_t idx = 0; idx < my_vector.size(); idx++) {
#ifdef VIEW_EACH_ELEMENT_DELETION
		std::cout << "  Delete element at index: " << idx;
#endif
		uint64_t* ptr = my_vector[idx];
		if (ptr != nullptr) {
			delete ptr;
			ptr = nullptr;
#ifdef VIEW_EACH_ELEMENT_DELETION
			std::cout << " --> done.\n";
#endif
		}
		else {
#ifdef VIEW_EACH_ELEMENT_DELETION
			std::cout << " --> deleted before.\n";
#endif
		}
	}

	my_vector.clear();
	std::cout << "\nClean up done!!!\n";

#if !defined(_DEBUG) || defined(_MTP_CONSOLE_REPORT_ON_TERMINATION)
	system("pause");
#endif

	return 0;
}
#endif
