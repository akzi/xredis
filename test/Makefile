CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++11 \
-I ./../../include

server:
	g++ hash_test.cpp xredis_test.cpp  $(CFLAGS) -o unit_test
rm_server:
	$(shell if [ -f server ]; then rm server; fi;)

