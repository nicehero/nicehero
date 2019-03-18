#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include "Message.h"
#include <windows.h>


int main()
{
	std::shared_ptr<nicehero::TcpSessionC> c = std::make_shared<nicehero::TcpSessionC>();
	std::shared_ptr<nicehero::TcpSession> c2(c->shared_from_this());
	c->connect("127.0.0.1", 7000);
	c->init();
	ui32 dat[2] = {8,0};
	*(ui16*)(dat + 1) = 100;
	nicehero::Message msg(dat, 8);
	c->sendMessage(msg);
	*(ui16*)(dat + 1) = 101;
	for (int i = 0; i < 10000; ++ i)
	{
		nicehero::Message msg2(dat, 8);
		c->sendMessage(msg2);
	}
	*(ui16*)(dat + 1) = 102;
	nicehero::Message msg3(dat, 8);
	c->sendMessage(msg3);
	nicehero::start();
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