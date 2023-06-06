
#include "ObjectPool.hpp"
#include "ConcurrentAlloc.hpp"


void Alloc1()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(6);  // 申请6字节
	}
}

void Alloc2()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(7);  // 申请7字节
	}
}

void TLSTest()
{
	std::thread t1(Alloc1);  // 创建线程t1执行Alloc1
	t1.join();

	std::thread t2(Alloc2);
	t2.join();
}

void TestConcurrentAlloc1()
{
	void* p1 = ConcurrentAlloc(6);
	void* p2 = ConcurrentAlloc(7);
	void* p3 = ConcurrentAlloc(8);
	void* p4 = ConcurrentAlloc(1);
	void* p5 = ConcurrentAlloc(8);

	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	cout << p4 << endl;
	cout << p5 << endl;
}

void TestConcurrentAlloc2()
{
	for (size_t i = 0; i < 1024; ++i)
	{
		void* p1 = ConcurrentAlloc(6);
		cout << p1 << endl;
	}
	void* p2 = ConcurrentAlloc(6);
	cout << p2 << endl;
}

// 测试地址映射
void TestAddressShift()
{
	PAGE_ID id1 = 2000;
	PAGE_ID id2 = 2001;

	char* p1 = (char*)(id1 << PAGE_SHIFT);
	char* p2 = (char*)(id2 << PAGE_SHIFT);

	while (p1 < p2)
	{
		cout << (void*)p1 << ":" << ((PAGE_ID)p1 >> PAGE_SHIFT) << endl;
		p1 += 8;
	}
}

void TestConcurrentAlloc3()
{
	void* p1 = ConcurrentAlloc(6);
	void* p2 = ConcurrentAlloc(7);
	void* p3 = ConcurrentAlloc(8);
	void* p4 = ConcurrentAlloc(1);
	void* p5 = ConcurrentAlloc(8);
	void* p6 = ConcurrentAlloc(8);
	void* p7 = ConcurrentAlloc(8);

	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	cout << p4 << endl;
	cout << p5 << endl;

	ConcurrentFree(p1, 6);
	ConcurrentFree(p2, 7);
	ConcurrentFree(p3, 8);
	ConcurrentFree(p4, 1);
	ConcurrentFree(p5, 8);
	ConcurrentFree(p6, 8);
	ConcurrentFree(p7, 8);
}


void MultiThreadAlloc1()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = ConcurrentAlloc(6);  // 申请6字节
		v.push_back(ptr);
	}

	for (auto e : v)
	{
		ConcurrentFree(e, 6);   // 释放
	}
}

void MultiThreadAlloc2()
{
	std::vector<void*> v;
	for (size_t i = 0; i < 7; ++i)
	{
		void* ptr = ConcurrentAlloc(16);  // 申请6字节
		v.push_back(ptr);
	}

	for (auto e : v)
	{
		ConcurrentFree(e, 16);   // 释放
	}
}

void TestMultiThread()
{
	std::thread t1(MultiThreadAlloc1);  // 创建线程t1执行MultiThreadAlloc1

	std::thread t2(MultiThreadAlloc2);
	
	t1.join(); 
	t2.join();
}
int main()
{
	// TestObjectPool();
	
	// TLSTest();
	
	// TestConcurrentAlloc1();
	// TestConcurrentAlloc2();

	// TestAddressShift();

	// TestConcurrentAlloc3();
	
	TestMultiThread();
	return 0;
}