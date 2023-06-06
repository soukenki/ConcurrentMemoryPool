#pragma once


#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <thread>
#include <mutex>

#include <time.h>
#include <assert.h>


// 控制不同系统下的系统调用
#ifdef _WIN64
#include<windows.h>
#elif _WIN32
#include<windows.h>
#else
	// linux下brk mmap等函数的头文件
#endif

using std::cout;
using std::endl;

// 小于等于MAX_BYTES，找thread cache申请
// 大于MAX_BYTES，就直接找page cache或者系统堆申请
static const size_t MAX_BYTES = 256 * 1024;

// thread cache 和 central cache自由链表哈希桶的表大小
static const size_t N_FREELIST = 208;

// page cache 管理span list哈希表大小
static const size_t N_PAGES = 129;

// 字节数与页的转换 （字节数/8k == 右移13位）
static const size_t PAGE_SHIFT = 13;

// 控制指针大小
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#endif


// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN64
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等函数
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}


static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// 管理切分好的小对象的自由链表
class FreeList
{
public:
	// 插入单个
	void push(void* obj)
	{
		assert(obj);

		// 头插
		//*(void**)obj = _freeList;
		NextObj(obj) = _freeList;
		_freeList = obj;

		++_listSize;
	}

	// 插入多个
	void pushRange(void* start, void* end, size_t size)
	{
		NextObj(end) = _freeList;
		_freeList = start;

		_listSize += size;
	}

	// 删除单个
	void* pop()
	{
		assert(_freeList && _listSize != 0);

		// 头删
		void* obj = _freeList;
		_freeList = NextObj(obj);

		--_listSize;

		return obj;
	}

	// 删除多个
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

	// 判空
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
	void* _freeList = nullptr;   // 自由链表
	size_t _maxSize = 1;         // 慢开始大小，不断增大，直到上限NumMoveSize。
	size_t _listSize = 0;        // 链表现大小
};

// 计算对象大小的对齐映射规则
class SizeClass
{
public:
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128]               8byte对齐       freelist[0,16)
	// [128+1,1024]          16byte对齐      freelist[16,72)
	// [1024+1,8*1024]       128byte对齐     freelist[72,128)
	// [8*1024+1,64*1024]    1024byte对齐    freelist[128,184)
	// [64*1024+1,256*1024]  8*1024byte对齐  freelist[184,208)
	
	// 对齐数处理
	//size_t _RoundUp(size_t size, size_t alignNum)
	//{
	//	size_t alignSize;
	//	if (size % 8 != 0)
	//	{
	//		alignSize = (size / alignNum + 1) * alignNum;  // 向上对齐
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}
	//	return alignSize;
	//}
	
	static inline size_t _RoundUp(size_t bytes, size_t align)
	{
		return (((bytes)+align - 1) & ~(align - 1));    // 按位与，对齐
	}

	// 从对应的申请空间的大小中，找到对齐数
	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);  // 对齐数为8
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

	// 找出对应的桶
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
		// 位运算找到桶，因为是下标所以最后-1
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;  
	}

	// 计算映射的哪一个自由链表桶
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);
		
		// 每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) 
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) 
		{
			return _Index(bytes - 128, 4) + group_array[0];   // 加上group_array前面的桶数，等于当前桶的位置
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

	// thread cache一次从中央缓存获取多少个内存块
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);

		// [2,512] 一次批量移动多少个对象（慢启动）的上限值
		// 取内存越大块，取得越少。取内存越小块，取得越多。
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

	// 一次获取几个页
	// 单个对象 8byte
	// ...
	// 单个对象 256byte
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);    // 上层所需的内存块数量
		size_t npage = num * size;        // 字节数

		npage >>= PAGE_SHIFT;      // 字节数->页数
		if (0 == npage)
		{
			npage = 1;
		}

		return npage;
	}
};

// 管理多个连续页大块内存跨度结构
struct Span
{
	PAGE_ID _pageId = 0;         // 大块内存起始页的页号
	size_t _n = 0;               // 页的数量

	Span* _next = nullptr;       // 双向链表的结构
	Span* _prev = nullptr;

	size_t _useCount = 0;        // 切好小块内存，被分配给thread cache的计数
	void* _freeList = nullptr;   // 切好的小块内存的自由链表

	bool _isUse = false;               // 是否在被使用
};

// 带头双向循环链表
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	// 迭代器的头
	Span* Begin()
	{
		return _head->_next;
	}

	// 迭代器的尾
	Span* End()
	{
		return _head;
	}

	// 把span头插到桶中
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	// 头删一个桶中的span，并返回该span
	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}

	// pos位置前插入
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

	// pos位置删除
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

	// 判空
	bool Empty()
	{
		return _head->_next == _head;  // 哨兵位的头指向自己
	}

	// 获取锁
	std::mutex& GetMtx()
	{
		return _mtx;
	}

private:
	Span* _head;       // 哨兵位的头
	std::mutex _mtx;   // 桶锁
};