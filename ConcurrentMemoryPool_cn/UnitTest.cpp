
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

int main()
{
	// TestObjectPool();
	
	// TLSTest();
	
	// TestConcurrentAlloc1();
	TestConcurrentAlloc2();

	return 0;
}