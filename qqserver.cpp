#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "pub.h"
#include "work.h"

int main(int arg,char* args[])
{
	if(arg < 2)
	{
	    printf("usage:qqserverd port\n");
	    return -1;
	}	
	int iport = atoi(args[1]);
	if(iport == 0)
	{
	     printf("port %d is invaild\n",iport);
	     return -1;
	}
	setdeamon();
	work w(iport);
	
	printf("qqserver is begin\n");
	signal1(SIGINT,catch_Signal);
	signal1(SIGPIPE,catch_Signal);
	
	w.run();
	printf("qqserver is end\n");
	return 0;
	
}
