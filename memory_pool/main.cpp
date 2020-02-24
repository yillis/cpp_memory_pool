#include "stack_alloc.h"
#include "stack_alloc.cpp"
#include "memory_pool.h"
#include "memory_pool.cpp"
#include <ctime>
#include <iostream>
#include <memory>
#include <vector>
#include <stack>
#define NODE_SIZE 1000000
#define REPS 100

int main()
{
//	StackAlloc<int, std::allocator<int> > stack_alloc;
//	std::vector<int> v;
	std::stack<int> s;
	clock_t start = clock();
	for (int i = 0; i < REPS; ++i) {
		for (int j = 0; j < NODE_SIZE; ++j) {
			//stack_alloc.push(1);
			//v.push_back(1);
			s.push(1);
		}
		//stack_alloc.clear();
		//v.clear();z
		while (!s.empty()) {
			s.pop();
		}
	}
	std::cout << "alloc time is:" << ((double)clock() - start) / CLOCKS_PER_SEC << std::endl;
	system("pause");
	return 0;
}