#pragma once

#include "Common.hpp"
#include "ThreadCache.hpp"

// 封装Allocate到TLS中
static void* ConcurrentAlloc(size_t size)
{
	// 通过TLS 每个线程无锁的获取自己专属的ThreadCache对象
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}

	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

	return pTLSThreadCache->Allocate(size);
}


// 释放
static void ConcurrentFree(void* ptr, size_t size)
{
	assert(pTLSThreadCache);

	pTLSThreadCache->Deallocate(ptr, size);
}