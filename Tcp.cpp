#include "Tcp.h"
#include "Service.h"

#include <asio/asio.hpp>
#include <micro-ecc/uECC.h>
#include "Log.h"
#include "Message.h"
#include "Clock.h"

extern "C"
{
	extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);
}

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

	void TcpServer::addSession(const tcpuid& uid, std::shared_ptr<TcpSession> session)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end())
		{
			it->second->m_impl->m_socket.close();
		}
		m_sessions[uid] = session;
	}

	void TcpServer::removeSession(const tcpuid& uid, ui64 serialID)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end() && it->second->m_serialID == serialID)
		{
			m_sessions.erase(uid);
		}
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
		ui8 buff[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = {0};
		memcpy(buff, server.m_publicKey, PUBLIC_KEY_SIZE);
		ui64 now = nNow;
		*(ui64*)(buff + PUBLIC_KEY_SIZE) = nNow;
		sha3(buff, PUBLIC_KEY_SIZE + 8, buff + PUBLIC_KEY_SIZE + 8, HASH_SIZE);
		m_hash = std::string((const char*)(buff + PUBLIC_KEY_SIZE + 8),HASH_SIZE);
		if (uECC_sign(server.m_privateKey
			, buff + PUBLIC_KEY_SIZE + 8, HASH_SIZE
			, buff + PUBLIC_KEY_SIZE + 8 + HASH_SIZE
			, uECC_secp256k1()) != 1
			) 
		{
			nlogerr("uECC_sign() failed\n");
			return;
		}
// 		if (uECC_verify(buff, (const ui8*)(buff + PUBLIC_KEY_SIZE + 8), HASH_SIZE, buff + PUBLIC_KEY_SIZE + 8 + HASH_SIZE, uECC_secp256k1()) != 1)
// 		{
// 			nlogerr("error check hash2");
// 		}

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
			ui8 data_[PUBLIC_KEY_SIZE + SIGN_SIZE] = "";
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
			bool allSame = true;
			for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++ i)
			{
				if (server.m_publicKey[i] != data_[i])
				{
					allSame = false;
					break;
				}
			}
			if (allSame)
			{
				return;
			}
			if (uECC_verify(data_, (const ui8*)m_hash.c_str(), HASH_SIZE, data_ + PUBLIC_KEY_SIZE, uECC_secp256k1()) != 1)
			{
				return;
			}
			self->m_uid = std::string((const char*)data_, PUBLIC_KEY_SIZE);
			static ui64 nowSerialID = 10000;
			self->m_serialID = nowSerialID++;
			nicehero::post([&,this, self] {
				server.addSession(self->m_uid, self);
				self->doRead();
			});

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

	void TcpSession::doRead()
	{
		auto self(shared_from_this());
	}

	TcpSessionC::TcpSessionC()
	{
		m_impl = std::make_shared<TcpSessionImpl>(*this);
	}

	TcpSessionC::~TcpSessionC()
	{

	}

	bool TcpSessionC::connect(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("TcpSessionC::TcpSessionC ip error:%s", ip.c_str());
			return false;
		}
		m_impl->m_socket.connect({addr,port} , ec);
		if (ec)
		{
			nlogerr("TcpSessionC::TcpSessionC connect error:%s", ec.message().c_str());
			return false;
		}
		return true;
	}

	void TcpSessionC::init(bool isSync)
	{
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(gWorkerService);
		auto f = [&, t](std::error_code ec) {
			t->cancel();
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket.read_some(
				asio::buffer(data_, sizeof(data_)), ec);
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			if (len < sizeof(data_))
			{
				nlogerr("server sign data len err");
				return;
			}
			if (checkServerSign(data_) == 1)
			{
				nlogerr("server sign err");
				return;
			}
			ui8 sendSign[PUBLIC_KEY_SIZE + SIGN_SIZE] = { 0 };
			memcpy(sendSign, m_publicKey, PUBLIC_KEY_SIZE);
			if (uECC_sign(m_privateKey
				, (const ui8*)data_ + PUBLIC_KEY_SIZE + 8, HASH_SIZE
				, sendSign + PUBLIC_KEY_SIZE
				, uECC_secp256k1()) != 1)
			{
				nlogerr("uECC_sign() failed\n");
				return;
			}
			m_uid = std::string((const char*)data_, PUBLIC_KEY_SIZE);
			static ui64 nowSerialID = 10000;
			m_serialID = nowSerialID++;
			m_impl->m_socket.write_some(asio::buffer(sendSign, PUBLIC_KEY_SIZE + SIGN_SIZE), ec);
			if (ec)
			{
				nlogerr("TcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			m_isInit = true;
		};
		if (isSync)
		{
			m_impl->m_socket.async_wait(
				asio::ip::tcp::socket::wait_read,f);
			t->expires_from_now(std::chrono::seconds(2));
			t->async_wait([&](std::error_code ec) {
				if (!ec)
				{
					m_impl->m_socket.close();
				}
			});
		}
		else
		{
			f(std::error_code());
		}
	}

	int TcpSessionC::checkServerSign(ui8* data_)
	{
		bool allSame = true;
		for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++i)
		{
			if (m_publicKey[i] != data_[i])
			{
				allSame = false;
				break;
			}
		}
		if (allSame)
		{
			nlogerr("same publicKey");
			return 1;
		}
		ui64& serverTime = *(ui64*)(data_ + PUBLIC_KEY_SIZE);
		int ret = 0;
		if (nNow > serverTime + 10 || nNow < serverTime - 10)
		{
			nlogerr("your time is diff from serverTime");
			ret = 2;
		}
		ui64 checkHash[HASH_SIZE / 8] = { 0 };
		sha3(data_, PUBLIC_KEY_SIZE + 8, checkHash, HASH_SIZE);
		allSame = true;
		for (size_t i = 0; i < HASH_SIZE / 8; ++i)
		{
			if (checkHash[i] != ((ui64*)(data_ + PUBLIC_KEY_SIZE + 8))[i])
			{
				allSame = false;
			}
		}
		if (!allSame)
		{
			nlogerr("error check hash");
			return 1;
		}
		if (uECC_verify(data_, (const ui8*)(data_ + PUBLIC_KEY_SIZE + 8), HASH_SIZE, data_ + PUBLIC_KEY_SIZE + 8 + HASH_SIZE, uECC_secp256k1()) != 1)
		{
			nlogerr("error check hash2");
			return 1;
		}
		return ret;
	}

}

