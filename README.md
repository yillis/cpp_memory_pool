阐述传统c++内存分配，使用图文+code解释并实现内存池，并与传统内存分配进行比较。

到我的[blog](https://sanshui.space/2020/02/24/c++%E5%86%85%E5%AD%98%E6%B1%A0%E5%AE%9E%E7%8E%B0/)观看体验更佳
<!-- more -->

## 概述

传统的c++内存分配方式在于new和std::allocator，是从堆里获得内存的，使用结束后用delete释放掉，当然c风格的malloc和free也是兼容的，我们先对一些c++语法知识进行一个探讨，之后进入内存池的原理，最后使用代码实现。

## 你需要知道的

### 语法知识

#### new

大家都知道的new所做的三件事：1.开辟内存 2.调用构造函数 3.返回地址

我们将new分开来看，其实new还有两个种类，分别是**operator new**和**placement new**

##### operator new

用法：

```c++
operator new (the_size_you_want_to_get);
```

这样可以从堆申请到一块原始的空间，**不调用任何构造函数**

比如你想申请一块100字节的空间

```c++
pointer ptr = operator new (100);
```

这样申请空间的效率会更高

##### placement new

用法：

```c++
new (ptr) construct_func();
```

这样可以调用构造函数去构造一块已经申请到的空间

比如：

```c++
Obj* obj_ptr = operator new (sizeof(Obj));
new (obj_ptr) Ojb();
```

此时回过来看，其实new所做的三件事：1.调用operator new开辟空间 2.调用placement new进行构造 3.返回地址

#### delete

大家也都知道，delete是用于释放new申请到的空间，那么与之对应，delete也有对应的两个种类

##### operator delete

用法

```c++
operator delete (ptr); // this pointer must from operator new
```

显示调用operator new得到的空间必须显示调用operator delete释放

##### placement delete

placement delete与之前的不同，之前的用法都是expression，而placement delete则是一个function，其定义如下：

```c++
void operator delete  (void*, void*) noexcept { }
```

默认placement delete()的额外参数是void*，为空实现，什么也不做。

placement delete的作用无非就是调用析构函数，而调用完析构函数后你释放空间仍然只能调用delete，那么就会再次调用析构函数。看起来placement delete的存在似乎毫无意义，但如果存在一个只需要调用析构函数而不需要删除空间的delete情景，那么它便能派上用场。例如构造函数中抛出异常的时候：

```c++
#include <iostream>

struct A {}; //临时中间对象
struct E {}; //异常对象

class T {
  public:
    T() {
        std::cout << "T's constructor" << std::endl;
        throw E();
    }
};

void *operator new(std::size_t, const A &a) {
    std::cout << "Placement new called." << std::endl;
    return (void *)&a;
}

void operator delete(void *, const A &) {
    std::cout << "Placement delete called." << std::endl;
}

int main() {
    A a;
    try {
        T *p = new (a) T;
    } catch (E exp) {
        std::cout << "Exception caught." << std::endl;
    }
    return 0;
}

// output:
// Placement new called.
// T's constructor
// Placement delete called.
// Exception caught.
```

#### template rebind::other

用法：

```c++
typedef Alloc::template rebind<T>::other other_alloc;
```

rebind的目的是为了使与模板类型T相关联的数据如Node<T>和T使用同一种分配机制

贴一个stackoverflow上的一个高赞回答，讲得很清晰

> The `_Alloc` template is used to obtain objects of some type. The container may have an internal need to allocate objects of a different type. For example, when you have a `std::list`, the allocator `A` is meant to allocate objects of type `T` but the `std::list` actually needs to allocate objects of some node type. Calling the node type `_Ty`, the `std::list` needs to get hold of an allocator for `_Ty` objects which is using the allocation mechanism provided by `A`.

### 解决阅读障碍

block：内存块，使用operator new 申请到的一大片**连续**内存

slot：将block分为很多小块slot，是存储数据和指针的基本元素

## 内存池是什么

下图中：

1.一个小方块是一个slot

2.一片连续的slot是一个block

3.红色的slot代表slot作为指针

4.蓝色的slot代表slot作为数据且已被分配

5.绿色的slot代表slot未分配

<img src = 'http://image.sanshui.space/imgs/2020/02/7651095db647e605.png' />

如图可见，block的第一个slot作为指针，指向前一个申请的block，**保证能从当前block向前找到所有的block**

每当被析构的元素再次作为指针，便由指针free_slot一个个串起来作为一个**链表**，链表中的每一个slot都是可被分配的

current_slot代表当前第一个未分配的空间，使用`return current_slot++;`来分配内存

last_slot代表最后一块block中可被使用的slot，用于判断空间是否分配完全

 于是我们可以将内存池大致分为两个区域：1.**`block区`** 2.**`free_slot list 区`**

## 代码实现

> 代码风格：google c++开源风格

为了向std::allocator兼容，我们需要实现几个接口

```c++
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
```



### ~MemoryPool

```c++
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
```

从当前块开始，循着指针向前一次调用`operator delete`释放每一个`block`的内存

### deallocate

```c++
template<typename T, size_t block_size>
void MemoryPool<T, block_size>::deallocate(pointer p, size_t n)
{
	// if p is not nullptr, then add to the free_slot list
	if (p) {
		reinterpret_cast<slot_pointer_>(p)->next = free_slot_;
		free_slot_ = reinterpret_cast<slot_pointer_>(p);
	}
}
```

将`slot`回归到空闲状态，即加入`free_slot list`

### construct

```c++
template<typename T, size_t block_size>
template<typename U, typename ...Args>
inline void MemoryPool<T, block_size>::construct(U* p, Args&& ...args)
{
	// call the constructor, use std::forward handling variable parameters
	new (p) U(std::forward<Args>(args)...);
}
```

构造内存空间，通过`std::forward`来实现可变参数传参给构造函数

### allocate

```c++
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
```

最关键的一个部分

首先判断`free_slot list`中是否还有空间，如果有就返回

如果没有则判断`current_slot`是否越界，如果越界，便申请一块新的`block`，并对其内存和处理指针

最后返回`block`里的空闲空间

最后跑个性能测试：

```c++
// push 1000000 times then clear, loops 100 times
// std::allocator time: 55.202s
// MemoryPool time: 14.157s
// std::vector time: 27.716s
// std::stack time: 27.716s
```

可见我们的内存池效率还是蛮不错的

`vector`的实现思路其实和内存池类似，但是他每次`block`不够的时候，就会重新开辟一块更大的`block`来保证内存连续来实现随机访问，因此用`vecto`模拟stack实际上效果不算好

## 总结

c++的底层处理真的是很厉害，细节和各种骚操作非常多，这也是c++的魅力之一。

完整代码可以到[]()查看

## 参考文献

https://juejin.im/post/5c9c3ff7f265da60e86e2311

https://blog.csdn.net/xjtuse2014/article/details/52302083

https://blog.csdn.net/K346K346/article/details/49514063

https://stackoverflow.com/questions/14148756/what-does-template-rebind-do




```

```