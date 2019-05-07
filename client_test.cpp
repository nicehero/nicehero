#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include "Message.h"
#include "TestProtocol.h"
#include "Kcp.h"
class MyClient :public nicehero::TcpSessionC
{
public:
	void close();
};

void MyClient::close()
{
	nlog("client close");
	TcpSessionC::close();
}
int main()
{
	nicehero::start(true);
	std::vector<std::shared_ptr<MyClient> > cs;
	std::shared_ptr<nicehero::KcpSessionC> kcpc = std::make_shared<nicehero::KcpSessionC>();
	kcpc->connect("127.0.0.1", 7001);
	kcpc->init();
	kcpc->startRead();
	std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(nicehero::gService);
	t->expires_from_now(std::chrono::seconds(1));
	t->async_wait([kcpc](std::error_code ec) {
		Proto::XData xxx;
		xxx.n1 = 1;
		xxx.s1 = "666";
 		kcpc->sendMessage(xxx);
		std::shared_ptr<asio::steady_timer> t2 = std::make_shared<asio::steady_timer>(nicehero::gService);
		t2->expires_from_now(std::chrono::seconds(10));
		t2->async_wait([kcpc,t2](std::error_code ec) {
			if (ec)
			{
				nlogerr(ec.message().c_str());
			}
			Proto::XData xxx;
			xxx.n1 = 2;
			xxx.s1 = "66666666";
			kcpc->sendMessage(xxx);
		});
	});

	Proto::XData xxx;
	xxx.n1 = 1;
	xxx.s1 = "666";

	for (int i = 0;i < 1; ++ i)
	{
		std::shared_ptr<MyClient> c = std::make_shared<MyClient>();
		cs.push_back(c);
		nicehero::post([=] {
			c->connect("127.0.0.1", 7000);
			c->init();
			c->sendMessage(xxx);
			ui32 dat[2] = { 32,0 };
			*(ui16*)(dat + 1) = 101;
			for (int i = 0; i < 100; ++i)
			{
				nicehero::Message msg2(dat, 8);
				c->sendMessage(msg2);
			}
			*(ui16*)(dat + 1) = 102;
			nicehero::Message msg3(dat, 8);
			c->sendMessage(msg3);
			c->startRead();
		});
	}
	nicehero::gMainThread.join();
// 	asio::io_context io(1);
// 	asio::signal_set signals(io);
// 	signals.add(SIGINT);
// 	signals.add(SIGTERM);
// #if defined(SIGQUIT)
// 	signals.add(SIGQUIT);
// #endif
// 	signals.async_wait([&](std::error_code ec,int ic) {
// 		io.stop(); 
// 	});
// 	asio::io_context::work w(io);
// 	io.run();
	return 0;
}

using namespace Proto;

SESSION_COMMAND(MyClient, XDataID)
{
	nlog("recv XDataID size:%d", int(msg.getSize()));
	return true;
}
