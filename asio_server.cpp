// asio_server.cpp : Defines the entry point for the console application.
//

#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#ifdef WIN32
#include <windows.h>
#endif
#include <chrono>
#include <iomanip>

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
int main(int argc, char* argv[])
{
	fffff();
	int i = 0;
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
	nicehero::start();
	return 0;
}

SESSION_COMMAND(MyClient, 100)
{
	return true;
}

static int recv101Num = 0;
SESSION_COMMAND(MyClient, 101)
{
	++recv101Num;
	return true;
}

SESSION_COMMAND(MyClient, 102)
{
	return true;
}

