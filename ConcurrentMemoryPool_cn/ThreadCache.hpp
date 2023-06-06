#pragma once

#include "Common.hpp"

class ThreadCache
{
public:
	// 申请内存对象
	void* Allocate(size_t size);

	// 从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);

	// 释放
	void Deallocate(void* ptr, size_t size);

	// 释放对象时，链表太长时，回收内存到中央缓存
	void ListTooLong(FreeList& list, size_t size);

private:
	FreeList _freeLists[N_FREELIST];

};

// TLS thread local storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;