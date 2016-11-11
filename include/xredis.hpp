#pragma once
#include "detail/detail.hpp"
namespace xredis
{
	class redis_client
	{
	public:
		redis_client(xnet::connector && _connector)
			:connector_(std::move(_connector))
		{
			init();
		}
		~redis_client()
		{

		}
		template<typename CB>
		void req(std::string &&req, CB &&cb)
		{
			reply_parser_.regist_callback(std::move(cb));
			if (is_connected_ == false)
			{
				send_buffer_queue_.emplace_back(std::move(req));
				return;
			}
			if (send_buffer_queue_.size())
			{
				send_buffer_queue_.emplace_back(std::move(req));
				if (!is_send_)
				{
					conn_.async_send(std::move(send_buffer_queue_.front()));
					send_buffer_queue_.pop_front();
				}
			}
			else if (!is_send_)
			{
				conn_.async_send(std::move(req));
			}
			else
			{
				send_buffer_queue_.emplace_back(std::move(req));
			}
		}
		void regist_close_callback(std::function<void(redis_client&)> &&func)
		{
			close_callback_ = std::move(func);
		}
		template<typename CB>
		void regist_slots_moved_callback(CB cb)
		{
			slots_moved_callback_ = cb;
		}
		void set_master_info(const master_info &info)
		{
			master_info_ = info;
		}
		master_info &get_master_info()
		{
			return master_info_;
		}
		void connect(const std::string &ip, int port)
		{
			ip_ = ip;
			port_ = port;
			connector_.async_connect(ip, port);
		}
	private:
		void init()
		{
			reply_parser_.regist_moved_error_callback(
				[this](const std::string &error) {

			});
			connector_.bind_success_callback(
				std::bind(&redis_client::connect_success_callback,
					this,
					std::placeholders::_1));
			connector_.bind_fail_callback(
				std::bind(&redis_client::connect_failed_callback,
					this,
					std::placeholders::_1));
		}
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
			is_connected_ = true;
			conn_ = std::move(conn);
			conn_.regist_recv_callback([this](void *data, int len) {
				if (len <= 0)
				{
					close_callback();
					conn_.close();
					return;
				}
				reply_parser_.parse((char*)data, len);
				conn_.async_recv_some();
			});
			conn_.regist_send_callback([this](int len) {
				if (len < len)
				{
					conn_.close();
					is_send_ = false;
					close_callback();
					return;
				}
				send_req();
			});
			send_req();
			conn_.async_recv_some();
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
			reply_parser_.close("lost connection error");
			if(close_callback_)
				close_callback_(*this);
		}
		std::string ip_;
		int port_;
		std::function<void(std::string)> connect_failed_cb_;
		std::function<void(xnet::connection &&)> connect_success_cb_;
		std::function<void(const std::string &, redis_client &)> slots_moved_callback_;
		std::function<void(redis_client &)> close_callback_;

		std::list<std::string> send_buffer_queue_;
		bool is_send_ = false;
		xnet::connection conn_;
		xnet::connector connector_;
		master_info master_info_;
		reply_parser reply_parser_;
		bool is_connected_ = false;
	};

	class redis
	{
	public:
		redis(xnet::proactor &_proactor, int max_slots = 16383)
			:proactor_(_proactor),
			max_slots_(max_slots_)
		{

		}
		template<typename CB>
		bool req(const std::string &key, std::string &&buf, CB &&cb)
		{
			auto client = get_client(key);
			if (!client)
				return false;
			client->req(std::move(buf), std::move(cb));
			return true;
		}
		void set_addr(std::string ip, int port, bool cluster = true)
		{
			is_cluster_ = cluster;
			ip_ = ip;
			port_ = port;
			assert(!client_);
			client_ = std::make_unique<redis_client>(proactor_.get_connector());
			client_->regist_close_callback(
				std::bind(&redis::client_close_callback, 
					this, std::placeholders::_1));
			client_->regist_slots_moved_callback(
				std::bind(&redis::slots_moved_callback,
					this, std::placeholders::_1, std::placeholders::_2));
			client_->connect(ip_, port);
			if(is_cluster_)
				req_master_info();
		}
	private:
		void slots_moved_callback(const std::string & error, redis_client &client)
		{
			XLOG_INFO << error.c_str();
		}
		void client_close_callback(redis_client &client)
		{
			auto itr = clients_.find(client.get_master_info().max_slot_);
			if(itr != clients_.end())
				clients_.erase(itr);
			if (client.get_master_info() == client_->get_master_info())
			{
				client_.release();
			}
		}
		void req_master_info()
		{
			client_->req( redis_cmd_formarter()({ "cluster","slots"}), 
				[this](std::string &&error_code, cluster_slots &&slots){
				if(error_code.size())
				{
					XLOG_CRIT << "req cluster slots failed, "
						<< error_code.c_str();
					return;
				}
				cluster_slots_ = std::move(slots);
			});
			client_->req( redis_cmd_formarter()({ "cluster","nodes" }),
				[this](std::string &&error_code, std::string && result) {
				if (error_code.size())
				{
					XLOG_CRIT << "req cluster nodes failed, "
						<< error_code.c_str();
					return;
				}
				cluster_nodes_callback(result);
			});
		}
		void cluster_nodes_callback(const std::string &result)
		{
			auto infos = std::move(master_info_parser()(result));
			if (infos != master_infos_)
			{
				update_client(infos);
				master_infos_ = std::move(infos);
			}
		}
		redis_client *get_client(const std::string &key)
		{
			if(!is_cluster_ || key.empty())
				&client_;
			int slot = get_slot(key);
			auto itr = clients_.lower_bound(slot);
			if (itr == clients_.end() || 
				itr->second->get_master_info().min_slot_ > slot)
				return nullptr;
			return itr->second.get();
		}

		void update_client(const std::vector<master_info> & info)
		{
			for (auto &itr: info)
			{
				bool found = false;
				for(auto &ctr: clients_)
				{
					if (ctr.second->get_master_info().id_ == itr.id_)
					{
						found = true;
						if (ctr.second->get_master_info().min_slot_ != itr.min_slot_ )
						{
							ctr.second->get_master_info().min_slot_ = itr.min_slot_;
						}
						if (ctr.second->get_master_info().max_slot_ != itr.max_slot_ )
						{
							ctr.second->get_master_info().max_slot_ = itr.max_slot_;
							//fix clients_ key. 
							auto client = std::move(ctr.second);
							clients_.erase(clients_.find(ctr.first));
							clients_.emplace(client->get_master_info().max_slot_, std::move(client));
							break;
						}
					}
				}
				if (!found)
				{
					std::unique_ptr<redis_client> client(
						new redis_client(proactor_.get_connector()));
					client->get_master_info() = itr;
					clients_.emplace(itr.max_slot_, std::move(client));
				}
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
		std::string ip_;
		int port_;
		xnet::proactor &proactor_;
		std::unique_ptr<redis_client> client_;
		//cluster
		bool is_cluster_ = false;
		int max_slots_;
		std::map<int, std::unique_ptr<redis_client>> clients_;
		cluster_slots cluster_slots_;
		std::vector<master_info> master_infos_;
	};
}