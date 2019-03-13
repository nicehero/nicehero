#ifndef ____SERVICE___
#define ____SERVICE___

#include "dep/asio/include/asio.hpp"

namespace nicehero
{
	extern asio::io_context gService;
	extern asio::io_context gWorkerService;
	const int WORK_THREAD_COUNT = 8;
	void StartService(bool background = false);
}

#endif
