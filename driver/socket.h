#ifndef _MY_SOCKET_H
#define _MY_SOCKET_H

#include <linux/socket.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/net.h>
#include <linux/aio.h>
#include <net/sock.h>
#include <linux/in.h>
#include <stdarg.h>


extern int shutdown(struct socket *sock,int flag);
extern int socket_init(void);
extern struct socket * socket(int family, int type, int protocol );
extern int connect(struct socket *sock,struct sockaddr *addr,int addr_len);
extern int send(struct socket *sock, void *buf, int size, int flag);
extern int recv(struct socket *sock, void *buf, int size, int flag);
extern void close(struct socket *sock);
#endif
