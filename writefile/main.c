#include <stdio.h>
#include <Winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 4096

int set_file_content(char *infile, char *outfile,int offset,int len)
{
	OVERLAPPED inol,outol;
	BOOL ok ;
	int size = 0,i,j;
	char *buf;
	HANDLE hOut = CreateFile(outfile,GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	HANDLE hIn = CreateFile(infile,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	printf("\n");
	printf("offset=%d,len=%d\n",offset,len);
	if ( hIn == INVALID_HANDLE_VALUE){
		printf("cannot open file %s\n",infile);
		return GetLastError();	
	}
	if(hOut == INVALID_HANDLE_VALUE ){
		printf("cannot open file %s\n",outfile);
		return GetLastError();	
	}
	buf = malloc(BUF_SIZE+1);
	printf("write %s to %s ,offset: %d<br>\n",infile,outfile,offset);
	memset(&inol,0, sizeof(inol));
	memset(&outol,0, sizeof(outol));

	outol.Offset = offset;
	i = 0 ;
	ok = FALSE;
	while(ReadFile(hIn,buf,BUF_SIZE,&size,&inol))
	{
		if(size<0){
			printf("set_file_content error\n");
			break;
		}
		if(i+size>=len){
			size =  len - i;
			ok = TRUE;
		}
		i+=size;
		WriteFile(hOut,buf,size,&j,&outol);
		outol.Offset = outol.Offset +size;
		inol.Offset = inol.Offset +size;
		if(ok)
			break;
	}
	printf("now len=%d\n",i);
	free(buf);
	CloseHandle(hIn);
	CloseHandle(hOut);
	return 0;
}

int main(int argc,char * argv[])
{
	char *infile, *outfile;
	int offset,len;
	infile = argv[1];
	outfile = argv[2];
	offset = atoi(argv[3]);
	len = atoi(argv[4]);
	set_file_content(infile, outfile,offset,len);
}