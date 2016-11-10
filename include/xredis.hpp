#include "detail/detail.hpp"
namespace xredis
{
	class xredis_client
	{
	public:
		xredis_client()
		{

		}
		~xredis_client()
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
		void set_master_info(master_info info)
		{
			master_info_ = info;
		}
		master_info get_master_info()
		{
			return master_info_;
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
				send_req();
			});
			send_req();
			is_send_ = true;
		}
		void send_req()
		{
			if(send_buffer_queue_.size())
			{
				conn_.async_send(std::move(send_buffer_queue_.front()));
				send_buffer_queue_.pop_front();
				return;
			}
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
		master_info master_info_;
	};

	class xredis
	{
	private:
		struct connector
		{
			connector(xnet::connector &&connector)
				:xconnector_(std::move(connector))
			{

			}
			connector(connector &&self)
				:xconnector_(std::move(self.xconnector_))
			{
				info_ = std::move(self.info_);
			}
			~connector()
			{
				xconnector_.close();
			}
			xnet::connector xconnector_;
			master_info info_;
		};
		
	public:
		xredis(xnet::proactor &_proactor)
			:proactor_(_proactor),
			connector_(proactor_.get_connector())
		{

		}
		template<typename CB>
		bool req(const std::string &key, std::string &&buf, CB &&cb)
		{
			auto client = get_client(key);
			if(!client)
				return false;
			client.req(std::move(buf), std::move(cb));
			return true;
		}
		void set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = cluster;
			xclient_.regist_connector(connector_.xconnector_);
			xclient_.regist_close_callback(
				std::bind(&xredis::client_close_callback, 
				this, std::placeholders::_1));

			connector_.xconnector_.sync_connect(ip, port);
			if(is_cluster_)
				req_master_info();
		}
	private:
		void client_close_callback(xredis_client &client)
		{
			auto itr = clients_.find(client.get_master_info().max_slot_);
			if(itr != clients_.end())
				clients_.erase(itr);
			xassert(client.get_master_info() != xclient_.get_master_info())
		}
		void req_master_info()
		{
			xclient_.req(
				redis_cmd_formarter()({ "cluster","slots" }), 
				[this](std::string &&error_code, cluster_slots &&slots){
				if(error_code.size())
				{
					XLOG_CRIT << "req cluster slots failed, "
						<< error_code.c_str();
					return;
				}
				cluster_slots_ = std::move(slots);
			});
			xclient_.req(
				redis_cmd_formarter()({ "cluster","nodes" }), 
				[this](std::string &&error_code, std::string && result) {
				if(error_code.size())
				{
					XLOG_CRIT << "req cluster nodes failed, "
						<< error_code.c_str();
					return;
				}
				auto master_infos = std::move(master_info_parser()(result));
				if(master_infos != master_infos_)
					update_connector(std::move(master_infos_));
			});
		}
		
		xredis_client *get_client(const std::string &key)
		{
			if(!is_cluster_ || key.empty())
				&xclient_;
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
			return add_client(*_connector);
		}
		xredis_client * add_client(connector &_connector)
		{
			std::unique_ptr<xredis_client> client_ptr(new xredis_client);
			xredis_client *client = client_ptr.get();
			client_ptr->regist_connector(_connector.xconnector_);
			clients_.emplace(_connector.info_.max_slot_, std::move(client_ptr));
			_connector.xconnector_.
				sync_connect(_connector.info_.ip_, _connector.info_.port_);
			return client;
		}
		void add_connector(master_info info)
		{
			
			std::string key = info.ip_+":"+std::to_string(info.port_);
			auto itr = connectors_.find(key);
			if(itr == connectors_.end())
			{
				connector _connector(proactor_.get_connector());
				_connector.info_ = info;
				add_client(_connector);
				connectors_.emplace(key, std::move(_connector));
				return;
			}
			if (itr->second.info_ != info)
			{
				connectors_.erase(itr);
				connector _connector(proactor_.get_connector());
				_connector.info_ = info;
				add_client(_connector);
				connectors_.emplace(key, std::move(_connector));
			}
		}
		connector *get_connector(const std::string &key)
		{
			if(!is_cluster_)
				return &connector_;

			master_info info = get_master_info(get_slot(key));
			if (info.id_.empty())
			{
				XLOG_CRIT << "not find master info slot:" << get_slot(key);
				return nullptr;
			}
			auto first = info.ip_ + ":" + std::to_string(info.port_);
			if (connectors_.find(first) == connectors_.end())
			{
				add_connector(info);
			}
			return &connectors_.find(first)->second;
		}
		void update_connector(std::vector<master_info> && info)
		{
			master_infos_ = std::move(info);
			for (auto &itr: master_infos_)
			{
				add_connector(itr);
			}
		}
		master_info get_master_info(uint16_t slot)
		{
			for (auto &itr : master_infos_)
				if(itr.min_slot_ <= slot && slot < itr.max_slot_)
					return itr;
		}
		uint16_t get_slot(const std::string &key)
		{
			return crc16er()(key.c_str(), key.size()) % max_slots_;
		}

		bool is_cluster_ = false;
		xnet::proactor &proactor_;
		
		//alone
		connector connector_;
		xredis_client xclient_;

		//cluster
		const int max_slots_ = 16383;
		std::map<std::string, connector> connectors_;
		std::map<int, std::unique_ptr<xredis_client>> clients_;
		cluster_slots cluster_slots_;
		std::vector<master_info> master_infos_;
	};
}