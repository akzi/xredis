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
					e_new,
					e_data,//
					e_null,
					e_integal,
					e_array,
					e_error,//status error
					e_ok,//status ok
				} type_;
				union 
				{
					std::string *str_;
					std::list<obj*> *array_;
					uint32_t integral_;
				};
				int num_ = 0;
			};
			struct task
			{
				task() {}
				~task() {}
				typedef std::function<bool()> parse_func;
				std::vector<parse_func> parser_funcs_;
				obj obj_;
				void reset()
				{
					memset(&obj_, 0, sizeof obj_);
				}
			};
		public:
			reply_parser()
			{

			}
			void parse(std::string data)
			{
				data_.append(data);
				run();
			}
		private:
			void run()
			{
				if(task_.parser_funcs_.empty())
				{
					prepare();
				}
				while(task_.parser_funcs_.size())
				{
					if(task_.parser_funcs_.back()())
					{
						task_.parser_funcs_.pop_back();
					}
					else
						break;
				}
			}
			bool prepare()
			{
				if(task_.parser_funcs_.empty())
					task_.parser_funcs_.emplace_back(
					std::bind(&reply_parser::parser_done, this));

				if(pos_ >= data_.size())
					return false;// no data

				char ch = data_[pos_];
				switch (ch)
				{
				case ':':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_integal,this));
					break;
				case '$':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_bulk, this));
					break;
				case '*':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_array, this));
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_integal, this));
					task_.obj_.type_ = obj::e_array;
					task_.obj_.array_ = new std::list<obj*>;
					break;
				case '+':
					task_.obj_.type_ = obj::e_ok;
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_str, this));
				}
				++pos_;
				return true;
			}
			void get_str()
			{

			}
			bool get_bulk()
			{
				task_.parser_funcs_.emplace_back(
					std::bind(&reply_parser::get_data, this));
				task_.parser_funcs_.emplace_back(
					std::bind(&reply_parser::get_integal, this));
			}
			bool get_data()
			{
				obj &o = get_obj();
				if(pos_ + o.num_ < data_.size())
					return false;
				o.str_ = new std::string(data_.data() + pos_, o.num_);
			}
			bool get_integal()
			{
				set_roll_back();
				std::string buf;
				while(pos_ < (int)data_.size())
				{
					char ch = data_[pos_];
					if(ch != '\r')
						buf.push_back(ch);
					else if(data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						get_obj().num_ = std::strtol(buf.c_str(), 0, 10);
						if(errno == ERANGE)
							assert(false);
						return true;
					}
					++pos_;
				}
				roll_back();
				return false;
			}
			bool get_array()
			{
				int arrays_ = task_.obj_.array_->front()->integral_;
				for (int i = 0; i < arrays_; ++i)
				{
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::prepare, this));
				}
				return true;
			}
			bool reset_task()
			{
				task_.reset();
				return true;
			}
			bool parser_done()
			{

			}
			void set_roll_back()
			{
				last_pos_ = pos_;
			}
			void roll_back()
			{
				pos_ = last_pos_;
			}
			obj &get_obj()
			{
				if(task_.obj_.type_ == obj::e_data ||
				   task_.obj_.type_ == obj::e_integal)
					return task_.obj_;
			}
			std::string::size_type last_pos_ = 0;
			std::string::size_type pos_ = 0;
			std::string data_;
			task task_;
			std::list<callback> callbacks_;
		};

	}
}