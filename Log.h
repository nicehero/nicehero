#ifndef ____LOG____
#define ____LOG____
#include "Module.h"
namespace nicehero
{
	class Log:
		public Module<Log>
	{
	public:
		void log(const char *msg, ...);
		void logerr(const char *msg, ...);
	};
}
#define nlog Log::getInstance()->log
#define nlogerr Log::getInstance()->logerr
#endif // !____LOG____
