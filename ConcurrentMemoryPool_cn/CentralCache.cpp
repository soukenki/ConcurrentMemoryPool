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

	// 先把central cache的桶锁解掉，如果其他线程释放内存，不会阻塞
	list.GetMtx().unlock();

	// 没有空闲的span，往下一层page cache请求span
	PageCache::GetInstance()->GetPageMtx().lock();  // 进入下一层前，上锁

	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	
	PageCache::GetInstance()->GetPageMtx().unlock();   // 解锁

	// 对获取的span进行切分，不需要加锁，因为这时其他线程无法访问这个span
	
	// 计算span的大块内存的起始地址和大块内存的大小（字节数）
	char* start = (char*)(span->_pageID << PAGE_SHIFT);  // span页号 * 8k == span起始地址 
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	// 把大块内存切成小块，作为自由链表链接起来
	// 1.先切一块下来，做头。方便之后尾插
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	// 2.尾插
	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);   // tail = start;
		start += size;
	}

	// 3.把span头插到central cache的桶中
	// 插入central cache前，加锁
	list.GetMtx().lock();
	list.PushFront(span);

	return span;
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

	span->_useCount += actualNum;   // 记录所有被拿走的内存块数量


	_spanLists[index].GetMtx().unlock();  // 解锁
	
	return actualNum;
}

// 将一定数量的对象释放到span跨度
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);  // 桶位置
	_spanLists[index].GetMtx().lock();      // 上锁



	_spanLists[index].GetMtx().unlock();      // 解锁

}
