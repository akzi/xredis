#pragma once
#ifdef _WIN32
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <cassert> 
#include <cstdio>

//deps
#include "../../deps/xnet/include/xnet.hpp"
#include "../../deps/xtest/include/xtest.hpp"
#include "../../deps/xlog/include/xlog.hpp"
//end of deps
#include "../redis_reply_entry.hpp"
#include "callback_func.hpp"
#include "functional.hpp"
#include "reply_parser.hpp"

namespace xredis
{
	using detail::string_map_callback;
	using detail::bulk_callback;
	using detail::integral_callback;

	using detail::reply_parser;
	using detail::redis_cmd_formarter;
	using detail::master_info_parser;
	using detail::crc16er;
}