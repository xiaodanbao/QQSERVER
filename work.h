#ifndef WORK_H_
#define WORK_H_


#define CLIENTCOUNT  256
#define BODYBUF     1024

struct msg_t 
{
	unsigned char head[4];
	char body[BODYBUF];
};


class work
{
public:
	work(int port);
	~work();
	void run();
private:
	int socket_accept();
	void user_logout(int st);
	void sendmsg(const struct msg_t *msg,ssize_t msglen);
	void loginmsg(int st,int userid,const char* passwd);
	int socket_recv(int st);
	int setnonblocking(int st);
	int auth_passwd(int userid,const char* passwd);	
	void fix_socket_client(int index,int st);
	void broadcast_user_status();
	
	int listen_st;
	int socket_client[CLIENTCOUNT];
};


#endif
