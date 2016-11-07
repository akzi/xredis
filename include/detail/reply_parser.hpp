#pragma once
namespace xredis
{
	namespace detail
	{
		class reply_parser
		{
		private:
			struct callback
			{
				enum type_t
				{
					e_map_cb,
					e_integral_cb,
					e_string_cb,
				};
				union
				{
					string_map_callback *map_cb_;
					string_callback *str_cb_;
					integral_callback *int_cb_;
				};
			};
			struct obj
			{
				obj() {}
				~obj() {}
				enum type_t
				{
					e_str,
					e_integal,
					e_map,
				} type_;
				union 
				{
					std::string *str_;
					std::map<std::string, obj*> map_;
					uint32_t integral_;
				};
				int elements_;
			};
			struct task
			{
				task() {}
				~task() {}
				typedef std::function<bool()> parse_func;
				std::vector<parse_func> parser_funcs_;
				obj obj_;
			};
		public:
			reply_parser()
			{

			}
			void do_parse(std::string data)
			{
				data_.append(data);
				if (task_.parser_funcs_.empty()) 
				{
					prepare();
				}
			}
		private:
			bool prepare()
			{

				char ch = *(data_.data() + pos_);
				switch (ch)
				{
				case ':':
					task_.parser_funcs_.emplace_back(std::bind(&reply_parser::get_integal,this));
					break;
				case '$':
					task_.parser_funcs_.emplace_back(std::bind(&reply_parser::get_string, this));
					break;
				case '*':
					task_.parser_funcs_.emplace_back(std::bind(&reply_parser::get_integal,this));
				}
			}
			bool get_string()
			{

			}
			bool get_integal()
			{

			}
			bool get_map()
			{

			}
			uint32_t pos_ = 0;
			std::string data_;
			task task_;
			std::list<callback> callbacks_;
		};

	}
}