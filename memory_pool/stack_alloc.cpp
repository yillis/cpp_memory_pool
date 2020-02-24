#pragma once
#include "stack_alloc.h"

template<typename T, class Alloc>
inline bool StackAlloc<T, Alloc>::is_empty()
{
	return top_ptr_ == nullptr;
}

template<typename T, class Alloc>
void StackAlloc<T, Alloc>::clear()
{
	Node* cur_ptr = top_ptr_;
	Node* tmp_ptr;
	while (cur_ptr) {
		tmp_ptr = cur_ptr->pre_ptr;
		allocator_.destroy(cur_ptr);
		allocator_.deallocate(cur_ptr, 1);
		cur_ptr = tmp_ptr;
	}
	top_ptr_ = nullptr;
	size_ = 0;
}

template<typename T, class Alloc>
void StackAlloc<T, Alloc>::push(T data)
{
	Node* new_node = allocator_.allocate(1);
	allocator_.construct(new_node,Node());
	new_node->pre_ptr = top_ptr_;
	new_node->data = data;
	top_ptr_ = new_node;
	++size_;
}

template<typename T, class Alloc>
void StackAlloc<T, Alloc>::pop()
{
	Node* tmp = top_ptr_;
	top_ptr_ = top_ptr_->pre_ptr;
	allocator_.destroy(tmp);
	allocator_.deallocate(tmp, 1);
	--size_;
}

template<typename T, class Alloc>
T StackAlloc<T, Alloc>::top()
{
	return top_ptr_->data;
}
