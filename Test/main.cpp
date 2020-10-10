#include <iostream>

#include "Singleton.h"
#include "ThreadPool.h"





int main() 
{
	Singleton<ThreadPool>::GetInstance().ExecuteTask(
		[] 
		{
			std::cout << "task run!" << std::endl;
		});




	return EXIT_SUCCESS;
}