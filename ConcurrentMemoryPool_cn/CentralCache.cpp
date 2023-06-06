#include "CentralCache.hpp"
#include "PageCache.hpp"

CentralCache CentralCache::_sInst;

// ��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	// ������ǰspanlist���ҷǿ�span
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

	// �Ȱ�central cache��Ͱ���������������߳��ͷ��ڴ棬��������
	list.GetMtx().unlock();

	// û�п��е�span������һ��page cache����span
	PageCache::GetInstance()->GetPageMtx().lock();  // ������һ��ǰ������

	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	
	PageCache::GetInstance()->GetPageMtx().unlock();   // ����

	// �Ի�ȡ��span�����з֣�����Ҫ��������Ϊ��ʱ�����߳��޷��������span
	
	// ����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С���ֽ�����
	char* start = (char*)(span->_pageID << PAGE_SHIFT);  // spanҳ�� * 8k == span��ʼ��ַ 
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	// �Ѵ���ڴ��г�С�飬��Ϊ����������������
	// 1.����һ����������ͷ������֮��β��
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	// 2.β��
	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);   // tail = start;
		start += size;
	}

	// 3.��spanͷ�嵽central cache��Ͱ��
	// ����central cacheǰ������
	list.GetMtx().lock();
	list.PushFront(span);

	return span;
}

// �����뻺���ȡn�������Ķ����thread cache
size_t CentralCache::FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);  // Ͱλ��
	
	// ���̷߳���ͬһ��Ͱ������
	_spanLists[index].GetMtx().lock();

	Span* span = GetOneSpan(_spanLists[index], size);  // ��һ���ǿյ�span
	assert(span);
	assert(span->_freeList);

	// ���ض���span�У��������е�ͷβָ��ȡ��(��ȡbatchNum������)
	// �������batchNum�������ж���ȡ���ٸ���
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;   // ʵ�ʻ�ȡ�����ڴ������
	while (i < batchNum - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	span->_useCount += actualNum;   // ��¼���б����ߵ��ڴ������


	_spanLists[index].GetMtx().unlock();  // ����
	
	return actualNum;
}

// ��һ�������Ķ����ͷŵ�span���
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);  // Ͱλ��
	_spanLists[index].GetMtx().lock();      // ����



	_spanLists[index].GetMtx().unlock();      // ����

}
