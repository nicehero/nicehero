#ifndef __NICE_CLOCK___
#define __NICE_CLOCK___
#include "Module.h"
namespace nicehero
{
	class Clock
		:public Module<Clock>
	{
	public:
		ui64 getTime();
		ui64 getTimeMS();
	};


}

#define nNow nicehero::Clock::getInstance()->getTime()
#endif
