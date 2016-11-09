#include "detail/detail.hpp"
namespace xredis
{
	class xredis_client
	{
	public:
		xredis_client()
		{

		}

		void init()
		{
			conn_.regist_recv_callback([this](void *data, int len){
				if (len <= 0)
				{
					close_callback();
					return;
				}
				conn_.async_recv_some();
				reply_parser_.parse((char*)data, len);
			});
			conn_.regist_send_callback([this](int len){
				if (len < len)
				{
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
		template<typename CB>
		void req(std::string &&req, CB cb)
		{
			reply_parser_.regist_callback(cb);
		}
	private:
		void close_callback()
		{
			conn_.close();
			reply_parser_.close();
			close_callback_();
		}
		bool is_send_ = false;
		bool is_recv_ = false;
		std::list<std::string> send_buffer_queue_;
		reply_parser reply_parser_;
		xnet::connection conn_;
		std::function<void()> close_callback_;
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
			bool operator <(const connector &self) const
			{
				if (ip_ < self.ip_)
					return true;
				else if (port_ < self.port_)
					return true;
				return false;
			}
			std::string ip_;
			int port_;
			xnet::connector connector_;
		};
	public:
		xredis(xnet::proactor &_proactor)
			:proactor_(_proactor)
		{

		}
		void set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = true;
			ip_ = ip;
			port_ = port;

			connector _connector(proactor_.get_connector());
			_connector.ip_ = ip;
			_connector.port_ = port;
			std::string key = ip + ":" + std::to_string(port_);
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
			xredis_client* client = get_client(key);
			if (client == nullptr)
			{
				auto &connector_ = get_connector(key);
				connector_.connector_.bind_fail_callback([&](std::string error_code) {
					cb("connect redis server failed", {});
				});
				connector_.connector_.bind_success_callback([](xnet::connection &&conn) {

				});
			}
		}
	private:
		void get_cluster_slots()
		{

			connector &_connector = get_connector("");

		}
		xredis_client *get_client(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if(clients_.size())
					return &clients_.begin()->second;
				return nullptr;
			}
			//todo cluster
		}
		connector &get_connector(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if (connectors_.size())
					return connectors_.begin()->second;
				assert(false);//
			}
			//todo cluster
		}
		std::string ip_;
		int port_;
		bool is_cluster_ = false;
		std::map<std::string, connector> connectors_;
		//first is max slots.
		std::map<int, xredis_client> clients_;
		xnet::proactor &proactor_;
	};
}