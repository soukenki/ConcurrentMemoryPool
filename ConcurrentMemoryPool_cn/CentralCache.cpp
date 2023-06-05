#include "CentralCache.hpp"
#include "PageCache.hpp"

CentralCache CentralCache::_sInst;

// 获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	// 遍历当前spanlist，找非空span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	// 没有非空的span，往下一层page cache请求内存
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));

	return nullptr;
}

// 从中央缓存获取n个数量的对象给thread cache
size_t CentralCache::FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);  // 桶位置
	
	// 多线程访问同一个桶，加锁
	_spanLists[index].GetMtx().lock();

	Span* span = GetOneSpan(_spanLists[index], size);  // 找一个非空的span
	assert(span);
	assert(span->_freeList);

	// 在特定的span中，把链表中的头尾指针取到(获取batchNum个对象)
	// 如果不够batchNum个对象，有多少取多少个。
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;   // 实际获取到的内存块数量
	while (i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;


	_spanLists[index].GetMtx().unlock();  // 解锁
	
	return actualNum;
}