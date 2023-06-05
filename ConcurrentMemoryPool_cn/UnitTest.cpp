
#include "ObjectPool.hpp"
#include "ConcurrentAlloc.hpp"


void Alloc1()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(6);  // ����6�ֽ�
	}
}

void Alloc2()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(7);  // ����7�ֽ�
	}
}

void TLSTest()
{
	std::thread t1(Alloc1);  // �����߳�t1ִ��Alloc1
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