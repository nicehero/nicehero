#include "TcpServer.h"
#include "Service.h"

#include <asio/asio.hpp>
#include <micro-ecc/uECC.h>
#include "Log.h"
#include "Message.h"

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
			:m_acceptor(gWorkerService,{ ip,port },false),m_server(server_)
		{
			accept();
		}
		~TcpServerImpl()
		{
			nlogerr("~TcpServerImpl()");
		}
		void accept()
		{
			std::shared_ptr<TcpSession> s = std::shared_ptr<TcpSession>(m_server.createSession());
			m_acceptor.async_accept(s->m_impl->m_socket,
				[this,s](std::error_code ec) {
				if (ec)
				{
					nlogerr(ec.message().c_str());
				}
				s->init(m_server);
				accept();
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
			nlogerr("TcpServer::TcpServer ip error:%s", ip.c_str());
		}
		try
		{
			m_impl = std::make_shared<TcpServerImpl>(addr, port,*this);
		}
		catch (asio::system_error &)
		{
			nlogerr("cannot open %s:%d",ip.c_str(),int(port));
		}
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

	TcpSessionS::~TcpSessionS()
	{

	}

	void TcpSessionS::init(TcpServer& server)
	{
		auto self(shared_from_this());
		ui8 buff[PUBLIC_KEY_SIZE + HASH_SIZE + SIGN_SIZE + 8] = {0};
		memcpy(buff, server.m_publicKey, PUBLIC_KEY_SIZE);
		//TODO HASH
		m_sign;
		if (!uECC_sign(server.m_privateKey
			, buff + PUBLIC_KEY_SIZE, sizeof(HASH_SIZE)
			, buff + PUBLIC_KEY_SIZE + HASH_SIZE
			, uECC_secp256k1())
			) 
		{
			nlogerr("uECC_sign() failed\n");
			return;
		}

		memcpy(buff + PUBLIC_KEY_SIZE, server.m_publicKey, HASH_SIZE);


		m_impl->m_socket.async_write_some(
			asio::buffer(buff, sizeof(buff)), 
			[&,self](std::error_code ec,size_t s) {
			if (ec)
			{
				nlogerr("%d\n", ec.value());
				return;
			}
			self->init2(server);
		});
	}

	void TcpSessionS::init2(TcpServer& server)
	{
		auto self(shared_from_this());
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(gWorkerService);
		self->m_impl->m_socket.async_wait(
			asio::ip::tcp::socket::wait_read,
			[&, self,t](std::error_code ec)	{
			t->cancel();
			if (ec)
			{
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + HASH_SIZE + SIGN_SIZE] = "";
			std::size_t len = self->m_impl->m_socket.read_some(
				asio::buffer(data_, sizeof(data_)), ec);
			if (ec)
			{
				return;
			}
			if (len < sizeof(data_))
			{
				return;
			}
			//TODO uECC_verify
			unsigned long long uid = 1;

		});
		t->expires_from_now(std::chrono::seconds(2));
		t->async_wait([self](std::error_code ec) {
			if (!ec)
			{
				self->m_impl->m_socket.close();
			}
		});

	}

	TcpSession::TcpSession()
	{
		m_impl = std::make_shared<TcpSessionImpl>(*this);
	}

	void TcpSession::init(TcpServer& server)
	{

	}

	void TcpSession::init()
	{

	}

	void TcpSession::init2(TcpServer& server)
	{

	}

}

