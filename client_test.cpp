#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include "Message.h"

class MyClient :public nicehero::TcpSessionC
{
public:
	void close();
};

void MyClient::close()
{
	TcpSessionC::close();
}

int main()
{
	nicehero::start(true);
	std::shared_ptr<MyClient> c = std::make_shared<MyClient>();
	nicehero::post([=] {
		c->connect("127.0.0.1", 7000);
		c->init();
		ui32 dat[2] = { 32,0 };
		*(ui16*)(dat + 1) = 100;
		nicehero::Message msg(dat, 8);
		c->sendMessage(msg);
		*(ui16*)(dat + 1) = 101;
		for (int i = 0; i < 10000; ++i)
		{
			nicehero::Message msg2(dat, 8);
			c->sendMessage(msg2);
		}
		*(ui16*)(dat + 1) = 102;
		nicehero::Message msg3(dat, 8);
		c->sendMessage(msg3);
		c->startRead(); 
	});
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