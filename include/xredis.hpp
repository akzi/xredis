#include "detail/detail.hpp"
namespace xredis
{
	class xredis_client
	{
	public:
		xredis_client()
		{

		}
		void regist_close_callback(std::function<void(xredis_client&)> &&func)
		{
			close_callback_ = std::move(func);
		}
		template<typename CB>
		void req(std::string &&req, CB &&cb)
		{
			reply_parser_.regist_callback(std::move(cb));
			if (send_buffer_queue_.size())
			{
				send_buffer_queue_.emplace_back(std::move(req));
				if (!is_send_)
				{
					conn_.async_send(std::move(send_buffer_queue_.front()));
					send_buffer_queue_.pop_front();
				}
			}
			else
			{
				if (!is_send_)
					conn_.async_send(std::move(req));
				else
					send_buffer_queue_.emplace_back(std::move(req));
			}
		}

		void regist_connector(xnet::connector &_connector)
		{
			_connector.bind_success_callback(
				std::bind(&xredis_client::connect_success_callback,
					this,
					std::placeholders::_1));
			_connector.bind_fail_callback(
				std::bind(&xredis_client::connect_failed_callback, 
					this, 
					std::placeholders::_1));
		}
	private:
		void connect_success_callback(xnet::connection &&conn)
		{
			regist_connection(std::move(conn));
		}
		void connect_failed_callback(std::string error_code)
		{
			reply_parser_.close(std::move(error_code));
		}
		void regist_connection(xnet::connection &&conn)
		{
			conn_ = std::move(conn);
			conn_.regist_recv_callback([this](void *data, int len) {
				if (len <= 0)
				{
					close_callback();
					return;
				}
				conn_.async_recv_some();
				reply_parser_.parse((char*)data, len);
			});
			conn_.regist_send_callback([this](int len) {
				if (len < len)
				{
					is_send_ = false;
					close_callback();
					return;
				}
				if (send_buffer_queue_.size())
				{
					conn_.async_send(std::move(send_buffer_queue_.front()));
					send_buffer_queue_.pop_front();
					return;
				}
				is_send_ = false;
			});
		}
		void close_callback()
		{
			conn_.close();
			reply_parser_.close("lost connection error");
			close_callback_(*this);
		}
		std::function<void(std::string)> connect_failed_cb_;
		std::function<void(xnet::connection &&)> connect_success_cb_;
		bool is_send_ = false;
		std::list<std::string> send_buffer_queue_;
		reply_parser reply_parser_;
		xnet::connection conn_;
		std::function<void(xredis_client &)> close_callback_;
	};

	class xredis
	{
	private:
		struct connector
		{
			connector(xnet::connector &&connector)
				:connector_(std::move(connector))
			{

			}
			connector(connector &&info)
				:connector_(std::move(info.connector_))
			{
				ip_ = std::move(info.ip_);
				port_ = info.port_;
			}
			~connector()
			{
				connector_.close();
			}

			std::string ip_;
			int port_;
			xnet::connector connector_;
		};
		struct master_info
		{
			int min_slot_;
			int max_slot_;
			std::string ip_;
			std::string port_;
		};
	public:
		xredis(xnet::proactor &_proactor)
			:proactor_(_proactor)
		{

		}
		void set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = true;
			connector _connector(proactor_.get_connector());
			_connector.ip_ = ip;
			_connector.port_ = port;
			std::string key = ip + ":" + std::to_string(port);
			if (connectors_.emplace(key,std::move(_connector)).second == false)
			{
				connectors_.erase(connectors_.find(key));
				auto ret = connectors_.emplace(key,std::move(_connector)).second;
				assert(ret);
			}
		}
		
		template<typename CB>
		void req(const std::string &key, std::string &&buf, CB &&cb)
		{
			xredis_client& client = get_client(key);
		}
	private:
		void get_master_info()
		{
			//get default connector
			connector &_connector = get_connector("");
			xredis_client_ = new xredis_client;
			xredis_client_->regist_connector(_connector.connector_);
			xredis_client_->regist_close_callback([](xredis_client &_xredis_client) {
			});
			xredis_client_->req(
				redis_cmd_formarter()({ "cluster","slots" }), 
				[this](std::string &&error_code, cluster_slots &&slots){
				if(error_code.size())
				{
					//todo log
					return;
				}
				cluster_slots_ = std::move(slots);
			});
			xredis_client_->req(
				redis_cmd_formarter()({ "cluster","slots" }), 
				[this](std::string &&error_code, std::string && result) {
				if(error_code.size())
				{
					//todo log
					return;
				}
			});
		}
		xredis_client & add_client(const std::string &key)
		{
			auto &_connector = get_connector(key);

			_connector.connector_.sync_connect(_connector.ip_, _connector.port_);

		}
		xredis_client &get_client(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if(clients_.size())
					return clients_.begin()->second;
			}
			//todo cluster
		}
		connector &get_connector(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if (connectors_.size())
					return connectors_.begin()->second;
				assert(false);//nerver here
			}
			int slot = crc16(key.c_str(), key.size()) % max_slots_;
			std::string ip;
			int port;
			for (auto &itr: cluster_slots_.cluster_)
			{
				if (itr.min_slots_ <= slot && slot < itr.max_slots_)
				{

				}
			}
		}

		xredis_client *xredis_client_;
		bool is_cluster_ = false;
		std::map<std::string, connector> connectors_;
		//first is max slots.
		const int max_slots_ = 16383;
		std::map<int, xredis_client> clients_;
		xnet::proactor &proactor_;
		cluster_slots cluster_slots_;
		std::vector<master_info> master_infos_;
	};
}