#include "PageCache.hpp"

// ����PageCache
PageCache PageCache::_sInst;

// ��ȡһ����Kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k < N_PAGES);

	// ��k��Ͱ�У���û��span
	if (!_spanLists[k].Empty())
	{
		return _spanLists[k].PopFront(); 
	}
	
	// ����������Ͱ�У���û��span������У������з֣�
	for (size_t i = k + 1; i < N_PAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			// �зִ���ڴ��span
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			// ��nSpan��ͷ����һ��kҳ����
			// kҳ��span����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			// n-kҳ��spanͷ�嵽��Ӧ��λ��
			_spanLists[nSpan->_n].PushFront(nSpan);

			// �洢nSpan����βҳ�Ÿ�nSpanӳ�䣬�������page cache�����ڴ�ʱ���кϲ�����
			_idSpanMap[nSpan->_pageId] = nSpan;                  // ��ҳ��
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;  // βҳ��


			// ����id��span��ӳ�䣬����central cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	// ����û�д�ҳ��span���������һ��N_PAGESҳ��span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(N_PAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;    // �ֽ���->ҳ��
	bigSpan->_n = N_PAGES - 1;

	// ��N_PAGESҳ��span���뵽��N_PAGESҳ���ٵݹ��Լ�
	_spanLists[bigSpan->_n].PushFront(bigSpan);
	
	return NewSpan(k);
}

// ��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);  // �ֽ���->ҳ��
	auto ret = _idSpanMap.find(id);             // ҳ��->��ӳ���span
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

// �ͷſռ�span�ص�page cache�� �ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// ��spanǰ���ҳ�����Խ��кϲ��������ڴ�����Ƭ����
	while (1)
	{
		PAGE_ID prveID = span->_pageId - 1;
		auto ret = _idSpanMap.find(prveID);
		
		// ��ǰ�ϲ���ҳ��û���ˣ����ϲ�
		if (ret == _idSpanMap.end())       
		{
			break;
		}

		// ��ǰ����ҳ��span��ʹ���У����ϲ�
		Span* prevSpan = ret->second;
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// �ϲ�������128ҳ��spanû�취�������ϲ�
		if (prevSpan->_n + span->_n > N_PAGES - 1)
		{
			break;
		}

		// ��ǰ�ϲ�����ڴ�
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);   // �Ӷ�Ӧ��Ͱ��ɾ��

		delete prevSpan;
	}

	// ���ϲ�
	while (1)
	{
		PAGE_ID nextID = span->_pageId + span->_n;
		auto ret = _idSpanMap.find(nextID);
		// ����Ҳ���
		if (ret == _idSpanMap.end())
		{
			break;
		}

		// ��ʹ��
		Span* nextSpan = ret->second;
		if (nextSpan->_isUse == true)
		{
			break;
		}

		// ����128ҳ
		if (nextSpan->_n + span->_n > N_PAGES - 1)
		{
			break;
		}

		// �ϲ�
		span->_n += nextSpan->_n;

		_spanLists[nextSpan->_n].Erase(nextSpan);   // �Ӷ�Ӧ��Ͱ��ɾ��
		
		delete nextSpan;
	}

	// �ϲ����span������
	_spanLists[span->_n].PushFront(span);
	span->_isUse = false;
	
	// ��Ӻϲ����ӳ��
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_n - 1] = span;
}