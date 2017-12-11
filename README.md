# mysql_check_connections
Nagios pluging to check if mysql connections limits are triggering, dropping connections

Only three parameters:
 - hostname (ip address)
 - username (mysql user)
 - password (mysql password)
 
 Check the number of current threads (connections) versus the max configured.
 Status is *critical* if the number of the current connections is over 90% of the max capability; status is *warning* if is over 70%.
 
The number of failed connection is also tested: status is reported *critical* if there are more that 5 connection error since last check; *warning* if more than 1 (but less than 5).
