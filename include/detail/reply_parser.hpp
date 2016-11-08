#pragma once
/*
*2
	*5
		:4000
		:16383
		*3
			$13
			192.168.3.224
			:7000
			$40
			b9ab1cef8c926f9771fd4efa16d047391a179e4d
		*3
			$13
			192.168.3.224
			:7002
			$40
			232eb1f44c2860a2b17b9d1cc2d3cd5854a728aa
		*3
			$13
			192.168.3.224
			:7001
			$40
			c6d8894dc6b281488185a4734578eda2cf80442b
	*5
		:0
		:3999
		*3
			$13
			192.168.3.224
			:7003
			$40
			138eb855c041bbe2b8bb72e6f38c3a2d17697884
		*3
			$13
			192.168.3.224
			:7005
			$40
			308d41acf591ec905c577e997942fcded12882be
		*3
			$13
			192.168.3.224
			:7004
			$40
			0fcfcfdbd9228b3d90f3f2dc461447ae037091ba
*/
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
					e_str_map_cb,
					e_integral_cb,
					e_bulk_cb,
				} type_;
				union
				{
					string_map_callback *bulk_map_cb_;
					string_callback *bulk_cb_;
					integral_callback *int_cb_;
				};
				~callback()
				{
					if (type_ == e_str_map_cb)
						delete bulk_map_cb_;
					if (type_ == e_bulk_cb)
						delete bulk_cb_;
					if (type_ == e_integral_cb)
						delete int_cb_;
				}
			};
			struct obj
			{
				obj() {}
				~obj() {}
				enum type_t
				{
					e_new,
					e_bulk,//
					e_null,
					e_integal,
					e_array,
					e_error,//status error
					e_ok,//status ok
				} type_ = e_new;
				union 
				{
					std::string *str_;
					std::vector<obj*> *array_;
					int32_t integral_;
				};
				/*
				* for Bulk Strings length
				*     Arrays elements 
				*/
				int len_ = 0;
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
					if (obj_.type_ == obj::e_array)
					{
						for (auto &itr : *obj_.array_)
							delete itr;
						delete obj_.array_;
					}
					else if (obj_.type_ == obj::e_bulk)
						delete obj_.str_;
					memset(&obj_, 0, sizeof obj_);
				}
			};
			void regist_callback(string_map_callback cb)
			{
				callback tmp;
				tmp.bulk_map_cb_ = new string_map_callback(cb);
				tmp.type_ = callback::e_str_map_cb;
				callbacks_.emplace_back(tmp);
			}
		public:
			reply_parser()
			{

			}
			void parse(const char *data, int len)
			{
				data_.append(data, len);
				run();
			}
			void close()
			{

			}
		private:
			void run()
			{
				do 
				{
					if (task_.parser_funcs_.empty())
					{
						prepare();
					}
					while (task_.parser_funcs_.size())
					{
						auto func = task_.parser_funcs_.back();
						task_.parser_funcs_.pop_back();
						if (func() == false)
						{
							task_.parser_funcs_.emplace_back(func);
							return;
						}
					}

				} while (true);
				
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
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_len, this));
					break;
				case '*':
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_array, this));
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_len, this));
					break;
				case '-':
					task_.obj_.type_ = obj::e_error;
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_str, this));
					break;
				case '+':
					task_.obj_.type_ = obj::e_ok;
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::get_str, this));
					break;
				}
				++pos_;
				return true;
			}
			bool get_str()
			{
				obj &o = get_obj();
				if (!o.str_)
					o.str_ = new std::string;

				while (pos_ < data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						o.str_->push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						return true;
					}
					++pos_;
				}
				return false;
			}
			bool get_bulk()
			{
				obj &o = get_obj();
				if(pos_ + o.len_ > data_.size())
					return false;
				o.str_ = new std::string(data_.data() + pos_, o.len_);
				pos_ += o.len_;
				o.type_ = obj::e_bulk;
				return true;
			}
			bool get_len()
			{
				set_roll_back();
				std::string buf;
				while (pos_ < (int)data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						buf.push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						get_obj().len_ = std::strtol(buf.c_str(), 0, 10);
						if (errno == ERANGE)
							assert(false);
						return true;
					}
					++pos_;
				}
				roll_back();
				return false;
			}
			bool get_integal()
			{
				set_roll_back();
				std::string buf;
				while (pos_ < (int)data_.size())
				{
					char ch = data_[pos_];
					if (ch != '\r')
						buf.push_back(ch);
					else if (data_[pos_ + 1] == '\n')
					{
						pos_ += 2;
						obj &o = get_obj();
						o.integral_ = std::strtol(buf.c_str(), 0, 10);
						if (errno == ERANGE)
							assert(__FILE__ == 0);
						o.type_ = obj::e_integal;
						return true;
					}
					++pos_;
				}
				roll_back();
				return false;
			}
			bool get_array()
			{
				obj &o = get_obj();
				int elememts = o.len_;
				if (elememts == -1)
				{
					o.type_ = obj::e_null;
					return true;
				}
				for (int i = 0; i < elememts; ++i)
				{
					task_.parser_funcs_.emplace_back(
						std::bind(&reply_parser::prepare, this));
				}
				o.type_ = obj::e_array;
				return true;
			}
			bool reset_task()
			{
				task_.reset();
				return true;
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
				if(task_.obj_.type_ != obj::e_array)
					return task_.obj_;

				if (!task_.obj_.array_)
					task_.obj_.array_ = new std::vector<obj *>;

				if (task_.obj_.array_->empty())
					goto new_obj;

				if (task_.obj_.array_->back()->type_ != obj::e_new)
					goto new_obj;
				else
					return *task_.obj_.array_->back();

			new_obj:
				obj *o = new obj;
				task_.obj_.array_->push_back(o);
				return *o;
			}
			bool parser_done()
			{
				assert(callbacks_.size());
				callback cb = callbacks_.front();
				callbacks_.pop_front();
				switch (cb.type_)
				{
				case callback::e_bulk_cb:
					string_cb(cb);
					break;
				case callback::e_str_map_cb:
					str_map_cb(cb);
					break;
				default:
					break;
				}
				return true;
			}
			void str_map_cb(const callback &cb)
			{
				obj &o = task_.obj_;
				if(o.type_ == obj::e_null)
					(*cb.bulk_map_cb_)("null", {});
				else if(o.type_ == obj::e_error)
					(*cb.bulk_map_cb_)(std::move(*o.str_), {});
				else if (task_.obj_.type_ == obj::e_array)
				{
					std::map<std::string, std::string> result;
					assert(o.array_->size()%2 == 0);
					for (std::size_t i =0; i < o.array_->size();)
					{
						result.emplace(std::move(*(*o.array_)[i]->str_), std::move(*(*o.array_)[i+1]->str_));
						i += 2;
					}
				}
				
			}
			void string_cb(const callback &cb)
			{
				obj &o = get_obj();
				if(o.type_ == obj::e_null)
					(*cb.bulk_cb_)("null", "");
				(*cb.bulk_cb_)("",std::move(*o.str_));
				reset_task();
			}
			std::string::size_type last_pos_ = 0;
			std::string::size_type pos_ = 0;
			std::string data_;
			task task_;
			std::list<callback> callbacks_;
		};

	}
}