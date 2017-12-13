#ifndef _MY_HTTP_DISK_H
#define _MY_HTTP_DISK_H

#include <stdarg.h>
#include "socket.h"

//#define _SBD_DEBUG

extern int http_set_block(int ip,short port,char *filename,char *hostname,int offset,int len,void *buf);
extern int http_get_block(int ip,short port,char * filename,char *hostname,int offset,int len, void *buf);
extern int http_head_info(int ip,short port,char * filename,char *hostname);
#endif
