// asio_server.cpp : Defines the entry point for the console application.
//

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
#include "Bson.hpp"
void some_sync(std::function<void()> f)
{
	f();
}

class A:public std::enable_shared_from_this<A>
{
	friend class B;
public:
	virtual ~A() {

	}
	int aa = 0;
protected:
	virtual void a() const{}
	void ca() {}
};
class B :public A
{
public:
	virtual ~B() {
		bb = 1;
	}
	void a() const{}
	void b() {
		auto self(shared_from_this());
		some_sync([] {
			A xa;
			xa.a();
		});
		xb();
		B bbb;
		bbb.xb();
	}
private:
	void xb() {}
	int bb = 0;
};
#include <map>

extern std::map<int,int> testG;
class G
{
public:
	G()
	{
// 		testG[5] = 5;
	}
};
G gg;
std::map<int, int> testG;

void testType(A* a, A* a2)
{
	const std::type_info& pa = typeid(*a);
	const std::type_info& pb = typeid(*a2);
	const std::type_info& pb2 = typeid(*a2);
	if (&pb == &pb2)
	{
		printf("&pb == &pb2\n");
	}
	printf("%s\n",pa.name());
	printf("%s\n", pb.name());
	printf("%s\n", pb2.name());
}


void lamdbaTest()
{
	
}

class MyClient :public nicehero::TcpSessionS
{
public:
	int recv101Num = 0;
};
class MyServer :public nicehero::TcpServer
{
public:
	MyServer(const std::string& ip, ui16 port)
		:TcpServer(ip, port)
	{}
	virtual nicehero::TcpSessionS* createSession();
};

nicehero::TcpSessionS* MyServer::createSession()
{
	return new MyClient();
}
static void fffff();
static void fffff()
{
	printf("fffff\n");
}

struct task : asio::coroutine
{
	bool operator()()
	{
		int v = 11;
		reenter(this)
		{
			printf("t1:%d\n",v);
			v += 1;//ÎÞ·¨±£´æ×´Ì¬
			yield return true;;
			printf("t2:%d\n", v);
			return true;
		}
		return true;
	}
};

namespace nnn {
	namespace {
		class noNameClass
		{
		public:
			noNameClass()
			{
				noName = 1;
			}
			int noName = 0;
		};
		noNameClass noNameObj;
	}
}

int main(int argc, char* argv[])
{
	task t;
	t();
	t();
	t();

	nicehero::start(true);
	fffff();
	int c = 0;
	uint8_t privateKey[32] = { 0 };
	uint8_t publicKey[64] = { 0 };
	uint8_t hash[32] = { 0 };
	uint8_t sig[64] = { 0 };

	const struct uECC_Curve_t * curves[5];
	int num_curves = 0;
#if uECC_SUPPORTS_secp256k1
	curves[num_curves++] = uECC_secp256k1();
#endif
	if (!uECC_make_key(publicKey, privateKey, curves[c])) {
		printf("uECC_make_key() failed\n");
		return 1;
	}
	memcpy(hash, publicKey, sizeof(hash));

	if (!uECC_sign(privateKey, hash, sizeof(hash), sig, curves[c])) {
		printf("uECC_sign() failed\n");
		return 1;
	}
#ifdef WIN32
	double dff;
	LARGE_INTEGER  large_interger;
	QueryPerformanceFrequency(&large_interger);
	dff = double(large_interger.QuadPart);
	LARGE_INTEGER  large_interger1;
	LARGE_INTEGER  large_interger2;
	QueryPerformanceCounter(&large_interger1);
	if (!uECC_verify(publicKey, hash, sizeof(hash), sig, curves[c])) {
		printf("uECC_verify() failed\n");
		return 1;
	}
	QueryPerformanceCounter(&large_interger2);
	__int64  x = large_interger2.QuadPart - large_interger1.QuadPart;
	printf("%lf\n", x * 1000 / dff);
#endif
 	auto tcpServer = MyServer("0.0.0.0", 7000);
	tcpServer.accept();
	auto privateKey1 = tcpServer.GetPrivateKeyString();
	tcpServer.SetPrivateKeyString(privateKey1);
	auto privateKey2 = tcpServer.GetPrivateKeyString();
	std::function<bool()> ff;
	if (!ff)
	{
		ff = [] {
			printf("ff\n");
			return true;
		};
		if (ff)
		{
			ff();
		}
	}
	nicehero::post([] {
		asio::ip::tcp::socket s(nicehero::gService);
		s.close();
		s.close();
		nlog("post test");
	});
	mongoc_init();
	auto uri = mongoc_uri_new("mongodb://dev:devv@192.168.9.5/?authSource=admin");
	mongoc_uri_set_appname(uri, "connect-example");
	mongoc_uri_set_option_as_int32(uri, MONGOC_URI_MAXPOOLSIZE, 100);
	auto pool = mongoc_client_pool_new(uri);
	bson_error_t error;
	if (pool)
	{
		auto client = mongoc_client_pool_pop(pool);
		if (client)
		{
			//mongoc_client_set_appname(client, "connect-example");
			auto database = mongoc_client_get_database(client, "testc");
			auto collection = mongoc_client_get_collection(client, "testc", "testcoll");
			if (database && collection)
			{
				auto insert = NBSON(
					"hello", BCON_UTF8("world")
					, "ar",
					"["
					, "{"
					, "hello", BCON_INT64(666)
					, "}"
					, "world5",
					"]"
					);
				std::string s = insert->asString("ar.1");
				mongoc_collection_insert(collection, MONGOC_INSERT_NONE, *insert, NULL, &error);
				auto cursor = mongoc_collection_find_with_opts(collection,
					*NBSON("_id", BCON_INT32(1)), nullptr, nullptr
					);
				const bson_t *doc = nullptr;
				while (mongoc_cursor_next(cursor, &doc)) {
					auto str = bson_as_canonical_extended_json(doc, nullptr);
					printf("%s\n", str);
					bson_free(str);
				}
				mongoc_cursor_destroy(cursor);
			}
			mongoc_collection_destroy(collection);
			mongoc_database_destroy(database);
			mongoc_client_pool_push(pool, client);
		}
		mongoc_client_pool_destroy(pool);
		mongoc_uri_destroy(uri);
	}
	mongoc_cleanup();
	nicehero::gMainThread.join();
	return 0;
}
using namespace Proto;
SESSION_COMMAND(MyClient, XDataID)
{
	nlog("recv100 size:%d",int(msg.getSize()));

	return true;
}

SESSION_COMMAND(MyClient, 101)
{
	MyClient& client = (MyClient&)session;
	//nlog("recv101 recv101Num:%d", client.recv101Num);
	++client.recv101Num;
	return true;
}
static int numClients = 0;

SESSION_COMMAND(MyClient, 102)
{
	MyClient& client = (MyClient&)session;
	++numClients;
	nlog("recv102 recv101Num:%d,%d", client.recv101Num,numClients);
	XData d;
	d.n1 = 2;
	d.s1 = "xxxx";
	client.sendMessage(d);
	return true;
}

