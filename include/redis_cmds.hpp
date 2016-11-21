#pragma once
#include "../include/xredis.hpp"
namespace xredis
{
	class lists
	{
	public:
		lists(redis &_redis)
			:redis_(_redis)
		{

		}
		template<typename CB>
		bool LPOP(std::string &&key, CB &&cb)
		{
			std::string tmp = key;
			std::string buf = cmd_builder()({ "LPOP", std::move(key) });
			return redis_.req(key, buf, std::move(cb));
		}
	private:
		redis &redis_;
	};
}
