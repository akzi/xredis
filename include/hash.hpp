#pragma once
#include "xredis.hpp"
namespace xredis
{
	class hash
	{
	public:
		hash(redis &_redis)
			:redis_(_redis)
		{

		}
		void hset(const std::string &name, std::string &key, std::string &&value, integral_callback && callback)
		{
			std::string data = cmd_builder()({"HSET", name, std::move(key),std::move(value)});
			redis_.req(name, std::move(data), std::move(callback));
		}
		void hget(const std::string &name, std::string &&key, bulk_callback&& callback)
		{
			std::string data = cmd_builder()({"HGET", name, std::move(key)});
			redis_.req(key, std::move(data), callback);
		}
	private:
		redis &redis_;
	};
}