#pragma once

#include "Common.hpp"

class PageCache
{
public:
	// 获取单例的PageCache
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// 获取一个K页的span
	Span* NewSpan(size_t k);

	// 获取锁
	std::mutex& GetPageMtx()
	{
		return _pageMtx;
	}

private:
	SpanList _spanLists[N_PAGES];   // 哈希桶结构
	std::mutex _pageMtx;           // PageCache整体的大锁

private:
	PageCache()
	{}
	
	// 禁用拷贝构造
	PageCache(const PageCache& n) = delete;


	static PageCache _sInst;   // 饿汉
};