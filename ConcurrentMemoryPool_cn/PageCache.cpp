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
			kSpan->_pageID = nSpan->_pageID;
			kSpan->_n = k;

			nSpan->_pageID += k;
			nSpan->_n -= k;

			// n-kҳ��spanͷ�嵽��Ӧ��λ��
			_spanLists[nSpan->_n].PushFront(nSpan);

			return kSpan;
		}
	}

	// ����û�д�ҳ��span���������һ��N_PAGESҳ��span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(N_PAGES - 1);
	bigSpan->_pageID = (PAGE_ID)ptr >> PAGE_SHIFT;    // �ֽ���->ҳ��
	bigSpan->_n = N_PAGES - 1;

	// ��N_PAGESҳ��span���뵽��N_PAGESҳ���ٵݹ��Լ�
	_spanLists[bigSpan->_n].PushFront(bigSpan);
	
	return NewSpan(k);
}