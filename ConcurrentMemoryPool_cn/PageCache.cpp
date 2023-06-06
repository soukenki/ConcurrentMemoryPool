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
			kSpan->_pageID = nSpan->_pageID;
			kSpan->_n = k;

			nSpan->_pageID += k;
			nSpan->_n -= k;

			// n-k页的span头插到对应的位置
			_spanLists[nSpan->_n].PushFront(nSpan);

			return kSpan;
		}
	}

	// 后面没有大页的span，向堆申请一个N_PAGES页的span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(N_PAGES - 1);
	bigSpan->_pageID = (PAGE_ID)ptr >> PAGE_SHIFT;    // 字节数->页号
	bigSpan->_n = N_PAGES - 1;

	// 把N_PAGES页的span插入到第N_PAGES页，再递归自己
	_spanLists[bigSpan->_n].PushFront(bigSpan);
	
	return NewSpan(k);
}