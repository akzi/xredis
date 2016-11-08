#include "detail/detail.hpp"
namespace xredis
{
	class xredis_client :
		public std::enable_shared_from_this<xredis_client>
	{
	public:
		typedef std::shared_ptr<xredis_client> ptr;
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
	class xredis_cluster
	{
	private:
		struct connector_info
		{
			connector_info(xnet::connector &&connector)
				:connector_(std::move(connector))
			{

			}
			connector_info(connector_info &&info)
				:connector_(std::move(info.connector_))
			{
				ip_ = std::move(info.ip_);
				port_ = info.port_;
			}
			~connector_info()
			{
				connector_.close();
			}
			bool operator <(const connector_info &self) const
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
		xredis_cluster(xnet::proactor &_proactor)
			:proactor_(_proactor)
		{
		}
		void set_addr(std::string ip, int port)
		{
			connector_info connector(proactor_.get_connector());
			connector.ip_ = ip;
			connector.port_ = port;
			if (connectors_.emplace(std::move(connector)).second == false)
			{
				connectors_.erase(connectors_.find(connector));
				auto ret = connectors_.emplace(std::move(connector)).second;
				assert(ret);
			}
		}
		
		template<typename CB>
		void req(const std::string &key, std::string &&buf, CB &&cb)
		{
			xredis_client::ptr client = get_client(key);
			if(client == nullptr)

		}
	private:
		xredis_client::ptr get_client(const std::string &key)
		{
			// todo 
			return clients_.begin()->second;
		}
		std::set<connector_info> connectors_;
		std::map<int, xredis_client::ptr> clients_;
		xnet::proactor &proactor_;
	};
	class xredis 
	{
	public:
		xredis()
		{

		}
	};
}