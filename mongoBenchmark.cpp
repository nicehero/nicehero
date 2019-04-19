#include "mongoBenchmarkImpl.cpp"
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		nlogerr("need param: ./mongoBenchmark threadNum");
		return 0;
	}
	benchmark(atoi(argv[1]));
	return 0;
}