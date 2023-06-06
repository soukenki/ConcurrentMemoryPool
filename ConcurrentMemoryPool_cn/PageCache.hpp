#pragma once

#include "Common.hpp"

class PageCache
{
public:
	// ��ȡ������PageCache
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// ��ȡһ��Kҳ��span
	Span* NewSpan(size_t k);

	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſռ�span�ص�page cache�� �ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);

	// ��ȡ��
	std::mutex& GetPageMtx()
	{
		return _pageMtx;
	}

private:
	SpanList _spanLists[N_PAGES];					// ��ϣͰ�ṹ
	std::mutex _pageMtx;							// PageCache����Ĵ���
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;  // pageID��span֮���ӳ��

private:
	PageCache()
	{}
	
	// ���ÿ�������
	PageCache(const PageCache& n) = delete;


	static PageCache _sInst;   // ����
};