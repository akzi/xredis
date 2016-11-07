#pragma once
namespace xredis
{
	namespace detail
	{
		struct redis_cmd_formarter
		{
			std::string operator()(std::vector<std::string> &&chunks)
			{
				std::string buf;
				buf.push_back('*');
				buf.append(std::to_string(chunks.size()));
				buf.append("\r\n");
				for (auto &itr: chunks)
				{
					buf.push_back('$');
					buf.append(std::to_string(itr.size()));
					buf.append(itr);
					buf.append("\r\n");
				}
				return buf;
			}
		};
	}
}