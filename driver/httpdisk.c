#include "socket.h"
#include "httpdisk.h"
#include <linux/kernel.h>
#include <linux/string.h>
#define REQUEST_SIZE (4096)


int http_head_info(int ip,short port,char * filename,char * hostname)
{
    struct socket *kSocket;
    struct sockaddr_in addr;
    char *request,*buffer;
    char *pstr;
    int status,size;

    request = kmalloc(REQUEST_SIZE,GFP_KERNEL);
    buffer  = kmalloc(REQUEST_SIZE,GFP_KERNEL);
    kSocket = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = ip;
    
    status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
    if(status<0){

#ifdef _SBD_DEBUG	
	printk("cannot connect to host\n");
#endif

	goto err_loc;
    }
      
    snprintf(
	request,
	REQUEST_SIZE,
	"HEAD /%s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nUser-Agent: HD\r\nConnection: close\r\n\r\n",
	filename,
	hostname);  

#ifdef _SBD_DEBUG	
    printk("\nhttp_head_info send :\n%s\n",request);
#endif

    size = send(kSocket,request,strlen(request),0);
    if(size<0){

#ifdef _SBD_DEBUG	
        printk("http_head_info send error");
#endif

	goto err_loc;
    }
    size = recv(kSocket,buffer,REQUEST_SIZE,0);
    if(size<0){

#ifdef _SBD_DEBUG	
    	printk("http_head_info recv err"); 
#endif

	goto err_loc;
    }
    buffer[size-1] = '\0';
    if(strnicmp(buffer,"HTTP/1.1 200 OK",15)){

#ifdef _SBD_DEBUG	
        printk("http_head_info not 200");
#endif

	status = -1;
	goto err_loc;
    }

#ifdef _SBD_DEBUG
    printk("http_head_info recv:\n%s\n",buffer); 
#endif

    pstr = strstr(buffer,"Content-Length:");
    if(pstr == NULL|| pstr+16>= buffer+REQUEST_SIZE){

#ifdef _SBD_DEBUG
        printk("no valid Content-Length\n");
#endif

        status =-1;
	goto err_loc;
    }
    status = simple_strtoul(pstr+16,NULL,0);
err_loc:
    close(kSocket); 
    kfree(buffer);
    kfree(request);
    return status;
}
int http_get_block(int ip,short port,char * filename,char *hostname,int offset,int len, void *buf)
{
    struct socket *kSocket=NULL;
    struct sockaddr_in addr;
    char *request,*buffer;
    char *pstr;
    int status,size,datalen;
    request = kmalloc(REQUEST_SIZE,GFP_KERNEL);
    buffer  = kmalloc(REQUEST_SIZE,GFP_KERNEL);

    datalen = 0;   
    snprintf(
	request,
	REQUEST_SIZE,
	"GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%u-%u\r\nAccept: */*\r\nUser-Agent: HD\r\n\r\n",
	filename,
	hostname,
        (unsigned int) offset,
        (unsigned int) (offset+len-1)
        );  

#ifdef _SBD_DEBUG
    printk("\nhttp_get_block send :\n%s\n",request);
#endif
   
    kSocket = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = ip;
    status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
    size = send(kSocket,request,strlen(request),0);
    if(status<0||size<0){
        close(kSocket);
        kSocket = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = port;
        addr.sin_addr.s_addr = ip;
        status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
        size = send(kSocket,request,strlen(request),0);
        if(status<0||size<0){

#ifdef _SBD_DEBUG
	    printk("http_get_block send to host\n");	
#endif

	    status = -1;
	    goto err_loc;
        }
    } 
    size = recv(kSocket,buffer,REQUEST_SIZE-1,0);
    if(size<0){
        close(kSocket);
        kSocket = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = port;
        addr.sin_addr.s_addr = ip;
        status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
        size = send(kSocket,request,strlen(request),0);
    	size = recv(kSocket,buffer,REQUEST_SIZE,0);
    	if(size<0){

#ifdef _SBD_DEBUG
            printk("http_get_block recv error\n");
#endif

	    status = -1;
	    goto err_loc;
        }
    }
    if(strnicmp(buffer,"HTTP/1.1 206 Partial Content",28)){

#ifdef _SBD_DEBUG
        printk("http_get_block not 206\n");
#endif

	status = -1;
	goto err_loc;
    }
    pstr = strstr(buffer,"\r\n\r\n")+4;
    if(pstr == NULL||pstr<buffer|| pstr>= buffer+REQUEST_SIZE){

#ifdef _SBD_DEBUG
        printk("http_get_block not valid ended\n");
#endif

	status = -1;
	goto err_loc;
    }
    *(pstr-2) = 0;

#ifdef _SBD_DEBUG
    printk("http_get_block recv:\n%s\n",buffer);
#endif

    datalen = size - (int)(pstr - buffer);
    if(datalen>len || pstr+datalen > buffer+REQUEST_SIZE){
        printk("http_get_block too small buffer size to hold content\n");
	status = -1;
	goto err_loc;
    }
    if(datalen>0){
      memcpy(buf,pstr,datalen);
    }
    while(datalen<len){
        size = recv(kSocket,buffer,REQUEST_SIZE,0);
        if(size<0){

#ifdef _SBD_DEBUG
       	    printk("http_get_block abort transporting \n");
#endif

	    status = -1;
	    goto err_loc;
        }
        if(size==0)
	    break;    
        if(datalen+size>len||size>REQUEST_SIZE){

#ifdef _SBD_DEBUG
       	    printk("http_get_block stop transporting \n");
#endif

	    status = -1;
	    break;
        }
        memcpy(((char *)buf + datalen),buffer,size);
	datalen += size;
    }  
    
    if(datalen!=len){

#ifdef _SBD_DEBUG
        printk("HttpDisk: received data :%d,expected data: %d\n",datalen,len);
#endif

	status = -1;
    }  
    status =1 ;
     
err_loc:
    kfree(buffer); 
    kfree(request); 
    close(kSocket); 
    return status;
}
char * boundary="---------------------------7dd2033a30146";
char * prefix ="-----------------------------7dd2033a30146\r\nContent-Disposition: form-data; name=\"file\"; filename=\"C:\\config.ini\"\r\nContent-Type: application/octet-stream\r\n\r\n";
char * postfix ="\r\n-----------------------------7dd2033a30146\r\nContent-Disposition: form-data; name=\"submit\"\r\n\r\nSubmit\r\n-----------------------------7dd2033a30146--\r\n";

int http_set_block(int ip,short port,char *filename,char *hostname,int offset,int len,void *buf)
{
    struct socket *kSocket=NULL;
    struct sockaddr_in addr;
    char *request,*buffer;
    int status,size,datalen;
    request = kmalloc(REQUEST_SIZE,GFP_KERNEL);
    buffer  = kmalloc(REQUEST_SIZE,GFP_KERNEL);

    datalen = 0;   
    snprintf(
	request,
	REQUEST_SIZE,
	"POST /%s?of=%d&len=%d HTTP/1.1\r\nHost: %s\r\nContent-Type: multipart/form-data; boundary=%s\r\nContent-Length: %d\r\nConnection: Close\r\n\r\n",
	filename,
        (unsigned int) offset,
        (unsigned int) len,
	hostname,
	boundary,
	strlen(prefix)+len+strlen(postfix)
        );  
    kSocket = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = ip;
    status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
    size = send(kSocket,request,strlen(request),0);
    size = send(kSocket,prefix,strlen(prefix),0);
    size = send(kSocket,buf,len,0);
    size = send(kSocket,postfix,strlen(postfix),0);
	
    if(size<0){
        close(kSocket);
        kSocket = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = port;
        addr.sin_addr.s_addr = ip;
        status = connect(kSocket,(struct sockaddr *)&addr,sizeof(addr));     
        size = send(kSocket,request,strlen(request),0);
    	size = send(kSocket,prefix,strlen(prefix),0);
    	size = send(kSocket,buf,len,0);
    	size = send(kSocket,postfix,strlen(postfix),0);
        if(size<0){

#ifdef _SBD_DEBUG
            printk("http_set_block send error\n");
#endif

	    status = -1;
	    goto err_loc;
        }
    } 

#ifdef _SBD_DEBUG
    printk("\nhttp_set_block send :\n%s\n",request);
#endif
    
    size = recv(kSocket,buffer,REQUEST_SIZE-1,0);
    if(size<=0){

#ifdef _SBD_DEBUG
        printk("http_set_block recv error\n");
#endif

	status = -1;
	goto err_loc;
    }
    buffer[size-1]='\0';

#ifdef _SBD_DEBUG
    printk("http_set_block recv:\n%s\n",buffer);
#endif

    if(strnicmp(buffer,"HTTP/1.1 200 OK",15)){

#ifdef _SBD_DEBUG
        printk("http_set_block not 200 OK\n");
#endif
	status = -1;
	goto err_loc;
    }
    status = 1; 

err_loc:
    kfree(buffer); 
    kfree(request); 
    close(kSocket); 
    return status;
}
