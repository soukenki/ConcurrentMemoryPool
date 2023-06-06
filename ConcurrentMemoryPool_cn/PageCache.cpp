#include "PageCache.hpp"

// 定义PageCache
PageCache PageCache::_sInst;

// 获取一个第K页的span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k < N_PAGES);

	// 第k个桶中，有没有span
	if (!_spanLists[k].Empty())
	{
		return _spanLists[k].PopFront(); 
	}
	
	// 检查后面更大的桶中，有没有span（如果有，进行切分）
	for (size_t i = k + 1; i < N_PAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			// 切分大块内存的span
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			// 在nSpan的头部切一个k页下来
			// k页的span返回
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			// n-k页的span头插到对应的位置
			_spanLists[nSpan->_n].PushFront(nSpan);

			// 存储nSpan的首尾页号跟nSpan映射，方便回收page cache回收内存时进行合并查找
			_idSpanMap[nSpan->_pageId] = nSpan;                  // 首页号
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;  // 尾页号


			// 建立id和span的映射，方便central cache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	// 后面没有大页的span，向堆申请一个N_PAGES页的span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(N_PAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;    // 字节数->页号
	bigSpan->_n = N_PAGES - 1;

	// 把N_PAGES页的span插入到第N_PAGES页，再递归自己
	_spanLists[bigSpan->_n].PushFront(bigSpan);
	
	return NewSpan(k);
}

// 获取从对象到span的映射
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);  // 字节数->页号
	auto ret = _idSpanMap.find(id);             // 页号->找映射的span
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

// 释放空间span回到page cache， 合并相邻的span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// 对span前后的页，尝试进行合并，缓解内存外碎片问题
	while (1)
	{
		PAGE_ID prveID = span->_pageId - 1;
		auto ret = _idSpanMap.find(prveID);
		
		// 向前合并的页号没有了，不合并
		if (ret == _idSpanMap.end())       
		{
			break;
		}

		// 向前相邻页的span在使用中，不合并
		Span* prevSpan = ret->second;
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// 合并出超过128页的span没办法管理，不合并
		if (prevSpan->_n + span->_n > N_PAGES - 1)
		{
			break;
		}

		// 向前合并大块内存
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);   // 从对应的桶中删除

		delete prevSpan;
	}

	// 向后合并
	while (1)
	{
		PAGE_ID nextID = span->_pageId + span->_n;
		auto ret = _idSpanMap.find(nextID);
		// 向后找不到
		if (ret == _idSpanMap.end())
		{
			break;
		}

		// 在使用
		Span* nextSpan = ret->second;
		if (nextSpan->_isUse == true)
		{
			break;
		}

		// 大于128页
		if (nextSpan->_n + span->_n > N_PAGES - 1)
		{
			break;
		}

		// 合并
		span->_n += nextSpan->_n;

		_spanLists[nextSpan->_n].Erase(nextSpan);   // 从对应的桶中删除
		
		delete nextSpan;
	}

	// 合并后的span挂起来
	_spanLists[span->_n].PushFront(span);
	span->_isUse = false;
	
	// 添加合并后的映射
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_n - 1] = span;
}