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

void benchmark_query(int threadNum, std::shared_ptr<nicehero::MongoConnectionPool> pool)
{
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	std::shared_ptr<int> xx = std::make_shared<int>(0);
	for (int j = 1; j <= threadNum; ++j)
	{
		nicehero::post([xx, j, pool, t1, threadNum] {
			int yy = 0;
			for (int i = 1; i <= 10 * (1000 / threadNum); ++i)
			{
				auto cursor = pool->find("easy", NBSON_T(
					"_id", BCON_INT64(j * 1000 + i))
					, nicehero::Bson(nullptr)
				);
				while (auto r = cursor->fetch())
				{
					++ yy;
				}
			}
			if (yy >= 10 * (1000 / threadNum))
			{
				nlog("aaaaaa");
				nicehero::post([pool, xx, t1, threadNum] {
					++(*xx);
					if (*xx >= threadNum)
					{
						auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
						double qps = 10000.0 / double(t)  * 1000.0;
						nlog("query qps:%.2lf", qps);
					}
				});
			}
		}, nicehero::TO_DB);
	}
}

void benchmark_insert(int threadNum,std::shared_ptr<nicehero::MongoConnectionPool> pool)
{
	auto t1 = nicehero::Clock::getInstance()->getMilliSeconds();
	std::shared_ptr<int> xx = std::make_shared<int>(0);
	for (int j = 1; j <= threadNum; ++j)
	{
		nicehero::post([xx, j, pool, t1, threadNum] {
			for (int i = 1; i <= 10 * (1000 / threadNum); ++i)
			{
				pool->insert("easy",
					NBSON_T(
						"_id", BCON_INT64(j * 1000 + i)
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
						, "xhello", BCON_INT64(666)
						, "}"
					));
			}
			nicehero::post([pool,xx, t1, threadNum] {
				++(*xx);
				if (*xx >= threadNum)
				{
					auto t = nicehero::Clock::getInstance()->getMilliSeconds() - t1;
					double qps = 10000.0 / double(t)  * 1000.0;
					nlog("insert qps:%.2lf", qps);
					benchmark_query(threadNum,pool);
				}
			});
		}, nicehero::TO_DB);
	}
}
int benchmark(int threadNum,const char* db)
{
	nicehero::start(true);
	std::shared_ptr<nicehero::MongoConnectionPool> pool = std::make_shared<nicehero::MongoConnectionPool>();
	pool->init(db, "easy");
	benchmark_insert(threadNum,pool);
	nicehero::gMainThread.join();
	return 0;
}