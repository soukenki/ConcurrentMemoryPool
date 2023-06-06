#pragma once


#include "Common.hpp"

// 单例模式 懒汉
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// 获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t size);

	// 从中心缓存获取n个数量的对象给thread cache
	size_t FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t byte_size);

private: 
	SpanList _spanLists[N_FREELIST];    // 哈希桶结构，每个桶有N个span

private:
	// 构造函数私有
	CentralCache()
	{}

	// 禁用拷贝构造
	CentralCache(const CentralCache& n) = delete;


	static CentralCache _sInst;  // CentralCache内存池
};