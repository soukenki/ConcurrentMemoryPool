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

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空间span回到page cache， 合并相邻的span
	void ReleaseSpanToPageCache(Span* span);

	// 获取锁
	std::mutex& GetPageMtx()
	{
		return _pageMtx;
	}

private:
	SpanList _spanLists[N_PAGES];					// 哈希桶结构
	std::mutex _pageMtx;							// PageCache整体的大锁
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;  // pageID与span之间的映射

private:
	PageCache()
	{}
	
	// 禁用拷贝构造
	PageCache(const PageCache& n) = delete;


	static PageCache _sInst;   // 饿汉
};