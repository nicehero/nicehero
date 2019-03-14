#ifndef ____SERVICE___
#define ____SERVICE___

#include <functional>

namespace asio
{
	class io_context;
}

namespace nicehero
{
	extern asio::io_context gService;
	extern asio::io_context gWorkerService;
	const int WORK_THREAD_COUNT = 8;
	void start(bool background = false);
	void stop();
	void post(std::function<void()> f);
}

#endif
