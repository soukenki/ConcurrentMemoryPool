
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

int main()
{
	//TestObjectPool();
	
	TLSTest();
	
	return 0;
}