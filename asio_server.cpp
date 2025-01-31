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
#include "Mongo.hpp"
#include "Clock.h"
#include "Kcp.h"
#include <kcp/ikcp.h>
#include "CopyablePtr.hpp"

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
			v += 1;//�޷�����״̬
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
void kcpTest()
{
	int iii = 0;
	auto kcp1 = ikcp_create(1, &iii);
	iii = ikcp_nodelay(kcp1, 1, 0, 2, 1);
	auto kcp2 = ikcp_create(1, &iii);
	iii = ikcp_nodelay(kcp2, 1, 0, 2, 1);
	IUINT32 current = (IUINT32)nicehero::Clock::getInstance()->getTimeMS();
	ikcp_update(kcp1, current);
	ikcp_check(kcp1, current);
	ikcp_release(kcp1);
	ikcp_release(kcp2);
}

class MyKcpSession : public nicehero::KcpSessionS
{
public:
};
class MyKcpServer :public nicehero::KcpServer
{
public:
	MyKcpServer(const std::string& ip, ui16 port)
		:KcpServer(ip, port)
	{}
	virtual nicehero::KcpSessionS* createSession();
};

nicehero::KcpSessionS* MyKcpServer::createSession()
{
	return new MyKcpSession();
}

auto g_deleter = [](int* p) {printf("g_deleter()\n"); delete p; p = nullptr; };
//void g_delete(int* p){ delete p; p = nullptr; }
class G
{
public:
	G()
		:m_u(new int(0), g_deleter)
	{

	}

	G(G&& other) : m_u(std::move(other.m_u)) 
	{
		printf("G( G&& g)\n");
	}

	G(const G& g)
		:m_u(std::move(g.m_u))
	{
		printf("G(const G& g)\n");
	}
	~G()
	{
		printf("~G()\n");
	}
	mutable std::unique_ptr<int,decltype(g_deleter) > m_u;
	char nnn[10] = "a\n";
};


#include "HttpServer.h"
#include <iostream>
int main(int argc, char* argv[])
{
	bool v6 = (argc > 1 && std::string(argv[1]) == "v6") ? true : false;
	kcpTest();
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
	std::string listenIP = "0.0.0.0";
	if (v6)
	{
		listenIP = "::";
	}
	nicehero::HttpServer httpServer(listenIP,8080);
	httpServer.addHandler("/", [] HTTP_HANDLER_PARAMS{
		res->write("hello world");
	});
	httpServer.start();
	MyServer tcpServer(listenIP, 7000);
	tcpServer.accept();
	MyKcpServer kcpServer(listenIP, 7001);
 	kcpServer.accept();
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
	{
		nicehero::CopyablePtr<G> ug(new G());
		nicehero::post([ug] {
			printf("nicehero::post [g]\n");
		});
	}
	nicehero::post([] {
		asio::ip::tcp::socket s(nicehero::gService);
		s.close();
		s.close();
		nlog("post test");
	});
	{
		nicehero::MongoConnectionPool pool;
		pool.init("mongodb://127.0.0.1/?authSource=admin", "easy");
		pool.insert("easy",
			NBSON_T(
				"_id", BCON_INT64(103)
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
		pool.update("easy",
			NBSON_T("_id", BCON_INT64(103)),
			NBSON_T("ar.0.hello", BCON_INT64(101),"bbb", BCON_INT64(102)));
		auto cursor = pool.find("easy", NBSON_T("_id", BCON_INT64(103)), nicehero::Bson(nullptr));
		while (auto r = cursor->fetch())
		{
			if (r->isInt64("oo.xhello"))
			{
				printf("oo.xhello: %d\n", int(r->asInt64("oo.xhello")));
			}
			if (r->isInt64("ar.0.hello"))
			{
				printf("ar.0.hello: %d\n", int(r->asInt64("ar.0.hello")));
			}
		}
		auto ccc = pool.popClient();
	}
	nicehero::gMainThread.join();
	return 0;
}
using namespace Proto;
TCP_SESSION_COMMAND(MyClient, XDataID)
{
	XData d;
	msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);

	nlog("tcp recv XData size:%d,%s", int(msg.getSize()),s.c_str());
	MyClient& client = (MyClient&)session;
	client.sendMessage(d);
	return true;
}

TCP_SESSION_COMMAND(MyClient, 101)
{
	MyClient& client = (MyClient&)session;
	//nlog("recv101 recv101Num:%d", client.recv101Num);
	++client.recv101Num;
	return true;
}
static int numClients = 0;

TCP_SESSION_COMMAND(MyClient, 102)
{
	MyClient& client = (MyClient&)session;
	++numClients;
	nlog("tcp recv102 recv101Num:%d,%d", client.recv101Num,numClients);
	return true;
}

KCP_SESSION_COMMAND(MyKcpSession, XDataID)
{
	XData d;
	msg >> d;
	d.s1 = "xxxx";
	std::string s;
	s.assign(d.s2.m_Data.get(), d.s2.m_Size);
	nlog("kcp recv XData size:%d,%s", int(msg.getSize()),s.c_str());
	
	session.sendMessage(d);
	return true;
}
