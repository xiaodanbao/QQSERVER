#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "work.h"
#include "pub.h"

int work::setnonblocking(int st) 
{
	int opts = fcntl(st, F_GETFL);
	if (opts < 0)
	{
		printf("fcntl failed %s\n", strerror(errno));
		return 0;
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(st, F_SETFL, opts) < 0)
	{
		printf("fcntl failed %s\n", strerror(errno));
		return 0;
	}
	return 1;
}



int work::socket_accept()
{
	struct sockaddr_in client_addr;
	memset(&client_addr,0,sizeof(client_addr));
	socklen_t len = sizeof(client_addr);	
	int client_st = accept(listen_st,(struct sockaddr*)&client_addr,&len);
	if(client_st < 0)
	{
	    printf("accept failed %s\n",strerror(errno));
	}
	else 
	{
	    printf("accept by %s\n",inet_ntoa(client_addr.sin_addr));
	}
	return client_st;
}
void work::sendmsg(const struct msg_t *msg,ssize_t msglen)
{
	if(msg->head[2] < 0 || (msg->head[2]) >= CLIENTCOUNT)
	{
	   printf("%d: have not this userid\n",msg->head[2]);
	}
	else 
	{
	      if(socket_client[msg->head[2]] == 0)
	      printf("%d: have not this userid\n",msg->head[2]);
	      else 
		{
		     send(socket_client[msg->head[2]],(const char*)msg,msglen,0);
		     printf("send messgae : %d to %d -%s\n",msg->head[1],msg->head[2],msg->body);
		     
		}
	}
}
void work::fix_socket_client(int index,int st)
{
	if(socket_client[index] != 0)
	{
	  printf("%d:userid already login\n",index);
	  struct msg_t msg;
	  memset(&msg,0,sizeof(msg));
	  msg.head[0] = 2;
	  msg.head[1] = 3;
	  msg.head[2] = 0;
   	  msg.head[3] = 0;
	  send(st,(const char*)&msg,sizeof(msg.head),0);
	  close(st);
	}
	else 
	{
	   socket_client[index] = st;
	}
	
}

int work::auth_passwd(int userid,const char* passwd)
{
	if(strncmp(passwd,"123456",6) == 0)
	     return 1;
	else 
	     return 0;
}
void work::broadcast_user_status()
{
	struct msg_t msg;
	memset(&msg,0,sizeof(msg));
	msg.head[0] = 1;
	
	for(int i=0;i< CLIENTCOUNT;i++)
	{
	       if(socket_client[i] != 0)
		{
		   msg.body[i] = '1';
		}
		else 
			msg.body[i] = '0';
	}
	for(int i=0;i < CLIENTCOUNT;i++)
	{
	     if(socket_client[i] != 0)
	     {
		send(socket_client[i],(const char*)&msg,sizeof(msg.head)+strlen(msg.body),0);
	     printf("%d:boradcast,%s\n",i,msg.body);
	      }
	}	
}


void work::loginmsg(int st,int o_userid,const char* passwd)
{
	struct msg_t msg;
	memset(&msg,0,sizeof(msg));
	msg.head[0] = 2;
	msg.head[2] = 0;
	msg.head[3] = 0;
	
	if( (o_userid < 0) || (o_userid >= CLIENTCOUNT) )
	{
	     printf("login failed, %d:invaild userid\n",o_userid);
	     msg.head[1] = 1;
	     send(st,(const char*)&msg,sizeof(msg.head),0);
	     close(st);
	     return ;
	} 	
	if(!auth_passwd(o_userid,passwd))
	{
	   printf("login failed ,userid = %d ,passwd = %s :invailed password\n",o_userid,passwd);
	   msg.head[1] = 2;
	   send(st,(const char*)&msg,sizeof(msg.head),0);
	  close(st);
	 return ;
	   
	}
	printf("%d: login success\n",o_userid);
	fix_socket_client(o_userid,st);
	broadcast_user_status();	
}

int work::socket_recv(int st)
{
	struct msg_t msg;
	memset(&msg,0,sizeof(msg));
	ssize_t rc = recv(st,(char*)&msg,sizeof(msg),0);
	if(rc <= 0)
	{
	    if(rc < 0)
	    {
		 printf("recv failed %s\n",strerror(errno));  
	     }
	}
	else 
	{ 
	  switch(msg.head[0])
	 {
	    case 0: sendmsg(&msg,rc);
	    break;
	    case 1: loginmsg(st,msg.head[1],msg.body);break;
	    default :
	     printf("login fail,it's not login message,%s\n",(const char*)&msg);
	     msg.head[0] = 2;
	     msg.head[1] = 0;
	     msg.head[2] = 0;
	     msg.head[3] = 0;
	     send(st,(const char*)&msg,sizeof(msg.head),0);
	     return 0;
	  }
	}
	return rc;	
}




work::work(int port)
{
	memset(socket_client,0,sizeof(socket_client));
	listen_st = socket_create(port);
	if(listen_st == 0)
	{
	   exit(-1);
	}
	
}
work::~work()
{
	if(listen_st)
	{
	   close(listen_st);
	}
}

void work::user_logout(int st)
{
	int i=0;
	for(i=0;i<CLIENTCOUNT;i++)
	{
	   if(socket_client[i] == st)
	   {
		printf("%d user: socket disconnect\n",i);
		close(socket_client[i]);
		socket_client[i] = 0;
		broadcast_user_status();
		return ;
		
		} 
	}
}
void work::run()
{
	struct epoll_event ev,events[CLIENTCOUNT];
	setnonblocking(listen_st);
	int epfd = epoll_create(CLIENTCOUNT);
	ev.data.fd = listen_st;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	epoll_ctl(epfd,EPOLL_CTL_ADD,listen_st,&ev);
	
	int st = 0;
	while(1)
	{
	     int nfds = epoll_wait(epfd,events,CLIENTCOUNT,-1);
	     if(nfds == -1)
	     {
		 printf("epoll_wait failed %s\n",strerror(errno));
		  break;
	     }  
	     for(int i=0;i < nfds ;i++)
	     {
		   if(events[i].data.fd < 0)
		    continue;
		   if(events[i].data.fd == listen_st)
		  {
			st = socket_accept();
			if( st >= 0)
			{
			   setnonblocking(st);
			   ev.data.fd = st;
			   ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			   epoll_ctl(epfd,EPOLL_CTL_ADD,st,&ev);
			  continue;
		            
			}
		   
		   }
		    if(events[i].events & EPOLLIN)
		   {
			st = events[i].data.fd;
			if(socket_recv(st) <= 0)
			{
			     user_logout(st);
			     events[i].data.fd = -1;
			}
		    }
		     if(events[i].events & EPOLLERR)
		     {
		         st = events[i].data.fd;
			  user_logout(st);
			  events[i].data.fd = -1;
			}
		     if(events[i].events & EPOLLHUP)
		     {
			 st = events[i].data.fd;
			 user_logout(st);
			 events[i].data.fd = -1;
			}

		} 
	}

	
}
