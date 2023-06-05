#pragma once

#include "Common.hpp"

class PageCache
{
public:
	// 获取单例的PageCache
	static PageCache* GetInstance()
	{
		return &_sInstan;
	}

	// 获取一个K页的span
	Span* NewSpan();

private:
	SpanList _spanList[N_PAGES]; // 
	std::mutex _pageMtx;         // PageCache整体的大锁

private:
	PageCache()
	{}
	
	// 禁用拷贝构造
	PageCache(const PageCache& n) = delete;


	static PageCache _sInstan;   // 饿汉
};