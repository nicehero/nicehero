#include "Kcp.h"
#include "Service.h"

#include <asio/asio.hpp>
#include <micro-ecc/uECC.h>
#include "Log.h"
#include "Message.h"
#include "Clock.h"
#include <map>
#include <asio/yield.hpp>
#include <kcp/ikcp.h>

extern "C"
{
	extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);
}

namespace nicehero
{
	KcpMessageParser& getKcpMessagerParse(const std::type_info& typeInfo)
	{
		static std::map<const std::type_info*, KcpMessageParser> gKcpMessageParse;
		return gKcpMessageParse[&typeInfo];
	}
	class KcpSessionImpl
	{
	public:
		KcpSessionImpl(KcpSession& session)
			:m_session(session),m_iocontext(getWorkerService())
		{
			m_createTime = Clock::getInstance()->getTime();
		}
		~KcpSessionImpl()
		{
			if (m_kcp)
			{
				ikcp_release(m_kcp);
				m_kcp = nullptr;
			}
		}
		void kcpUpdate()
		{
			ui64 milliSec = Clock::getInstance()->getMilliSeconds();
			if (milliSec >= m_kcpUpdateTime)
			{
				IUINT32 current = (IUINT32)milliSec;
				ikcp_update(m_kcp, current);
				IUINT32 next = ikcp_check(m_kcp, current);
				m_kcpUpdateTime = milliSec + (next - current);
			}
		}
		int kcpRecv(ikcpcb *kcp, unsigned char *buffer, int len)
		{
			return ikcp_recv(m_kcp, (char*)buffer, len);
		}
		static int udpOutput(const char *buf, int len, ikcpcb *kcp, void *user);
		std::shared_ptr<asio::ip::udp::socket> m_socket;
		asio::ip::udp::endpoint m_endpoint;
		KcpSession& m_session;
		ikcpcb* m_kcp = nullptr;
		asio::io_context& m_iocontext;
		ui64 m_kcpUpdateTime = 0;
		ui64 m_createTime;
		bool m_inited = false;
	};

	class KcpServerImpl
	{
public:
		KcpServerImpl(asio::ip::address ip,ui16 port,KcpServer& server_)
			:m_server(server_), m_ip(ip), m_port(port)
		{
			m_socket = std::make_shared<asio::ip::udp::socket>(gMultiWorkerService, asio::ip::basic_endpoint<asio::ip::udp>(m_ip, m_port));
		}
		~KcpServerImpl()
		{
			nlogerr("~KcpServerImpl()");
		}
		void accept()
		{
			for (size_t i = 0; i < WORK_THREAD_COUNT; ++ i)
			{
				std::shared_ptr<std::string> buffer(new std::string(NETWORK_BUF_SIZE,0));
				std::shared_ptr<asio::ip::udp::endpoint> senderEndpoint(new asio::ip::udp::endpoint());
				startReceive(buffer, senderEndpoint,m_socket);
			}
		}
		void startReceive(std::shared_ptr<std::string> buffer
			, std::shared_ptr<asio::ip::udp::endpoint> senderEndpoint
			, std::shared_ptr<asio::ip::udp::socket> s)
		{
			s->async_receive_from(asio::buffer(const_cast<char *>(buffer->c_str()), buffer->length())
				, asio::ip::basic_endpoint<asio::ip::udp>(m_ip, m_port),
				[=](asio::error_code ec, size_t bytesRecvd) 
			{
				if (bytesRecvd < IKCP_OVERHEAD)
				{
					std::shared_ptr<KcpSession> ks = std::shared_ptr<KcpSession>(m_server.createSession());
					KcpSessionS* ss = dynamic_cast<KcpSessionS*>(ks.get());
					if (ss)
					{
						ss->m_KcpServer = &m_server;
						ss->m_MessageParser = &getKcpMessagerParse(typeid(*ss));
					}
					ks->m_impl->m_endpoint = *senderEndpoint;
					ks->m_impl->m_socket = m_socket;
					nicehero::post([&, ks]() {
						ks->init(m_server);
					});
				}
				startReceive(buffer, senderEndpoint, s);
			});
		}
		KcpServer& m_server;
		std::shared_ptr<asio::ip::udp::socket> m_socket;
		asio::ip::address m_ip;
		ui16 m_port;
	};

	KcpServer::KcpServer(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("KcpServer::KcpServer ip error:%s", ip.c_str());
		}
		try
		{
			m_impl = std::make_shared<KcpServerImpl>(addr, port,*this);
		}
		catch (asio::system_error & ec)
		{
			nlogerr("cannot open %s:%d", ip.c_str(), int(port));
			nlogerr("%s",ec.what());
		}
	}

	KcpServer::~KcpServer()
	{
		
	}

	KcpSessionS* KcpServer::createSession()
	{
		return new KcpSessionS();
	}

	void KcpServer::addSession(const kcpuid& uid, std::shared_ptr<KcpSession> session)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end())
		{
			it->second->close();
		}
		m_sessions[uid] = session;
	}

	void KcpServer::removeSession(const kcpuid& uid, ui64 serialID)
	{
		auto it = m_sessions.find(uid);
		if (it != m_sessions.end() && it->second->m_serialID == serialID)
		{
			it->second->close();
			m_sessions.erase(uid);
		}
	}

	void KcpServer::accept()
	{
		m_impl->accept();
	}

	ui32 KcpServer::getFreeUid()
	{
		if (m_sessions.size() >= m_maxSessions)
		{
			return INVALID_CONV;
		}
		return 1;
		/*
		for (;;)
		{
			if (m_nextConv == INVALID_CONV)
			{
				++m_nextConv;
			}
			if (m_sessions.find(m_nextConv) == m_sessions.end())
			{
				return m_nextConv++;
			}
			++m_nextConv;
		}
		*/
	}

	KcpSessionS::KcpSessionS()
	{

	}

	KcpSessionS::~KcpSessionS()
	{

	}

	void KcpSessionS::init(KcpServer& server)
	{
		m_conv = server.getFreeUid();
		if (m_conv == INVALID_CONV)
		{
			return;
		}
		init_kcp();
		auto self(shared_from_this());

		ui8 buff[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = {0};
		memcpy(buff, server.m_publicKey, PUBLIC_KEY_SIZE);
		ui64 now = nNow;
		*(ui64*)(buff + PUBLIC_KEY_SIZE) = now;
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
		ikcp_send(m_impl->m_kcp, (const char*)buff, sizeof(buff));
		init2(server);
// 		m_impl->m_socket->async_send(
// 			asio::buffer(buff, sizeof(buff)), 
// 			[&,self](std::error_code ec,size_t s) {
// 			if (ec)
// 			{
// 				nlogerr("%d\n", ec.value());
// 				return;
// 			}
// 			self->init2(server);
// 		});
	}

	void KcpSessionS::init2(KcpServer& server)
	{
		auto self(shared_from_this());
		m_impl->m_iocontext.post([&, self] {
			m_impl->kcpUpdate();
			unsigned char data_[PUBLIC_KEY_SIZE + SIGN_SIZE];
			int len = m_impl->kcpRecv(m_impl->m_kcp, data_, NETWORK_BUF_SIZE);
			if (len > 0)
			{
				if (len < sizeof(data_))
				{
					return;
				}
				bool allSame = true;
				for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++i)
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
				nicehero::post([&, this, self] {
					server.addSession(m_uid, self);
					doRead();
				});
			}
			else
			{
				init2(server);
				return;
			}
		});
		/*
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(gMultiWorkerService);
		m_impl->m_socket->async_wait(
			asio::ip::udp::socket::wait_read,
			[&, self,t](std::error_code ec)	{
			t->cancel();
			if (ec)
			{
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket->receive(
				asio::buffer(data_, sizeof(data_)));
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
			m_uid = 1;
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
				nlog("session connecting timeout");
				self->close();
			}
		});
		*/
	}


	void KcpSessionS::removeSelf()
	{
		auto self(shared_from_this());
		nicehero::post([&,self] {
			removeSelfImpl();
		});
	}

	void KcpSessionS::removeSelfImpl()
	{
		if (m_KcpServer)
		{
			m_KcpServer->removeSession(m_uid, m_serialID);
		}
	}

	KcpSession::KcpSession()
	{
		m_impl = std::make_shared<KcpSessionImpl>(*this);
		m_IsSending = false;
	}

	void KcpSession::init(KcpServer& server)
	{

	}

	void KcpSession::init()
	{

	}

	void KcpSession::init2(KcpServer& server)
	{

	}

	void KcpSession::doRead()
	{
		auto self(shared_from_this());
		m_impl->m_iocontext.post([&, self] {
			m_impl->kcpUpdate();
			unsigned char data_[NETWORK_BUF_SIZE];
			int len = m_impl->kcpRecv(m_impl->m_kcp,data_, NETWORK_BUF_SIZE);
			if (len > 0)
			{
				if (!parseMsg(data_, len))
				{
					self->removeSelf();
					return;
				}
			}
			doRead();
		});
/*
		this->m_impl->m_socket->async_wait(asio::ip::udp::socket::wait_read,
			[=](std::error_code ec) {
			if (ec)
			{
				self->removeSelf();
				return;
			}
			unsigned char data_[NETWORK_BUF_SIZE];
			ui32 len = (ui32)self->m_impl->m_socket->receive(asio::buffer(data_));
			if (ec)
			{
				self->removeSelf();
				return;
			}
			if (len > 0)
			{
				if (!parseMsg(data_, len))
				{
					self->removeSelf();
					return;
				}
			}
			doRead();
		});*/
	}

	bool KcpSession::parseMsg(unsigned char* data, ui32 len)
	{
		if (len > (ui32)NETWORK_BUF_SIZE)
		{
			return false;
		}
		Message& prevMsg = m_PreMsg;
		auto self(shared_from_this());
		if (prevMsg.m_buff == nullptr)
		{
			if (len < 4)
			{
// 				nlog("KcpSession::parseMsg len < 4");
				memcpy(&prevMsg.m_writePoint, data, len);
				prevMsg.m_buff = (unsigned char*)&prevMsg.m_writePoint;
				prevMsg.m_readPoint = len;
				return true;
			}
			ui32 msgLen = *((ui32*)data);
			if (msgLen > MSG_SIZE)
			{
				return false;
			}
			if (msgLen <= len)
			{
				auto recvMsg = std::make_shared<Message>(data, *((ui32*)data));
				
// 				if (m_MessageParser && m_MessageParser->m_commands[recvMsg->getMsgID()] == nullptr)
// 				{
// 					nlogerr("KcpSession::parseMsg err 1");
// 				}

				nicehero::post([self,recvMsg] {
					self->handleMessage(recvMsg);
				});
				if (msgLen < len)
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
				prevMsg.m_writePoint = len;
				return true;
			}
		}
		ui32 msgLen = 0;
		ui32 cutSize = 0;
		if (prevMsg.m_buff == (unsigned char*)&prevMsg.m_writePoint)
		{
			if (prevMsg.m_readPoint + len < 4)
			{
// 				nlog("KcpSession::parseMsg prevMsg.m_readPoint + len < 4");
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
// 			ui32 oldWritePoint = 0;//test value
// 			oldWritePoint = prevMsg.m_writePoint;//test value
			memcpy(prevMsg.m_buff + prevMsg.m_writePoint, data + cutSize, msgLen - prevMsg.m_writePoint);
			data = data + cutSize + (msgLen - prevMsg.m_writePoint);
			len = len - cutSize - (msgLen - prevMsg.m_writePoint);
			auto recvMsg = std::make_shared<Message>();
			recvMsg->swap(prevMsg);
// 			if (m_MessageParser && m_MessageParser->m_commands[recvMsg->getMsgID()] == nullptr)
// 			{
// 				nlogerr("KcpSession::parseMsg err 2");
// 			}

			nicehero::post([=] {
				self->handleMessage(recvMsg);
			});
			if (len > 0)
			{
				return parseMsg( data, len);
			}
			return true;
		}
// 		nlog("KcpSession::parseMsg else");
		memcpy(prevMsg.m_buff + prevMsg.m_writePoint, data + cutSize, len - cutSize);
		prevMsg.m_writePoint += len - cutSize;
		return true;
	}

	void KcpSession::removeSelf()
	{
	}

	void KcpSession::removeSelfImpl()
	{

	}

	void KcpSession::handleMessage(std::shared_ptr<Message> msg)
	{
		if (m_MessageParser)
		{
			if (m_MessageParser->m_commands[msg->getMsgID()] == nullptr)
			{
				nlogerr("KcpSession::handleMessage undefined msg:%d", ui32(msg->getMsgID()));
				return;
			}
			m_MessageParser->m_commands[msg->getMsgID()](*this, *msg.get());
		}
	}

	void KcpSession::close()
	{
		m_uid = "";
	}

	void KcpSession::setMessageParser(KcpMessageParser* messageParser)
	{
		m_MessageParser = messageParser;
	}

	kcpuid& KcpSession::getUid()
	{
		return m_uid;
	}

	void KcpSession::doSend(Message& msg)
	{
		auto self(shared_from_this());
		std::shared_ptr<Message> msg_ = std::make_shared<Message>();
		msg_->swap(msg);
		m_impl->m_socket->get_io_service().post([this,self, msg_] {
			//same thread ,no need lock
			m_SendList.emplace_back();
			m_SendList.back().swap(*msg_);
			if (m_IsSending)
			{
				return;
			}
			doSend();
		});
	}

	void KcpSession::doSend()
	{
		auto self(shared_from_this());
		m_IsSending = true;
		while (m_SendList.size() > 0 && m_SendList.front().m_buff == nullptr)
		{
			m_SendList.pop_front();
		}
		if (m_SendList.empty())
		{
			m_IsSending = false;
			return;
		}
		ui8* data = m_SendList.front().m_buff;
		ui32 size_ = m_SendList.front().getSize();
		if (m_SendList.size() > 1 && size_ <= (ui32)NETWORK_BUF_SIZE)
		{
			ui8 data2[NETWORK_BUF_SIZE];
			size_ = 0;
			while (m_SendList.size() > 0)
			{
				Message& msg = m_SendList.front();
				if (size_ + msg.getSize() > (ui32)NETWORK_BUF_SIZE)
				{
					break;
				}
				memcpy(data2 + size_, msg.m_buff, msg.getSize());
				size_ += msg.getSize();
				m_SendList.pop_front();
			}
			m_impl->m_socket->async_send(asio::buffer(data2, size_)
				, [this,self, size_](asio::error_code ec, std::size_t s) {
				if (ec)
				{
					removeSelf();
					return;
				}
				if (s < size_)
				{
					nlogerr("async_write buffer(data2, size_) err s:%d < size_:%d", int(s), int(size_));
					removeSelf();
					return;
				}
				if (m_SendList.size() > 0)
				{
					doSend();
					return;
				}
				m_IsSending = false;
			});
		}
		else
		{
			m_impl->m_socket->async_send(
				 asio::buffer(data, size_)
				, [this, self, size_](asio::error_code ec, std::size_t s) {
				if (ec)
				{
					removeSelf();
					return;
				}
				if (s < size_)
				{
					nlogerr("async_write buffer(data2, size_) err s:%d < size_:%d", int(s), int(size_));
					removeSelf();
					return;
				}
				m_SendList.pop_front();
				if (m_SendList.size() > 0)
				{
					doSend();
					return;
				}
				m_IsSending = false;
			});
		}

	}

	void KcpSession::sendMessage(Message& msg)
	{
		doSend(msg);
	}

	void KcpSession::sendMessage(Serializable& msg)
	{
		Message msg_;
		msg.toMsg(msg_);
		sendMessage(msg_);
	}

	int KcpSessionImpl::udpOutput(const char *buf, int len, ikcpcb *kcp, void *user)
	{
		KcpSession* s = reinterpret_cast<KcpSession*>(user);
		std::shared_ptr<std::string> buffer(new std::string());
		buffer->assign(buf, len);
		s->m_impl->m_socket->async_send_to(
			asio::buffer(buffer->c_str(), len),
			s->m_impl->m_endpoint,
			[] (asio::error_code, std::size_t) {});
		return 0;
	}

	void KcpSession::init_kcp()
	{
		m_impl->m_kcp = ikcp_create(m_conv,this);
		m_impl->m_kcp->output = &KcpSessionImpl::udpOutput;
		// boot fast
		// param2 nodelay
		// param3 interval default 1ms
		// param4 resend 2
		// param5 disable congestion control
		ikcp_nodelay(m_impl->m_kcp, 1, 0, 2, 1);
	}

	KcpSessionC::KcpSessionC()
	{
		m_isInit = false;
		m_impl = std::make_shared<KcpSessionImpl>(*this);
	}

	KcpSessionC::~KcpSessionC()
	{

	}

	bool KcpSessionC::connect(const std::string& ip, ui16 port)
	{
		asio::error_code ec;
		auto addr = asio::ip::address::from_string(ip, ec);
		if (ec)
		{
			nlogerr("KcpSessionC::KcpSessionC ip error:%s", ip.c_str());
			return false;
		}
		m_impl->m_socket->connect({addr,port} , ec);
		if (ec)
		{
			nlogerr("KcpSessionC::KcpSessionC connect error:%s", ec.message().c_str());
			return false;
		}
		return true;
	}

	void KcpSessionC::init(bool isSync)
	{
		std::shared_ptr<asio::steady_timer> t = std::make_shared<asio::steady_timer>(getWorkerService());
		auto f = [&, t](std::error_code ec) {
			t->cancel();
			if (ec)
			{
				nlogerr("KcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			ui8 data_[PUBLIC_KEY_SIZE + 8 + HASH_SIZE + SIGN_SIZE] = "";
			std::size_t len = m_impl->m_socket->receive(
				asio::buffer(data_, sizeof(data_)));
			if (ec)
			{
				nlogerr("KcpSessionC::init err %s", ec.message().c_str());
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
			m_impl->m_socket->send(asio::buffer(sendSign, PUBLIC_KEY_SIZE + SIGN_SIZE));
			if (ec)
			{
				nlogerr("KcpSessionC::init err %s", ec.message().c_str());
				return;
			}
			m_isInit = true;
			m_MessageParser = &getKcpMessagerParse(typeid(*this));
		};
		if (isSync)
		{
			m_impl->m_socket->async_wait(
				asio::ip::udp::socket::wait_read,f);
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


	void KcpSessionC::startRead()
	{
		doRead();
	}

	void KcpSessionC::removeSelf()
	{
		close();
	}

	int KcpSessionC::checkServerSign(ui8* data_)
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

