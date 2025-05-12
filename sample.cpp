#include <iostream>
#include <vector>

#define SMART_GC_OVERRIDE_NEW
#include "smart-gc.h"

#define DEFAULT_TEST

#ifdef DEFAULT_TEST
class MyClass {
	int data[100];
};

int main() 
{
	std::cout << "Normal allocation test. \n";

	MyClass* ptr = smart_new MyClass;
	int* nums = new int[100000];

	/*
	std::cout << "Template new allocation test. \n";

	int* i_ptr = gcNew<int>(15);
	int* arr_ptr = gcNewArray<int>(30);
	*/

	/*
	char* a = new char[15];
	MyClass* obj2 = new MyClass;
	MyClass* objArray = new MyClass[20];
	int* b = new int[3];
	float* c = new float[7];
	*/

	SmartGarbageCollector* pGC = gcGetAllocTracker();
	if (pGC) {
		std::cout << "\n--- Checking tracker and allocated memory size ---\n";
		size_t gcTrackerSize = pGC->gcGetTrackerSize();
		std::cout << "Memory tracker size: " << gcTrackerSize << " bytes. \n";
		size_t gcAllocMemSize = pGC->gcGetMemorySize();
		std::cout << "Memory allocated size: " << gcAllocMemSize << " bytes. \n";
	}

	delete ptr;

	return 0;
}
#endif

#ifdef STRESS_TEST

#include <random>
#include <chrono>

#define MAX_VECTOR_NUM		1000000
#define VECTOR_DEL_COUNT	100000

void vector_ptr_del(std::vector<int*>& _vec, int _del_count)
{
	// Can not delete
	if (_vec.empty() || _vec.size() < _del_count)
		return;

	// Entry
	std::cout << "Function 'vector_ptr_del' start: delete " << _del_count << " elements." << std::endl;

	// Create a random number generator with a random seed (based on system time)
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister pseudo-random generator

	// Define the range for random numbers (e.g., between 1 and 10000000)
	std::uniform_int_distribution<> dis(1, MAX_VECTOR_NUM);  // [1, 10000000]

	// Record the start time
	auto start = std::chrono::high_resolution_clock::now();

	int idx = 0;
	int _real_del_count = 0;
	while (_real_del_count < _del_count) {

		do {
			idx = dis(gen);
			if (idx < _vec.size()) break;
		} while (true);

		int* ptr = _vec[idx];
		if (ptr != nullptr) {
//			std::cout << "  Delete element at index: " << idx << ". Count: " << _real_del_count << "/" << _del_count << "\n";

			delete ptr;
			ptr = nullptr;

			_vec[idx] = nullptr;

			idx--;
			_real_del_count++;
		}
	}

	// Record the end time
	auto end = std::chrono::high_resolution_clock::now();

	// Calculate the duration
	std::chrono::duration<double> duration = end - start;

	// Output the runtime in seconds
	std::cout << "Function 'vector_ptr_del' done. Real delete count: " << _real_del_count << ", total runtime: " << duration.count() << " secs." << std::endl;
}

int main()
{
	// Super large vector of pointers
	std::vector<int*> my_vector(MAX_VECTOR_NUM);
	for (uint64_t idx = 0; idx < MAX_VECTOR_NUM; idx++) {
		my_vector[idx] = new int(idx);
	}

	vector_ptr_del(my_vector, VECTOR_DEL_COUNT);

	int _not_null_remain = 0;
	for (uint64_t idx = 0; idx < my_vector.size(); idx++) {
		int* ptr = my_vector[idx];
		if (ptr != nullptr) {
			_not_null_remain++;
		}
	}
	std::cout << "\nValid pointers (not null) remaining: " << _not_null_remain << "\n";
	std::cout << "\nClean up remaining...\n";

	for (uint64_t idx = 0; idx < my_vector.size(); idx++) {
		int* ptr = my_vector[idx];
		if (ptr != nullptr) {
//			std::cout << "  Delete element at index: " << idx << "\n";
			delete ptr;
			ptr = nullptr;
		}
	}

	my_vector.clear();
	std::cout << "\nClean up done!!!\n";
}
#endif