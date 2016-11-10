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
			connector()
			{

			}
			connector(xnet::connector &&connector)
				:xconnector_(std::move(connector))
			{

			}
			connector(connector &&info)
				:xconnector_(std::move(info.xconnector_))
			{
				ip_ = std::move(info.ip_);
				port_ = info.port_;
			}
			~connector()
			{
				xconnector_.close();
			}

			std::string ip_;
			int port_;
			int max_slots_;
			xnet::connector xconnector_;
		};
		
	public:
		xredis(xnet::proactor &_proactor)
			:proactor_(_proactor)
		{

		}

		template<typename CB>
		void req(const std::string &key, std::string &&buf, CB &&cb)
		{
			xredis_client* client = get_client(key);
		}

		void set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = cluster;
			add_connector(ip, port, 0);
			if(is_cluster_)
				req_master_info();
		}

	private:
		void req_master_info()
		{
			//get default connector
			connector *_connector = get_connector("");
			xassert(_connector);
			xredis_client_ = new xredis_client;
			xredis_client_->regist_connector(_connector->xconnector_);
			xredis_client_->regist_close_callback([](xredis_client &_xredis_client) {

			});
			xredis_client_->req(
				redis_cmd_formarter()({ "cluster","slots" }), 
				[this](std::string &&error_code, cluster_slots &&slots){
				if(error_code.size())
				{
					XLOG_CRIT << "req cluster slots failed, "<<error_code.c_str();
					return;
				}
				cluster_slots_ = std::move(slots);
			});
			xredis_client_->req(
				redis_cmd_formarter()({ "cluster","nodes" }), 
				[this](std::string &&error_code, std::string && result) {
				if(error_code.size())
				{
					XLOG_CRIT << "req cluster nodes failed, " << error_code.c_str();
					return;
				}
				auto master_infos = std::move(master_info_parser()(result));
				if(master_infos != master_infos_)
					reset_connector(std::move(master_infos_));
			});
		}
		
		xredis_client *get_client(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if(clients_.size())
					return clients_.begin()->second.get();
			}
			int slot = get_slot(key);
			auto itr = clients_.lower_bound(slot);
			if (itr == clients_.end())
				return add_client(key);
			else
				return itr->second.get();
		}
		xredis_client *add_client(const std::string &key)
		{
			auto _connector = get_connector(key);
			if (!_connector)
				return nullptr;
			std::unique_ptr<xredis_client> client(new xredis_client);
			xredis_client *ptr = client.get();
			client->regist_connector(_connector->xconnector_);
			clients_.emplace(_connector->max_slots_, std::move(client));
			_connector->xconnector_.sync_connect(_connector->ip_, _connector->port_);
			return ptr;
		}
		void add_connector(const std::string ip,int port,int max_slot)
		{
			connector _connector(proactor_.get_connector());
			_connector.ip_ = ip;
			_connector.port_ = port;
			_connector.max_slots_ = max_slots_;
			std::string key = ip + ":" + std::to_string(port);
			if (connectors_.emplace(key, std::move(_connector)).second == false)
			{
				connectors_.erase(connectors_.find(key));
				auto ret = connectors_.emplace(key, std::move(_connector)).second;
				xassert(ret);
			}
		}
		connector *get_connector(const std::string &key)
		{
			if (!is_cluster_ || key.empty())
			{
				if (connectors_.size())
					return &connectors_.begin()->second;
				XLOG_CRIT << "not connector,set_addr first";
				return nullptr;
			}
			master_info info = get_master_info(get_slot(key));
			if (info.id_.empty())
			{
				XLOG_CRIT << "not find master info slot:" << get_slot(key);
				return nullptr;
			}
			auto first = info.ip_ + ":" + std::to_string(info.port_);
			if (connectors_.find(first) == connectors_.end())
			{
				add_connector(info.ip_, info.port_,info.max_slot_);
			}
			return &connectors_[first];
		}
		void reset_connector(std::vector<master_info> && info)
		{
			master_infos_ = std::move(info);
			for (auto &itr: master_infos_)
			{
				add_connector(itr.ip_, itr.port_, itr.max_slot_);
			}
		}
		master_info get_master_info(uint16_t slot)
		{
			for (auto &itr : master_infos_)
			{
				if (itr.min_slot_ <= slot && slot < itr.max_slot_)
				{
					return itr;
				}
			}
			return master_info();
		}
		uint16_t get_slot(const std::string &key)
		{
			return crc16er()(key.c_str(), key.size()) % max_slots_;
		}
		xredis_client *xredis_client_;
		bool is_cluster_ = false;
		std::map<std::string, connector> connectors_;
		//first is max slots.
		const int max_slots_ = 16383;
		std::map<int, std::unique_ptr<xredis_client>> clients_;
		xnet::proactor &proactor_;
		cluster_slots cluster_slots_;
		std::vector<master_info> master_infos_;
	};
}