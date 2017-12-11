#include <stdio.h>
#include <unistd.h>
#include <mysql.h>

#define NAGIOS_OK	0
#define NAGIOS_WARNING	1
#define NAGIOS_CRITICAL	2
#define NAGIOS_UNKNOWN	3

const char*nagios_status[]={ "OK", "WARNING", "CRITICAL", "UNKNOWN" };

struct arguments {
	char *username;
	char *password;
	char *hostname;
};

void help_and_die(void) {
	printf("USAGE: mysql_check_connections -H hostname -u username -p password\n");
	_exit(NAGIOS_UNKNOWN);
}

void parse_opts(int argc, char ** argv,struct arguments * a) {
	int oc;             /* option character */
	while ((oc=getopt(argc,argv,"hu:p:H:"))!=-1) {
		switch (oc) {
		case 'h': help_and_die(); break;
		case 'u': a->username=optarg; break;
		case 'p': a->password=optarg; break;
		case 'H': a->hostname=optarg; break;
		}
	}
	if (a->username==NULL || a->password==NULL || a->hostname==NULL) 
		help_and_die();
}
void err(const char * str) {
	fprintf(stderr, "ERR: %s\n", str);
	_exit(NAGIOS_UNKNOWN);
}
int eq_store(MYSQL_ROW row, const char * str,int * val) {
	if (strcmp(row[0],str)==0) {
		*val=atoi(row[1]);
		return 1;
	}
	return 0;
}

int get_last_errmax(struct arguments *a) {
	char fnam[1024];
	char buf[1024];
	snprintf(fnam,1024,"/tmp/mysql_check_connections_%s",a->hostname);
	FILE *f=fopen(fnam,"rt");
	if (f==NULL)
		return 0;
	fgets(buf,1024,f);
	fclose(f);
	return atoi(buf);
}

int put_last_errmax(struct arguments *a,int errmax) {
	char fnam[1024];
	snprintf(fnam,1024,"/tmp/mysql_check_connections_%s",a->hostname);
	FILE *f=fopen(fnam,"wt");
	fprintf(f,"%d\n",errmax);
	fclose(f);
}

int main(int argc, char ** argv) {
	struct arguments a={0};
	MYSQL *con;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int status=NAGIOS_UNKNOWN;
	
	int max_connections=-1, current_connections=-1, err_max_connections=-1, last_errmax=-1, errconn=-1;
	
	parse_opts(argc,argv,&a);
	con = mysql_init(NULL);
	if (con == NULL) 
		err(mysql_error(con));
	if (mysql_real_connect(con, a.hostname, a.username, a.password,NULL,0,NULL,0)==NULL)
		err(mysql_error(con));
	if (mysql_query(con,"show status"))
		err(mysql_error(con));
	result = mysql_store_result(con);
	if (result==NULL)
		err("NO RESULTS on STATUS");
	while ((row=mysql_fetch_row(result))!=NULL) {
		if (eq_store(row,"Threads_connected",&current_connections)) continue;
		if (eq_store(row,"Connection_errors_max_connections",&err_max_connections)) continue;
	}
	mysql_free_result(result);
	if (mysql_query(con,"show global variables"))
		err(mysql_error(con));
	result = mysql_store_result(con);
	if (result==NULL)
		err("NO RESULTS on GLOBAL VARIABLES");
	while ((row=mysql_fetch_row(result))!=NULL) {
		if (eq_store(row,"max_connections",&max_connections)) continue;
	}
	mysql_free_result(result);
	mysql_close(con);
	last_errmax=get_last_errmax(&a);
	put_last_errmax(&a,err_max_connections);
	errconn=err_max_connections-last_errmax;
	
	if ((current_connections>(max_connections*9/10)) || (errconn>5))
		status=NAGIOS_CRITICAL;
	else if ( (current_connections>(max_connections*7/10)) || (errconn>0) )
		status=NAGIOS_WARNING;
	else
		status=NAGIOS_OK;
	printf("%s: Current connections: %d, max connections: %d, new connecction errors: %d [%d-%d]\n",nagios_status[status],current_connections, max_connections, errconn, err_max_connections,last_errmax);
	return status;
}
