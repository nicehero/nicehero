#ifndef ____SERVICE___
#define ____SERVICE___

#include <functional>

namespace std
{
	class thread;
};
namespace asio
{
	class io_context;
}

namespace nicehero
{
	const int WORK_THREAD_COUNT = 8;
	extern asio::io_context gService;
	extern asio::io_context gWorkerServices[nicehero::WORK_THREAD_COUNT];
	extern std::thread gMainThread;
	void start(bool background = false);
	void stop();
	void post(std::function<void()> f);
	asio::io_context& getWorkerService();
}

#endif
