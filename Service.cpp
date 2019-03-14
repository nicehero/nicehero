#include "Service.h"
#include <thread>
#include <asio/asio.hpp>

asio::io_context nicehero::gService(1);
asio::io_context nicehero::gWorkerService(nicehero::WORK_THREAD_COUNT);

void nicehero::start(bool background)
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

void nicehero::stop()
{
	gService.stop();
	gWorkerService.stop();
}

void nicehero::post(std::function<void()> f)
{
	gService.post(f);
}
