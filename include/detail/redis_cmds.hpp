namespace xredis
{
	namespace detail
	{
		struct lists
		{
			template<typename CB>
			void LPOP(std::string &&key, CB cb)
			{
				std::string cmd = redis_cmd_formarter()({ "LPOP", key});

			}
		};
	}
}
