#ifndef ____NICE_COMMON___
#define ____NICE_COMMON___
#include <string>
namespace nicehero
{
	inline int StrCaseCmp(const std::string& a, const std::string& b) //same return 0
	{
		if (a.size() != b.size())
		{
			return 1;
		}
		for (size_t i = 0; i < a.size(); ++ i)
		{
			char x = a[i];
			char y = b[i];
			if (a[i] >= 'A' && a[i] <= 'Z')
			{
				x = (a[i] - ('A' - 'a'));
			}
			if (b[i] >= 'A' && b[i] <= 'Z')
			{
				y = (b[i] - ('A' - 'a'));
			}
			if (x != y)
			{
				return 1;
			}
		}
		return 0;
	}
}

#endif