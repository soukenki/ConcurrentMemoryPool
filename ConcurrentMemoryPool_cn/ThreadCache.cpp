
#include "ThreadCache.hpp"
#include "CentralCache.hpp"

// �����뻺��central cacheȡ�ڴ��
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// ����ʼ���������㷨
	// 1.�ʼ����һ����central cacheһ������Ҫ̫�࣬��Ϊ̫���ò��ꡣ
	// 2.���Ŀǰ����Ҫһ������̫�࣬��ômaxSize��������ֱ������NumMoveSize��
	// 3.sizeԽ��һ����central cacheҪ��batchNum��ԽԽС
	// 4.sizeԽС��һ����central cacheҪ��batchNum��ԽԽ��
	
	size_t batchNum = min(_freeLists[index].GetMaxSize(),SizeClass::NumMoveSize(size));  // ʹ��maxSize �� NumMoveSize��С���Ǹ� 

	if (_freeLists[index].GetMaxSize() == batchNum)
	{
		_freeLists[index].GetMaxSize() += 1;
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangObj(start, end, batchNum, size);
	// ÿ��span�п��е��ڴ���ǲ�ȷ���ģ���������һ��
	assert(actualNum > 0);
	if (actualNum == 1)
	{
		// ��CentralCache��ֻ��ȡ��һ���ڴ��
		assert(start == end);
		return start;
	}
	else
	{
		// ��CentralCache��ȡ������ڴ��
		_freeLists[index].pushRange(NextObj(start), end, actualNum - 1);  // �������ڴ��
		return start;
	}
}

// �����ڴ����
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	
	size_t alignSize = SizeClass::RoundUp(size);  // ��������С
	size_t index = SizeClass::Index(size);   // Ͱλ��

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].pop();
	}
	else
	{
		// ����һ������
		return FetchFromCentralCache(index, alignSize);
	}
}

// �ͷ�
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);
	
	// �ҳ�ӳ�����������Ͱ��ͷ��
	size_t index = SizeClass::Index(size);   // ����Ͱ
	_freeLists[index].push(ptr);

	// �������ȴ���һ������������ڴ�ʱ����central cache�ͷ�һ��list
	if (_freeLists[index].Size() >= _freeLists[index].GetMaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

// �ͷŶ���ʱ������̫��ʱ�������ڴ浽���뻺��
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;

	// ָ��sizeȫ������
	list.PopRange(start, end, list.GetMaxSize());

	// �ŵ�central cache
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
