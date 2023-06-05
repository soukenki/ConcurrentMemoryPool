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

	// û�зǿյ�span������һ��page cache�����ڴ�
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));

	return nullptr;
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


	_spanLists[index].GetMtx().unlock();  // ����
	
	return actualNum;
}