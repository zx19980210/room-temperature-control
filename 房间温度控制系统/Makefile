TITLE_COLOR = \033[33m
NO_COLOR = \033[0m


# When trying to compile one of the executables, first look for its .c files
# Then check if the libraries are in the lib folder
sensor_gateway : main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.so lib/libtcp.so
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING sensor_gateway *****$(NO_COLOR)"
	gcc -g -Wall -Werror -lm -L./lib -o sensor_gateway main.c connmgr.c datamgr.c sbuffer.c sensor_db.c -lpthread -lsqlite3 lib/libtcp.so lib/libdplist.so -DTIMEOUT=5 -DSET_MAX_TEMP=20 -DSET_MIN_TEMP=10


sensor_node_limited_loops: sensor_node.c lib/libtcp.so
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING sensor_node with limited loops *****$(NO_COLOR)"
	gcc -Wall -Werror -o sensor_node sensor_node.c lib/libtcp.so -DLOOPS=20

sensor_node_inifinite_loops: sensor_node.c lib/libtcp.so
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING sensor_node with inifinite loops *****$(NO_COLOR)"
	gcc -Wall -Werror -o sensor_node sensor_node.c lib/libtcp.so

open_gateway: sensor_gateway
	@echo "$(TITLE_COLOR)\n***** OPEN GATEWAY *****$(NO_COLOR)"
	./sensor_gateway 1234

open_gateway_with_valgrind: sensor_gateway
	@echo "$(TITLE_COLOR)\n***** OPEN GATEWAY *****$(NO_COLOR)"
	valgrind -v --tool=memcheck --leak-check=full ./sensor_gateway 1234

test_single_node:sensor_node
	./sensor_node 15 1 0.0.0.0 1234

test: sensor_node
	@echo "$(TITLE_COLOR)\n***** SEND DATA INFINITELY *****$(NO_COLOR)"
	./sensor_node 15 1 0.0.0.0 1234 &
	./sensor_node 21 2 0.0.0.0 1234 &
	./sensor_node 37 1 0.0.0.0 1234 &
	./sensor_node 49 3 0.0.0.0 1234 &
	./sensor_node 112 2 0.0.0.0 1234

test_terminate: sensor_node
	@echo "$(TITLE_COLOR)\n***** DATA ARE SEND WITH DIFFERENT FREQUENCY WITH TIMEOUT *****$(NO_COLOR)"
	./sensor_node 15 1 0.0.0.0 1234 &
	./sensor_node 21 2 0.0.0.0 1234 &
	./sensor_node 37 1 0.0.0.0 1234 &
	./sensor_node 49 3 0.0.0.0 1234 &
	./sensor_node 112 2 0.0.0.0 1234

test_fast: sensor_node
	./sensor_node 15 0 0.0.0.0 1234 &
	./sensor_node 21 0 0.0.0.0 1234 &
	./sensor_node 37 0 0.0.0.0 1234 &
	./sensor_node 49 0 0.0.0.0 1234 &
	./sensor_node 112 0 0.0.0.0 1234

test_durability: sensor_node
	timeout 1h ./sensor_node 15 1 0.0.0.0 1234 &
	timeout 1h ./sensor_node 21 2 0.0.0.0 1234 &
	timeout 1h ./sensor_node 37 1 0.0.0.0 1234 &
	timeout 1h ./sensor_node 49 3 0.0.0.0 1234 &
	timeout 1h ./sensor_node 112 2 0.0.0.0 1234

stop: sensor_node
	killall sensor_node
