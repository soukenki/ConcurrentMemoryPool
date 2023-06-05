#pragma once

#include "Common.hpp"

class ThreadCache
{
public:
	// �����ڴ����
	void* Allocate(size_t size);

	// �ͷ�
	void Deallocate(void* ptr, size_t size);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);

private:
	FreeList _freeLists[N_FREELIST];

};

// TLS thread local storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;