#pragma once
#include <climits>
#include <cstddef>

template <typename T, size_t block_size = 4096>
class MemoryPool{
public:
	typedef T* pointer;

	// rebind::other api
	template <typename U>
	struct rebind {
		typedef MemoryPool<U> other;
	};

	// construct func
	MemoryPool() noexcept:current_block_(nullptr),
		current_slot_(nullptr),
		last_slot_(nullptr),
		free_slot_(nullptr) {}
	// destruct func
	~MemoryPool() noexcept;

	// alloc the memory from memory pool
	pointer allocate(size_t n = 1, const pointer hint = nullptr);
	// return the memory to the memory pool
	void deallocate(pointer p, size_t n = 1);
	// call the constructor to construct object
	template<typename U, typename... Args>
	void construct(U* p, Args&&... args);
	// call destructor
	template<typename U>
	void destroy(U* p) { p->~U(); }

private:
	union Slot_ {
		T element;
		Slot_* next;
	};
	typedef char* data_pointer_;
	typedef Slot_ slot_type_;
	typedef Slot_* slot_pointer_;

	// MemoryPool essential pointer
	slot_pointer_ current_block_;
	slot_pointer_ current_slot_;
	slot_pointer_ last_slot_;
	slot_pointer_ free_slot_;
	// assert, to judge the block_size
	static_assert(block_size >= sizeof(slot_type_), "block size too small.");
};