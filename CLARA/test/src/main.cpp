//#include "pch.h"
#define CATCH_CONFIG_MAIN
//#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
/*#include <CLARA/Common/Vector.h>
#include <chrono>
#include <iostream>
#include <vector>*/

/*template<typename T, std::size_t Size>
auto vector_test()
{
	T vec;
	auto cap = vec.capacity();
	auto address = vec.data();
	auto size = vec.size();

	std::cout << "Starting capacity: " << cap << "\n";
	std::cout << "Starting size: " << size << "\n";

	auto start_time = std::chrono::high_resolution_clock::now();

	for (auto i = 0u; i < Size; ++i) {
		vec.push_back(i);
		if (cap != vec.capacity()) {
			cap = vec.capacity();
			if (address == vec.data()) {
				std::cout << "Realloc, same address - new cap: " << vec.capacity() << '\n';
			}
			else {
				address = vec.data();
				std::cout << "Realloc to " << address << " - new cap: " << cap << '\n';
			}
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

	std::cout << "Ending size: " << vec.size() << '\n';
	std::cout << "Time: " << duration << "ns\n";

	bool ok = true;

	for (auto i = 0u; i < Size; ++i) {
		if (vec[i] != i) {
			ok = false;
			std::cout << "Container error vec[" << i << "] was " << vec[i] << '\n';
			break;
		}
	}

	if (ok) std::cout << "Container OK\n";
}*/

/*int main(int, char*[])
{
	try {
		std::cout << "std::vector\n";
		vector_test<std::vector<unsigned int>, 5000>();
		std::cout << "\nstatic_vector\n";
		vector_test<static_vector<unsigned int, 5000>, 5000>();
		std::cout << "\nauto_vector\n";
		vector_test<auto_vector<unsigned int, 1>, 5000>();
	}
	catch (const std::exception& ex) {
		std::cout << ex.what();
	}
	return 0;
}*/