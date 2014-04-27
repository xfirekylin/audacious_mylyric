#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <unistd.h>
#include   <wchar.h>  
#include   <iconv.h> 
#include <string.h>
#include <stdlib.h>

//#define __DEBUG__
#ifdef __DEBUG__  
    #define DEBUG_TRACE printf  
#else  
    #define DEBUG_TRACE(...)  
#endif 

#define MAX_PATH_LENGTH 255

extern long int download_by_http_return_buffer(char *url, char *msgbody, char **buffer);
extern int download_by_http(char *url, char *msgbody, char *filename);
int download_lrc(int argc, char **argv);
bool find_lrc(const char *path, const char *lrc_name, char * match_name);
void remove_str_blank(char *buf);
void savefile(char *file, int length, char *filename);
void unicodetostr(wchar_t *from, char *to, char hightolow);
void urlencode(const char *from, char *to);
size_t convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen);
