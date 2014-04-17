#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define LINUS_OS

#ifdef WIN_OS
#pragma   comment(lib, "Ws2_32.lib")
#include   <WINSOCK2.H>   
#include   <WINBASE.H> 
#endif

#ifdef LINUS_OS
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>
#endif

int download_by_http(char *url, char *msgbody, char *filename);
void save_chunked_file(char *file, int buflen, FILE *fp);
//#define test
long int download_by_http_return_buffer(char *url, char *msgbody, char **buffer)
{
	char filename[]="download-temp.txt";
	FILE *fp;
	long int len;

	download_by_http(url, msgbody, filename);
	
	fp=fopen(filename, "r+");
	if(NULL == fp)
	{
		printf("open file error\n");
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	if(-1 != len)
	{
		if(NULL == (*buffer = (char *)malloc(len+10)))
		{
			printf("malloc error!\n");
			return 0;
		}
		memset(*buffer, 0, len+10);
	}

	fseek(fp, 0, SEEK_SET);
	fread(*buffer, len, 1, fp);

	fclose(fp);
	(*buffer)[len] = '\0';

    printf("\nbuf len=%ld",len);
	return len;
}

int download_by_http(char *url, char *msgbody, char *filename)
{
	int sockfd;
	int len,i;
	struct sockaddr_in address;
	struct hostent *server_host_name;
	int result;
	char message_str[500]={0};
	char ch[4000] = {0};
	char *file = NULL;
	char chbuf[5000] = {0};
	int port=80;
	char defaulturi[2] = "";
	char *uri;	
	char *hostname=url;
	int headcnt=0;
	int byte=0,byte_read=0,length=0,flag=1,status =0,endflag=0,ipcnt=0,chunked=0;
	FILE *fp=NULL;	


	#ifdef WIN_OS
	{
		WSADATA	WsaData;
		WSAStartup (0x0101, &WsaData);
	}
	#endif

	
	if (NULL == url)
	{
		printf("params is null!");
		return 0;
	}
	//strcpy(hostname, url);
	i = 0;
	while('\0' != hostname[i])
	{
		if(hostname[i]<='Z' && hostname[i]>='A')
		{
			hostname[i]=hostname[i]+'a'-'A';
		}
		
		if('/' == hostname[i] && '/' == hostname[i+1])
		{
			if (0 == memcmp(hostname,"http://", 7))
			{
				hostname+=7;
				if ('\0' == *hostname)
				{
					return 0;
				}
				i = 0;
				break;
			}
			else
			{
				printf("it is not http!\n");
				return 0;
			}
		}
		i++;	
	}

	
	
	i=0;
	while(hostname[i]!='\0' && hostname[i]!='/')
	{
		i++;
	}
	
	if('/' == hostname[i] && '\0' != hostname[i+1])
	{
		uri = hostname+i+1;
	}
	else
	{
		uri = defaulturi;
	}
	hostname[i]='\0';
	
	
	
	if(0 == (server_host_name = gethostbyname(hostname)))
	{
		perror("can not resolving localhost\n");
		exit(1);
	}
	else 
	{
	#ifdef test
		char str[20];
		char ** pptr;
		pptr=server_host_name->h_addr_list;  //ip ��ַ
		for(; *pptr!=NULL; pptr++)
		{inet_ntop(server_host_name->h_addrtype,
                		*pptr, str, sizeof(str));
        		printf(" address:%s\n",str);
		}
	#endif
	}

	
	while (1)
	{
	 	if(1 == endflag)
		{
			if(-1 != sockfd)			
			close(sockfd);
			printf("\n");
		   	 break;
		}
		else
		{
			memset(ch, 0,sizeof(ch));
			memset(chbuf,0,sizeof(chbuf));
			byte = 0;
		}
				
		if(NULL == server_host_name->h_addr_list[ipcnt])
		{
			return 0;
		}
		else
		{
			if((sockfd = socket(AF_INET, SOCK_STREAM,0))==-1)
			{
				perror("error opening socket\n");
				return 0;
			}
			memset(&address,0,sizeof(address));
			address.sin_family = AF_INET;
			address.sin_addr.s_addr =
			(*(unsigned long int*)(server_host_name->h_addr_list[ipcnt]));
			address.sin_port=htons(port);
			len = sizeof(address);
		}
		
            #ifdef test		
		printf("\n\nbyte=%d\n\n",byte);
            #endif	
		
		if(NULL == msgbody)
		{	//	"Referer:http://lrcct2.ttplayer.com/\r\n"
		sprintf(message_str,"GET /%s HTTP/1.1\r\n"
			"HOST:%s\r\n"
			"Accept:image/gif,image/x-xbitmap,image/jpg,text/html,*/*\r\n"
			"User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n"
			"Connection:Keep-Alive\r\n"
			"Cache-Control:no-cache\r\n"
        	"\r\n",
        		uri, hostname);
		//"Range:bytes=0-0\r\n"strcat(message_str,"Referer: http://www.qianqian.com/lrcresult_frame.php?pe=1&qword=8&qfield=12&page=12\r\n\r\n");

		}
		else
		{
		sprintf(message_str,"GET /%s HTTP/1.1\r\n"
        		"HOST:%s\r\n%s",
        		uri, hostname,msgbody);
		}

		printf("\nmessage_str=\n%s\n",message_str);

		if(-1 == (result = connect(sockfd, (struct sockaddr*)&address, len)))
		{
			perror("can not connect to server");
			exit(1);
		}
		#ifdef test
		else
		{
			printf("\nconnet to host\n");
		}
		#endif
		
		if(send(sockfd, message_str, strlen(message_str), 0)==-1)
		{
			perror("can not send message");
			//close(sockfd);
			return 0;
		}
		
		while(1)
		{
			memset(ch, 0,sizeof(ch));
			if((byte_read=recv(sockfd, ch,sizeof(ch)-1, 0))==-1)
			{
				perror("can not receive data");
				printf("\nread data error");
				exit(1);
			}
			
							
			if(0!=byte_read)
			{
				if(NULL != fp)
				{
					if (!chunked)
					{
						fwrite(ch,byte_read,1,fp);
					}	
					else
					{
						save_chunked_file(ch, byte_read, fp);
					}
				}
				else
				{
					memcpy(chbuf+byte,ch,byte_read);
				}
				#ifdef test
				printf("%s",ch);
				#endif
				byte+=byte_read;
				
			#ifdef LINUX_OS
				printf("\33[?25l");
				if(length>0)
				printf("\x1b\x5bu\x1b\x5bshave downloaded %d%%,"
				      "length=%d",byte*100/length,length);
				else
				printf("\x1b\x5bs");
       				printf("\33[?25h");   
			#endif
			}
			else
			{
				#ifdef test
				printf("\nbyte_read=%d this need 0 byte\n",byte_read);
				#endif
				endflag=1;
				break;
			}
		
			if (0 == status)
			{
				int i=0;

				while(chbuf[i]!=' ' && '\0' != chbuf[i])i++;
				if ('\0' != chbuf[i] && '\0' != chbuf[i+1])//���ش���
				{
					if('2'!=chbuf[i+1])
					{
						ipcnt++;
						printf("\n%s\n",chbuf);
						break;
					}
					else
					{
						status = 1;
					}
				}
			}

			
				
			if(flag==1) //�ҵ� http responer content ���ݿ�ʼ
			{
				int j=byte-byte_read-30;
				if(j<0)j=0;
				while(j<=byte-4)
				{
					if((0 == length) && (0==memcmp(chbuf+j, "Content-Length",14)))
					{	
						int k=j+15,full=0;
						j = j + 14;
						while('\0' != chbuf[j])
						{
							if('\r' == chbuf[j])
							{
								full=1;
								break;
							}
							j++;
						}
						if(1 == full)
						{
							char buf[20] ={0};
							memcpy(buf,chbuf+k,j-k);
							length=atoi(buf);
						}
					}

					if ((0==chunked) && (!memcmp(chbuf+j, "Transfer-Encoding: chunked", 26)))
					{
						chunked = 1;
					}

					if (!memcmp(chbuf+j,"\r\n\r\n",4))
					{
						file=chbuf+j+4;
						headcnt=j+4;
						flag=0;
						if(NULL == (fp=fopen(filename,"wb")))
						{
						printf("can not open file %s",filename);
							return 0;
						}
						
						if(chunked == 1)
						{
							save_chunked_file(file, byte - headcnt, fp);
						}
						else
						{
							fwrite(file, byte-headcnt,1,fp);
						}						
						printf("\n%s\n",ch);
						break;
					}
					j++;
				}
			}
					
			#ifdef test
			if(0!=byte_read)
			printf("byte=%d,byte_read=%d,length=%d\n",byte,byte_read,length);
			#endif
					
			if(byte==length + headcnt)
			{	//
				endflag = 1;
				break;
			}
			
		}
	}//while(1) out
	
	#ifdef typebuf
	printf("\n%s\n",chbuf);
	#endif
		
	#ifdef test
	printf("\nhave read %d byte length=%d the end \n",byte,length);
	#endif
	
	if(NULL == fp)
	{
		if(NULL == (fp=fopen(filename,"wb")))
		{
			printf("can not open file %s",filename);
				return 0;
		}
		fwrite(chbuf, byte,1,fp);
	}
	
	if(NULL != fp)
	{
		fclose(fp);
	}

	 #ifdef WIN_OS
	   WSACleanup();
	 #endif

	return byte-headcnt;
} 


void save_chunked_file(char *file, int buflen, FILE *fp)
{
	static int chunked_len = 0;
	static int remain_chunked_len = 0;
	static int part_chunked_len = 1;
	static char lenbuf[10] ={0};
	int i=0;
	int writebytes = 0;
	
	if (part_chunked_len)
	{
		while ('\0' != *(file+i) && (' ' != *(file+i) && '\r' != *(file+i)) )
		{
			i++;
		}
		
		
		memcpy(lenbuf+strlen(lenbuf), file, i);
		

		if (' ' == *(file+i) || '\r' == *(file+i) )
		{
			chunked_len = strtol(lenbuf, NULL, 16);
			remain_chunked_len = chunked_len;
			if (0 == chunked_len)
			{	
				return;
			}
			printf("\nchunkedlen=%d",chunked_len);
			part_chunked_len = 0;
		}
		else
		{
			part_chunked_len = 1;
			return ;
		}

		
	}
	

	if (remain_chunked_len == chunked_len)
	{
		while ('\0' != *(file+i) && '\n' != *(file+i) )
		{
			i++;
		}
	}

	if ('\n' == *(file+i))
	{
		i++;
	}

	if ('\0' == *(file+i))
	{
		return;
	}


	if (buflen - i >= remain_chunked_len)
	{
		//fseek(fp, 0, SEEK_END);
		fwrite(file+i, remain_chunked_len, 1, fp);
		part_chunked_len = 1;
		memset(lenbuf, 0, sizeof(lenbuf));
		chunked_len = -1;
		save_chunked_file(file+i+remain_chunked_len, buflen-i-remain_chunked_len, fp);
	}
	else
	{
		//fseek(fp, 0, SEEK_END);
		fwrite(file+i, buflen-i, 1, fp);
		remain_chunked_len -= buflen - i;
	}
	
	return;
}







































