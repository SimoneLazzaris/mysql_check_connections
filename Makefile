MYSQL_FLAGS = $(shell mysql_config --cflags) $(shell mysql_config --libs)

mysql_check_connections: mysql_check_connections.c
	gcc -o $@ $^ $(MYSQL_FLAGS)
