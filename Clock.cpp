#include "Clock.h"
#include <time.h>
MODULE_IMPL(nicehero::Clock)

namespace nicehero
{
	ui64 Clock::getTime()
	{
		return time(0);
	}

	ui64 Clock::getTimeMS()
	{
		return time(0) * 1000;
	}

}
