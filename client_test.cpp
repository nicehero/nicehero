#include "Service.h"
#include "Tcp.h"
#include <micro-ecc/uECC.h>
#include "Log.h"
#include <asio/asio.hpp>
#include <windows.h>


int main()
{
	nicehero::TcpSessionC c;
	c.connect("127.0.0.1", 7000);
	c.init();
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