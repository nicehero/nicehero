#include "TcpServer.h"
#include "Service.h"

#include "dep/asio/include/asio.hpp"
#include "Log.h"

namespace nicehero
{
	class TcpSessionImpl
	{
	public:
		TcpSessionImpl(TcpSession& session)
			:m_socket(gWorkerService),m_session(session)
		{

		}
		asio::ip::tcp::socket m_socket;
		TcpSession& m_session;
	};
	class TcpServerImpl
	{
	public:
		TcpServerImpl(asio::ip::address ip,ui16 port,TcpServer& server_)
			:m_acceptor(gWorkerService,{ ip,port }),m_server(server_)
		{
			accept();
		}
		void accept()
		{
			std::shared_ptr<TcpSession> s = std::shared_ptr<TcpSession>(m_server.createSession());
			m_acceptor.async_accept(s->m_impl->m_socket,[=](std::error_code ec) {

			});
		}
		asio::ip::tcp::acceptor m_acceptor;
		TcpServer& m_server;
	};

	TcpServer::TcpServer(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlog("TcpServer::TcpServer ip error:%s", ip.c_str());
		}
		m_impl = std::make_shared<TcpServerImpl>(addr, port,*this);
	}

	TcpServer::~TcpServer()
	{
		
	}

	TcpSessionS* TcpServer::createSession()
	{
		return new TcpSessionS();
	}

	TcpSessionS::TcpSessionS()
	{

	}

	TcpSession::TcpSession()
	{
		m_impl = std::make_shared<TcpSessionImpl>(*this);
	}

}

