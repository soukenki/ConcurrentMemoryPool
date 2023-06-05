#pragma once

#include "Common.hpp"

class PageCache
{
public:
	// ��ȡ������PageCache
	static PageCache* GetInstance()
	{
		return &_sInstan;
	}

	// ��ȡһ��Kҳ��span
	Span* NewSpan();

private:
	SpanList _spanList[N_PAGES]; // 
	std::mutex _pageMtx;         // PageCache����Ĵ���

private:
	PageCache()
	{}
	
	// ���ÿ�������
	PageCache(const PageCache& n) = delete;


	static PageCache _sInstan;   // ����
};