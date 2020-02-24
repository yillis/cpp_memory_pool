#include "memory_pool.h"
#include <utility>
#include <cstdint>

template<typename T, size_t block_size>
MemoryPool<T, block_size>::~MemoryPool() noexcept
{
	// from current block, call operator delete to free each block of momory to heap
	slot_pointer_ cur = current_block_;
	while (cur) {
		slot_pointer_ tmp = cur->next;
		operator delete(reinterpret_cast<void*>(cur));
		cur = tmp;
	}
}

template<typename T, size_t block_size>
typename MemoryPool<T, block_size>::pointer MemoryPool<T, block_size>::allocate(size_t n, const pointer hint)
{
	// if free_slot_ list is not null, return the head of the free_slot list
	if (free_slot_) {
		pointer res = reinterpret_cast<pointer>(free_slot_);
		free_slot_ = free_slot_->next;
		return res;
	}
	else {
		// if current_slot out of the block, then get a new block
		if (current_slot_ >= last_slot_) {
			// call operator new
			data_pointer_ new_block = static_cast<data_pointer_>(operator new (block_size));
			reinterpret_cast<slot_pointer_>(new_block)->next = current_block_;
			current_block_ = reinterpret_cast<slot_pointer_>(new_block);

			// aligned memory so that each addresses start at multiples of alignof(slot_type_) 
			data_pointer_ body = new_block + sizeof(slot_type_);
			uintptr_t tmp = reinterpret_cast<uint16_t>(body);
			size_t body_padding = alignof(slot_type_) - tmp % alignof(slot_type_);
			current_slot_ = reinterpret_cast<slot_pointer_>(body + body_padding);
			last_slot_ = reinterpret_cast<slot_pointer_>(new_block + block_size - sizeof(slot_type_) + 1);
		}
		return reinterpret_cast<pointer>(current_slot_++);
	}
}

template<typename T, size_t block_size>
void MemoryPool<T, block_size>::deallocate(pointer p, size_t n)
{
	// if p is not nullptr, then add to the free_slot list
	if (p) {
		reinterpret_cast<slot_pointer_>(p)->next = free_slot_;
		free_slot_ = reinterpret_cast<slot_pointer_>(p);
	}
}

template<typename T, size_t block_size>
template<typename U, typename ...Args>
inline void MemoryPool<T, block_size>::construct(U* p, Args&& ...args)
{
	// call the constructor, use std::forward handling variable parameters
	new (p) U(std::forward<Args>(args)...);
}
