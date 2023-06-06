#pragma once


#include "Common.hpp"

// ����ģʽ ����
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// ��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t size);

	// �����Ļ����ȡn�������Ķ����thread cache
	size_t FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);

private: 
	SpanList _spanLists[N_FREELIST];    // ��ϣͰ�ṹ��ÿ��Ͱ��N��span

private:
	// ���캯��˽��
	CentralCache()
	{}

	// ���ÿ�������
	CentralCache(const CentralCache& n) = delete;


	static CentralCache _sInst;  // CentralCache�ڴ��
};