#include "../include/xredis.hpp"

XTEST_SUITE(reply_parser)
{
	XUNIT_TEST(parse)
	{
		const std::string buffer =
			"*2\r\n"
			"*5\r\n:4000\r\n:16383\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7000\r\n$40\r\nb9ab1cef8c926f9771fd4efa16d047391a179e4d\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7002\r\n$40\r\n232eb1f44c2860a2b17b9d1cc2d3cd5854a728aa\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:7001\r\n$40\r\nc6d8894dc6b281488185a4734578eda2cf80442b\r\n"
			"*5\r\n:0\r\n:3999\r\n"
			"*3\r\n$13\r\n192.168.3.224\r\n:700";
		const std::string buffer2 = "3\r\n$40\r\n138eb855c041bbe2b8bb72e6f38c3a2d17697884\r\n"
					"*3\r\n$13\r\n192.168.3.224\r\n:7005\r\n$40\r\n308d41acf591ec905c577e997942fcded12882be\r\n"
					"*3\r\n$13\r\n192.168.3.224\r\n:7004\r\n$40\r\n0fcfcfdbd9228b3d90f3f2dc461447ae037091ba\r\n";

		xredis::detail::reply_parser parser;

		xredis::detail::cluster_slots_callback cb = 
			[](std::string && status, xredis::cluster_slots &&cluster_slots) 
		{ 

		};

		parser.regist_callback(cb);

		parser.parse(buffer.data(), buffer.size());
		parser.parse(buffer2.data(), buffer2.size());
	}
}