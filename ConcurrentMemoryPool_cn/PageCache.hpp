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

	// ��ȡ��
	std::mutex& GetPageMtx()
	{
		return _pageMtx;
	}

private:
	SpanList _spanLists[N_PAGES];   // ��ϣͰ�ṹ
	std::mutex _pageMtx;           // PageCache����Ĵ���

private:
	PageCache()
	{}
	
	// ���ÿ�������
	PageCache(const PageCache& n) = delete;


	static PageCache _sInst;   // ����
};