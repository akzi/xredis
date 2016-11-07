#pragma once
namespace xredis
{
	namespace detail
	{
		static inline int intlen(int i) 
		{
			int len = 0;
			if (i < 0)
			{
				len++;
				i = -i;
			}
			do 
			{
				len++;
				i /= 10;
			} while (i);
			return len;
		}
		static inline size_t bulklen(size_t len)
		{
			// $ + {len}\r\n + {data}\r\n
			return size_t(1 + intlen((int)len) + 2 + (int)len + 2);
		}


		struct redis_cmd_formarter
		{
			std::string operator()(int argc, const char **argv,const size_t *argvlen)
			{
				std::string data;
				char *cmd = NULL; 
				int pos; 
				size_t len;
				int totlen, j;

				totlen = 1 + intlen(argc) + 2;
				for (j = 0; j < argc; j++) 
				{
					len = argvlen ? argvlen[j] : strlen(argv[j]);
					totlen += (int)bulklen(len);
				}

				data.resize(totlen + 1);
				cmd = (char*)data.data();

				pos = sprintf(cmd, "*%d\r\n", argc);
				for (j = 0; j < argc; j++) 
				{
					len = argvlen ? argvlen[j] : strlen(argv[j]);
					pos += sprintf(cmd + pos, "$%Iu\r\n", len);  
					memcpy(cmd + pos, argv[j], len);
					pos += (int)len;
					cmd[pos++] = '\r';
					cmd[pos++] = '\n';
				}
				assert(pos == totlen);
				cmd[pos] = '\0';
				return data;
			}
		};
	}
}