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
	};


}
#endif
