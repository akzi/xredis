namespace xredis
{
	namespace detail
	{
		struct lists
		{
			template<typename CB>
			void LPOP(std::string &&key, CB cb)
			{
				std::string tmp = key;
				std::string cmd = redis_cmd_formarter()({ "LPOP", std::move(key)});

			}
		};
	}
}
