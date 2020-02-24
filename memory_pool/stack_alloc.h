#pragma once
#include <memory>
#include "memory_pool.h"

template<typename T>
struct StackNode_ {
	T data;
	StackNode_* pre_ptr;
};

template<typename T, class Alloc>
class StackAlloc {
public:
	typedef StackNode_<T> Node;
	typedef typename Alloc::template rebind<Node>::other Allocator;
	/*
	other:
	template<typename U>
	struct rebind{
		typedef Alloc<U> other;
	}
	if typename U is associate with typename T;
	use rebind can let translater know that;

	#from stackoverflow
	The _Alloc template is used to obtain objects of some type.
	The container may have an internal need to allocate objects of a different type.
	For example, when you have a std::list<T, A>,
	the allocator A is meant to allocate objects of type T
	but the std::list<T, A> actually needs to allocate objects of some node type.
	Calling the node type _Ty, the std::list<T, A> needs to get hold of an allocator for _Ty objects
	which is using the allocation mechanism provided by A.
	*/

	// 栈的必备四个接口
	bool is_empty();
	void clear();
	void push(T data);
	void pop();
	T top();

	StackAlloc() :top_ptr_(nullptr), size_(0){}
	~StackAlloc() { clear(); }
private:
	Node* top_ptr_;
	Allocator allocator_;
	int size_;
};
