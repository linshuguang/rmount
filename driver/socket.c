#include "socket.h"

int socket_init(void)
{
    return 0;
}

struct socket * socket(int family, int type, int protocol )
{
    struct socket *sock;
    sock_create(family, type, protocol, &sock);
    return sock;
}
int shutdown(struct socket *sock,int flag)
{
    return sock->ops->shutdown(sock,flag); 
}
int connect(struct socket *sock,struct sockaddr *addr,int addr_len)
{
    return sock->ops->connect(sock,addr,addr_len,0); 
}
int send(struct socket *sock, void *buf, int size, int flag)
{
    mm_segment_t oldfs;
    int ssize;
    struct msghdr msg = {0};
    struct iovec iov;

    iov.iov_base = buf;
    iov.iov_len = size;
    
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    ssize = sock_sendmsg(sock,&msg,size);
    set_fs(oldfs);
    return ssize;
}

int recv(struct socket *sock, void *buf, int size, int flag)
{
    mm_segment_t oldfs;
    int rsize;
    struct msghdr msg = {0};
    struct iovec iov;
  
    iov.iov_base = buf;
    iov.iov_len = size;
    
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    rsize = sock_recvmsg(sock,&msg,size,flag);
    set_fs(oldfs);
    return rsize;
}

void close(struct socket *sock)
{
    sock_release(sock);
}

