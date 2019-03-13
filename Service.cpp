#include "Service.h"
#include <thread>

asio::io_context nicehero::gService(1);
asio::io_context nicehero::gWorkerService(nicehero::WORK_THREAD_COUNT);

void nicehero::StartService(bool background)
{

	for (size_t i = 0; i < WORK_THREAD_COUNT;++ i)
	{
		std::thread t([] {
			asio::io_context::work work(gWorkerService);
			gWorkerService.run();
		});
		t.detach();
	}

	if (background)
	{
		std::thread t([] {
			asio::io_context::work work(gService);
			gService.run();
		});
		t.detach();
	}
	else
	{
		asio::io_context::work work(gService);
		gService.run();
	}
}
