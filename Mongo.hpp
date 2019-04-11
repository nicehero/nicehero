#ifndef ___NICE__MONGO_HPP__
#define ___NICE__MONGO_HPP__

#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "Type.h"
#include <string>
#include <memory>
#include "Bson.hpp"
#include "Log.h"
#include <list>

namespace nicehero
{
	class MongoCursor
	{
		friend class MongoConnectionPool;
	protected:
		MongoCursor(int err, mongoc_cursor_t* cursor = nullptr);
	public:
		virtual ~MongoCursor();
		BsonPtr fetch();
		int m_err = 0;
		std::list<bson_t*> m_cursorImpl;
	};


	using MongoCursorPtr = std::shared_ptr<MongoCursor>;

	class MongoConnectionPool
	{
	public:
		virtual ~MongoConnectionPool();
		bool init(const std::string& mongoUrl
			, const std::string& dbname
			, int writeConcern = 1
			, const std::string& appName = std::string()
			);

		mongoc_uri_t* m_url = nullptr;
		mongoc_client_pool_t * m_poolImpl = nullptr;
		std::string m_dbname;

		bson_error_t insert(const std::string& collection
			, const Bson& doc
			, mongoc_insert_flags_t flags = MONGOC_INSERT_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		bson_error_t update(const std::string& collection
			, const Bson& query
			, const Bson& doc
			, mongoc_update_flags_t flags = MONGOC_UPDATE_NONE
			, bool specialWriteConcern = false
			, int writeConcern = 1
			);

		MongoCursorPtr find(const std::string& collection
			, const Bson& query
			, const Bson& opt
			, mongoc_read_mode_t readMode = MONGOC_READ_PRIMARY
			);
	};

	inline MongoConnectionPool::~MongoConnectionPool()
	{
		if (m_poolImpl)
		{
			mongoc_client_pool_destroy(m_poolImpl);
		}
		if (m_url)
		{
			mongoc_uri_destroy(m_url);
		}
	}

	inline bool MongoConnectionPool::init(const std::string& mongoUrl
		, const std::string& dbname
		, int writeConcern
		, const std::string& appName
		)
	{
		static bool firstInit = true;
		if (firstInit)
		{
			firstInit = false;
			mongoc_init();
		}
		m_url = mongoc_uri_new(mongoUrl.c_str());
		if (!m_url)
		{
			nlogerr("create mongo url err:%s", mongoUrl.c_str());
			return false;
		}
		if (appName != "")
		{
			if (!mongoc_uri_set_appname(m_url, appName.c_str()))
			{
				nlogerr("mongoc_uri_set_appname err:%s", appName.c_str());
				return false;
			}
		}
		if (!mongoc_uri_set_option_as_int32(m_url, MONGOC_URI_MAXPOOLSIZE, 100))
		{
			nlogerr("mongoc_uri_set_option_as_int32 err");
			return false;
		}
		auto write_concern = mongoc_write_concern_new();
		if (!write_concern)
		{
			nlogerr("mongoc_write_concern_new err");
			return false;
		}
		mongoc_write_concern_set_w(write_concern, writeConcern);
		mongoc_uri_set_write_concern(m_url, write_concern);
		mongoc_write_concern_destroy(write_concern);
		m_poolImpl = mongoc_client_pool_new(m_url);
		if (!m_poolImpl)
		{
			nlogerr("mongoc_client_pool_new err");
			return false;
		}
		auto client = mongoc_client_pool_pop(m_poolImpl);
		if (!client)
		{
			nlogerr("mongoc_client_pool_new2 err");
			return false;
		}
		bson_t ping = BSON_INITIALIZER;
		bson_error_t error;
		bool r;
		BSON_APPEND_INT32(&ping, "ping", 1);

		r = mongoc_client_command_simple(
			client, "admin", &ping, NULL, NULL, &error);

		if (!r) 
		{
			nlogerr("mongoc_client_pool_new3 %s", error.message);
			mongoc_client_pool_push(m_poolImpl,client);
			return false;
		}
		m_dbname = dbname;
		return true;
	}

	inline MongoCursor::MongoCursor(int err, mongoc_cursor_t* cursor /*= nullptr*/)
	{
		m_err = err;

		if (!cursor)
		{
			return;
		}
		const bson_t *doc = nullptr;
		while (mongoc_cursor_next(cursor, &doc))
		{
			if (doc)
			{
				bson_t * doc_ = bson_copy(doc);
				m_cursorImpl.push_back(doc_);
			}
		}
		mongoc_cursor_destroy(cursor);
	}

	inline MongoCursor::~MongoCursor()
	{
		for (auto v:m_cursorImpl)
		{
			if (v)
			{
				bson_destroy(v);
			}
		}
		m_cursorImpl.clear();
	}

	inline BsonPtr MongoCursor::fetch()
	{
		if (m_cursorImpl.size() < 1)
		{
			return BsonPtr();
		}
		auto ret = BsonPtr(new Bson(m_cursorImpl.front()));
		m_cursorImpl.pop_front();
		return ret;
	}

	inline bson_error_t MongoConnectionPool::insert(const std::string& collection, const Bson& doc, mongoc_insert_flags_t flags /*= MONGOC_INSERT_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 2, "db error");
		auto c = mongoc_client_pool_pop(m_poolImpl);
		if (!c)
		{
			return error;
		}
		auto collection_ = mongoc_client_get_collection(c, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			mongoc_client_pool_push(m_poolImpl, c);
			return error;
		}
		mongoc_write_concern_t* write_concern = nullptr;
		if (specialWriteConcern)
		{
			write_concern = mongoc_write_concern_new();
			if (!write_concern)
			{
				nlogerr("mongoc_write_concern_new err");
				return error;
			}
			mongoc_write_concern_set_w(write_concern, writeConcern);
		}
		if (!mongoc_collection_insert(collection_, MONGOC_INSERT_NONE, doc.m_bson, write_concern, &error))
		{
			if (write_concern)
			{
				mongoc_write_concern_destroy(write_concern);
			}
			mongoc_client_pool_push(m_poolImpl, c);
			return error;
		}
		if (write_concern)
		{
			mongoc_write_concern_destroy(write_concern);
		}
		mongoc_client_pool_push(m_poolImpl, c);
		return error;
	}


	inline bson_error_t MongoConnectionPool::update(const std::string& collection, const Bson& query, const Bson& obj, mongoc_update_flags_t flags /*= MONGOC_UPDATE_NONE */, bool specialWriteConcern /*= false */, int writeConcern /*= 1 */)
	{
		bson_error_t error;
		bson_set_error(&error, 0, 2, "db error");
		auto c = mongoc_client_pool_pop(m_poolImpl);
		if (!c)
		{
			return error;
		}
		auto collection_ = mongoc_client_get_collection(c, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			mongoc_client_pool_push(m_poolImpl, c);
			return error;
		}
		mongoc_write_concern_t* write_concern = nullptr;
		if (specialWriteConcern)
		{
			write_concern = mongoc_write_concern_new();
			if (!write_concern)
			{
				nlogerr("mongoc_write_concern_new err");
				return error;
			}
			mongoc_write_concern_set_w(write_concern, writeConcern);
		}
		if (!mongoc_collection_update(collection_,flags,query.m_bson,obj.m_bson, write_concern, &error))
		{
			if (write_concern)
			{
				mongoc_write_concern_destroy(write_concern);
			}
			mongoc_client_pool_push(m_poolImpl, c);
			return error;
		}
		if (write_concern)
		{
			mongoc_write_concern_destroy(write_concern);
		}
		mongoc_client_pool_push(m_poolImpl, c);
		return error;
	}

	inline MongoCursorPtr MongoConnectionPool::find(const std::string& collection, const Bson& query, const Bson& opt, mongoc_read_mode_t readMode /*= MONGOC_READ_PRIMARY */)
	{
		auto c = mongoc_client_pool_pop(m_poolImpl);
		if (!c)
		{
			return MongoCursorPtr(new MongoCursor(2));
		}
		auto collection_ = mongoc_client_get_collection(c, m_dbname.c_str(), collection.c_str());
		if (!collection_)
		{
			mongoc_client_pool_push(m_poolImpl, c);
			return MongoCursorPtr(new MongoCursor(2));
		}
		auto read_prefs = mongoc_read_prefs_new(readMode);
		auto cursor = mongoc_collection_find_with_opts(collection_, query.m_bson, opt.m_bson, read_prefs);
		mongoc_read_prefs_destroy(read_prefs);
		if (!cursor)
		{
			mongoc_client_pool_push(m_poolImpl, c);
			return MongoCursorPtr(new MongoCursor(2));
		}
		return MongoCursorPtr(new MongoCursor(0, cursor));
	}

}

#endif