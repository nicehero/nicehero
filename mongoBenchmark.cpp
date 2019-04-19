#include "mongoBenchmarkImpl.cpp"
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		nlogerr("./mongoBenchmark threadNum");
	}
	benchmark(atoi(argv[1]));
	return 0;
}