#pragma once

#include "Common.hpp"
#include "ThreadCache.hpp"

// ��װAllocate��TLS��
static void* ConcurrentAlloc(size_t size)
{
	// ͨ��TLS ÿ���߳������Ļ�ȡ�Լ�ר����ThreadCache����
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}

	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

	return pTLSThreadCache->Allocate(size);
}


// �ͷ�
static void ConcurrentFree(void* ptr, size_t size)
{
	assert(pTLSThreadCache);

	pTLSThreadCache->Deallocate(ptr, size);
}