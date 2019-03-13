#include "Clock.h"
#include <time.h>
MODULE_IMPL(nicehero::Clock)

namespace nicehero
{
	ui64 Clock::getTime()
	{
		return time(0);
	}


}
