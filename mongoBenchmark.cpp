#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include <asio/yield.hpp>
#ifdef WIN32
#include <windows.h>
#endif
#include <chrono>
#include <iomanip>
#include "TestProtocol.h"
#include <mongoc/mongoc.h>
#include "Mongo.hpp"
#include "Clock.h"
#include "Kcp.h"
#include <kcp/ikcp.h>

int main(int argc, char* argv[])
{
	nicehero::start(true);
	nicehero::MongoConnectionPool pool;
	pool.init("mongodb://root:mztunv3wXYxbX@s-bp1711bd204bfe14.mongodb.rds.aliyuncs.com:3717,s-bp1f53f7e7932de4.mongodb.rds.aliyuncs.com:3717/?authSource=admin", "easy");
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	for (int i = 1;i <= 100000;++ i)
	{
		pool.insert("easy",
			NBSON_T(
				"_id", BCON_INT64(i)
				, "hello", BCON_UTF8("world")
				, "ar"
				, "["
					, "{"
						, "hello", BCON_INT64(666)
					, "}"
					, "world5"
					, BCON_DATE_TIME(nicehero::Clock::getInstance()->getTimeMS())
				, "]"
				, "oo"
				, "{"
					,"xhello", BCON_INT64(666)
				, "}"
				));
	}
	auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
	double qps = double(t) / 10000.0;
	nlog("qps:%.2lf",qps);
	nicehero::gMainThread.join();
	return 0;
}