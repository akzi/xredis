#include "../include/hash.hpp"


XTEST_SUITE(hash)
{
	xnet::proactor pro;
	xredis::redis redis(pro);


	void hset_test()
	{
		xredis::hash hash(redis);
		hash.hset("hash", "hello", "world", [](std::string && status, uint32_t && result) {
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
			std::cout << result << std::endl;
		});
	}

	void hget_test()
	{
		xredis::hash hash(redis);
		hash.hget("hash", "hello", [](std::string && status, std::string && result) {
			if (status.size())
			{
				std::cout << status << std::endl;
				return;
			}
			std::cout << result << std::endl;
		});
	}

	XUNIT_TEST(set_addr)
	{
		redis.set_addr("192.168.3.224",7000);
		redis.regist_cluster_init_callback([](std::string &&status, bool result) {
			if (!result)
				xassert(false);
			hset_test();
			hget_test();
		});
	}
	
	
	XUNIT_TEST(run)
	{
		pro.run();
	}
}