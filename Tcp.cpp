#include "Tcp.h"
#include "Service.h"

#include <asio/asio.hpp>
#include <micro-ecc/uECC.h>
#include "Log.h"
#include "Message.h"
#include "Clock.h"
#include <map>

extern "C"
{
	extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);
}

namespace nicehero
{
	static std::map<const std::type_info*, TcpMessageParser> gTcpMessageParse;
	TcpMessageParser& getMessagerParse(const std::type_info& typeInfo)
	{
		return gTcpMessageParse[&typeInfo];
	}
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
			TcpSessionS* ss = dynamic_cast<TcpSessionS*>(s.get());
			if (ss)
			{
				ss->m_TcpServer = &m_server;
				ss->m_MessageParser = &getMessagerParse(typeid(*ss));
			}
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
			it->second->close();
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
		m_impl->m_socket.async_wait(
			asio::ip::tcp::socket::wait_read,
			[&, self,t](std::error_code ec)	{
			t->cancel();
			if (ec)
			{
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket.read_some(
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
			m_uid = std::string((const char*)data_, PUBLIC_KEY_SIZE);
			static ui64 nowSerialID = 10000;
			m_serialID = nowSerialID++;
			nicehero::post([&,this, self] {
				server.addSession(m_uid, self);
				doRead();
			});

		});
		t->expires_from_now(std::chrono::seconds(2));
		t->async_wait([self](std::error_code ec) {
			if (!ec)
			{
				self->close();
			}
		});

	}


	void TcpSessionS::removeSelf()
	{
		auto self(shared_from_this());
		nicehero::post([&,self] {
			removeSelfImpl();
		});
	}

	void TcpSessionS::removeSelfImpl()
	{
		if (m_TcpServer)
		{
			m_TcpServer->removeSession(m_uid, m_serialID);
		}
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
		this->m_impl->m_socket.async_wait(asio::ip::tcp::socket::wait_read,
			[=](std::error_code ec) {
			if (ec)
			{
				printf("do_read async_wait error ec:%d,msg:%s\n", ec.value(), ec.message().c_str());
				self->removeSelf();
				return;
			}
			unsigned char data_[2048];
			ui32 len = (ui32)self->m_impl->m_socket.read_some(asio::buffer(data_), ec);
			if (ec)
			{
				printf("do_read async_wait error ec:%d,msg:%s\n", ec.value(), ec.message().c_str());
				self->removeSelf();
				return;
			}
			if (len > 0)
			{
				if (!parseMsg(data_, len))
				{
					printf("do_read async_wait pack_msg error ec:%d,msg:%s\n", ec.value(), ec.message().c_str());
					self->removeSelf();
					return;
				}
			}
			doRead();
		});
	}

	bool TcpSession::parseMsg(unsigned char* data, ui32 len)
	{
		if (len > 2048)
		{
			return false;
		}
		Message& prevMsg = m_PreMsg;
		auto self(shared_from_this());
		if (prevMsg.m_buff == nullptr)
		{
			if (len < 4)
			{
				memcpy(&prevMsg.m_writePoint, data, len);
				prevMsg.m_buff = (unsigned char*)&prevMsg.m_writePoint;
				prevMsg.m_readPoint = len;
				return true;
			}
			unsigned long msgLen = *((unsigned long*)data);
			if (msgLen > MSG_SIZE)
			{
				return false;
			}
			if (msgLen >= len)
			{
				auto recvMsg = std::make_shared<Message>(data, *((unsigned long*)data));
				
				nicehero::post([=] {
					self->handleMessage(recvMsg);
				});
				if (msgLen > len)
				{
					return parseMsg( data + msgLen, len - msgLen);
				}
				else
				{
					return true;
				}
			}
			else
			{
				prevMsg.m_buff = new unsigned char[msgLen];
				memcpy(prevMsg.m_buff, data, len);
				return true;
			}
		}
		unsigned long msgLen = 0;
		unsigned long cutSize = 0;
		if (prevMsg.m_buff == (unsigned char*)&prevMsg.m_writePoint)
		{
			if (prevMsg.m_readPoint + len < 4)
			{
				memcpy(((unsigned char*)&prevMsg.m_writePoint) + prevMsg.m_readPoint
					, data, len);
				prevMsg.m_readPoint = prevMsg.m_readPoint + len;
				return true;
			}
			cutSize = 4 - prevMsg.m_readPoint;
			memcpy(((unsigned char*)&prevMsg.m_writePoint) + prevMsg.m_readPoint
				, data, cutSize);
			msgLen = prevMsg.m_writePoint;
			prevMsg.m_buff = new unsigned char[msgLen];
			memcpy(prevMsg.m_buff, &msgLen, 4);
			prevMsg.m_readPoint = 4;
			prevMsg.m_writePoint = 4;
		}
		msgLen = prevMsg.getSize();
		if (msgLen > MSG_SIZE)
		{
			return false;
		}
		if (len + prevMsg.m_writePoint - cutSize >= msgLen)
		{
			memcpy(prevMsg.m_buff, data + cutSize, msgLen - prevMsg.m_writePoint);
			data = data + cutSize + (msgLen - prevMsg.m_writePoint);
			len = len - cutSize - (msgLen - prevMsg.m_writePoint);
			auto recvMsg = std::make_shared<Message>();
			recvMsg->swap(prevMsg);
			nicehero::post([=] {
				self->handleMessage(recvMsg);
			});
			if (len > 0)
			{
				return parseMsg( data, len);
			}
			return true;
		}
		memcpy(prevMsg.m_buff + prevMsg.m_writePoint, data + cutSize, len - cutSize);
		prevMsg.m_writePoint += len - cutSize;
		return true;
	}

	void TcpSession::removeSelf()
	{
	}

	void TcpSession::removeSelfImpl()
	{

	}

	void TcpSession::handleMessage(std::shared_ptr<Message> msg)
	{
		if (m_MessageParser)
		{
			m_MessageParser->m_commands[msg->getMsgID()](*this, *msg.get());
		}
	}

	void TcpSession::close()
	{
		m_impl->m_socket.close();
	}

	void TcpSession::setMessageParser(TcpMessageParser* messageParser)
	{
		m_MessageParser = messageParser;
	}

	std::string& TcpSession::getUid()
	{
		return m_uid;
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
			m_MessageParser = &getMessagerParse(typeid(*this));
		};
		if (isSync)
		{
			m_impl->m_socket.async_wait(
				asio::ip::tcp::socket::wait_read,f);
			t->expires_from_now(std::chrono::seconds(2));
			t->async_wait([&](std::error_code ec) {
				if (!ec)
				{
					close();
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

