
#include "ThreadCache.hpp"
#include "CentralCache.hpp"

// 到CentralCache取内存块
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// 慢开始反馈调节算法
	// 1.最开始不会一次向central cache一次批量要太多，因为太多用不完。
	// 2.如果目前不需要一下申请太多，那么maxSize不断增大，直到上限NumMoveSize。
	// 3.size越大，一次向central cache要的batchNum就越越小
	// 4.size越小，一次向central cache要的batchNum就越越大
	
	size_t batchNum = std::min(_freeLists[index].GetMaxSize(),SizeClass::NumMoveSize(size));  // 使用maxSize 与 NumMoveSize较小的那个 

	if (_freeLists[index].GetMaxSize() == batchNum)
	{
		_freeLists[index].GetMaxSize() += 1;
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangObj(start, end, batchNum, size);
	// 每个span中空闲的内存块是不确定的，但至少有一个
	assert(actualNum > 1);
	if (actualNum == 1)
	{
		// 从CentralCache中只获取到一个内存块
		assert(start == end);
		return start;
	}
	else
	{
		// 从CentralCache获取到多个内存块
		_freeLists[index].pushRange(NextObj(start), end);  // 插入多个内存块
		return start;
	}
}

// 申请内存对象
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	
	size_t alignSize = SizeClass::RoundUp(size);  // 对齐数大小
	size_t index = SizeClass::Index(size);   // 桶位置

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].pop();
	}
	else
	{
		// 向下一层申请
		return FetchFromCentralCache(index, alignSize);
	}
}

// 释放
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);
	
	// 找出映射的自由链表桶，头插
	size_t index = SizeClass::Index(size);   // 几号桶
	_freeLists[index].push(ptr);

	// ..太多，往下一层释放
}


// 从中心缓存获取对象
void* FetchFromCentralCache(size_t index, size_t size)
{
	return nullptr;
}