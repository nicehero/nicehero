#include "Service.h"
#include <thread>
#include <asio/asio.hpp>
#include <random>

asio::io_context nicehero::gService(1);
asio::io_context nicehero::gWorkerServices[nicehero::WORK_THREAD_COUNT];
std::thread nicehero::gMainThread;
void nicehero::start(bool background)
{
	for (int i = 0; i < WORK_THREAD_COUNT;++ i)
	{
		std::thread t([i] {
			asio::io_context::work work(gWorkerServices[i]);
			gWorkerServices[i].run();
		});
		t.detach();
	}

	if (background)
	{
		std::thread t([] {
			asio::io_context::work work(gService);
			gService.run();
		});
		gMainThread.swap(t);
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
	for (int i = 0; i < WORK_THREAD_COUNT; ++i)
	{
		gWorkerServices[i].stop();
	}
}

void nicehero::post(std::function<void()> f)
{
	gService.post(f);
}

asio::io_context& nicehero::getWorkerService()
{
	static std::default_random_engine e;
	return gWorkerServices[e() % WORK_THREAD_COUNT];
}
