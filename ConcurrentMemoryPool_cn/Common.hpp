#pragma once


#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <thread>
#include <mutex>

#include <time.h>
#include <assert.h>


// ���Ʋ�ͬϵͳ�µ�ϵͳ����
#ifdef _WIN64
#include<windows.h>
#elif _WIN32
#include<windows.h>
#else
	// linux��brk mmap�Ⱥ�����ͷ�ļ�
#endif

using std::cout;
using std::endl;

// С�ڵ���MAX_BYTES����thread cache����
// ����MAX_BYTES����ֱ����page cache����ϵͳ������
static const size_t MAX_BYTES = 256 * 1024;

// thread cache �� central cache���������ϣͰ�ı��С
static const size_t N_FREELIST = 208;

// page cache ����span list��ϣ���С
static const size_t N_PAGES = 129;

// �ֽ�����ҳ��ת�� ���ֽ���/8k == ����13λ��
static const size_t PAGE_SHIFT = 13;

// ����ָ���С
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#endif


// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN64
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap�Ⱥ���
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}


static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// �����зֺõ�С�������������
class FreeList
{
public:
	// ���뵥��
	void push(void* obj)
	{
		assert(obj);

		// ͷ��
		//*(void**)obj = _freeList;
		NextObj(obj) = _freeList;
		_freeList = obj;

		++_listSize;
	}

	// ������
	void pushRange(void* start, void* end, size_t size)
	{
		NextObj(end) = _freeList;
		_freeList = start;

		_listSize += size;
	}

	// ɾ������
	void* pop()
	{
		assert(_freeList && _listSize != 0);

		// ͷɾ
		void* obj = _freeList;
		_freeList = NextObj(obj);

		--_listSize;

		return obj;
	}

	// ɾ�����
	void PopRange(void*& start, void*& end, size_t size)
	{
		assert(size >= _listSize);

		start = _freeList;
		end = start;

		for (size_t i = 0; i < size - 1; ++i)
		{
			end = NextObj(end);
		}

		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_listSize -= size;
	}

	// �п�
	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t& GetMaxSize()
	{
		return _maxSize;
	}

	size_t Size()
	{
		return _listSize;
	}

private:
	void* _freeList = nullptr;   // ��������
	size_t _maxSize = 1;         // ����ʼ��С����������ֱ������NumMoveSize��
	size_t _listSize = 0;        // �����ִ�С
};

// ��������С�Ķ���ӳ�����
class SizeClass
{
public:
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]               8byte����       freelist[0,16)
	// [128+1,1024]          16byte����      freelist[16,72)
	// [1024+1,8*1024]       128byte����     freelist[72,128)
	// [8*1024+1,64*1024]    1024byte����    freelist[128,184)
	// [64*1024+1,256*1024]  8*1024byte����  freelist[184,208)
	
	// ����������
	//size_t _RoundUp(size_t size, size_t alignNum)
	//{
	//	size_t alignSize;
	//	if (size % 8 != 0)
	//	{
	//		alignSize = (size / alignNum + 1) * alignNum;  // ���϶���
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}
	//	return alignSize;
	//}
	
	static inline size_t _RoundUp(size_t bytes, size_t align)
	{
		return (((bytes)+align - 1) & ~(align - 1));    // ��λ�룬����
	}

	// �Ӷ�Ӧ������ռ�Ĵ�С�У��ҵ�������
	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);  // ������Ϊ8
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8 * 1024);
		}
		else
		{
			assert(false);
			return -1;
		}
	}

	// �ҳ���Ӧ��Ͱ
	//size_t _Index(size_t bytes, size_t alignNum)
	//{
	//	if (bytes % alignNum == 0)
	//	{
	//		return bytes / alignNum - 1;
	//	}
	//	else
	//	{
	//		return bytes / alignNum;
	//	}
	//}

	// 1 + 7  8
	// 2      9
	// ...
	// 8      15

	// 9 + 7 16
	// 10
	// ...
	// 16    23

	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		// λ�����ҵ�Ͱ����Ϊ���±��������-1
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;  
	}

	// ����ӳ�����һ����������Ͱ
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);
		
		// ÿ�������ж��ٸ���
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) 
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) 
		{
			return _Index(bytes - 128, 4) + group_array[0];   // ����group_arrayǰ���Ͱ�������ڵ�ǰͰ��λ��
		}
		else if (bytes <= 8 * 1024) 
		{
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) 
		{
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1]
				+ group_array[0];
		}
		else if (bytes <= 256 * 1024) 
		{
			return _Index(bytes - 64 * 1024, 13) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		}
		else 
		{
			assert(false);
		}
		return -1;
	}

	// thread cacheһ�δ����뻺���ȡ���ٸ��ڴ��
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);

		// [2,512] һ�������ƶ����ٸ�������������������ֵ
		// ȡ�ڴ�Խ��飬ȡ��Խ�١�ȡ�ڴ�ԽС�飬ȡ��Խ�ࡣ
		size_t num = MAX_BYTES / size;
		if (num < 2)
		{
			num = 2;
		}
		if (num > 512)
		{
			num = 512;
		}
		return num;
	}

	// һ�λ�ȡ����ҳ
	// �������� 8byte
	// ...
	// �������� 256byte
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);    // �ϲ�������ڴ������
		size_t npage = num * size;        // �ֽ���

		npage >>= PAGE_SHIFT;      // �ֽ���->ҳ��
		if (0 == npage)
		{
			npage = 1;
		}

		return npage;
	}
};

// ����������ҳ����ڴ��Ƚṹ
struct Span
{
	PAGE_ID _pageId = 0;         // ����ڴ���ʼҳ��ҳ��
	size_t _n = 0;               // ҳ������

	Span* _next = nullptr;       // ˫������Ľṹ
	Span* _prev = nullptr;

	size_t _useCount = 0;        // �к�С���ڴ棬�������thread cache�ļ���
	void* _freeList = nullptr;   // �кõ�С���ڴ����������

	bool _isUse = false;               // �Ƿ��ڱ�ʹ��
};

// ��ͷ˫��ѭ������
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	// ��������ͷ
	Span* Begin()
	{
		return _head->_next;
	}

	// ��������β
	Span* End()
	{
		return _head;
	}

	// ��spanͷ�嵽Ͱ��
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	// ͷɾһ��Ͱ�е�span�������ظ�span
	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}

	// posλ��ǰ����
	void Insert(Span* pos, Span* newSpan)
	{
		assert(newSpan && pos);

		Span* prev = pos->_prev;
		
		// prev newspan pos
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}

	// posλ��ɾ��
	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;
		
		// prev pos next
		prev->_next = next;
		next->_prev = prev;
	}

	// �п�
	bool Empty()
	{
		return _head->_next == _head;  // �ڱ�λ��ͷָ���Լ�
	}

	// ��ȡ��
	std::mutex& GetMtx()
	{
		return _mtx;
	}

private:
	Span* _head;       // �ڱ�λ��ͷ
	std::mutex _mtx;   // Ͱ��
};