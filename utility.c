#include "utility.h"

bool match_lrc(const char *lrc_name, const char * match_name)
{
    int len1,len2;
    
    if (NULL == lrc_name || NULL == match_name)
    {
        return false;
    }
    
    len1 = strlen(lrc_name);
    len2 = strlen(match_name);
    
    
    while (--len1 >=0 && --len2 >=0)
    {
        if (('-' == lrc_name[len1])
            &&( '-' == match_name[len2]))
        {
            return true;
        }
        
        if (lrc_name[len1] != match_name[len2])
        {
            unsigned char c1,c2;
            
            if (lrc_name[len1] >= 'a' && lrc_name[len1] <= 'z')
            {
                c1 = lrc_name[len1] - 'a' + 'A';
            }
            
            if (match_name[len2] >= 'a' && match_name[len2] <= 'z')
            {
                c2 = match_name[len2] - 'a' + 'A';
            }
            
            if (c1 == c2)
                continue ;
                
            return false;
        }
    }
    
    return false;
}

bool find_lrc(const char *path, const char *lrc_name, char * match_name)
{
    DIR *pdir;
    struct dirent *pdirent;
    struct stat f_ftime;
    char full_path[256]={0};
    int path_len =0;
    
    if (NULL == path || NULL == lrc_name || NULL == match_name)
    {
        return false;
    }
    
    strcpy(full_path, path);
    path_len = strlen(full_path);
    
    if ('/' != full_path[path_len-1])
    {
        full_path[path_len++] = '/';
        full_path[path_len] = '\0';
    }
    
    if (NULL == (pdir = opendir(path)))
    {
        printf("\nopen dir %s error", path);
    }
    
    for(pdirent=readdir(pdir);pdirent!=NULL;pdirent=readdir(pdir))
    {
        if(strcmp(pdirent->d_name,".")==0||strcmp(pdirent->d_name,"..")==0)
            continue;

        strcpy(full_path+path_len, pdirent->d_name);
        if(-1 == stat(full_path,&f_ftime))
        {
             // printf("\nopen dir %s/%s stat file error", path,pdirent->d_name);
              continue;
            //return false;
        }
              
        if(S_ISDIR(f_ftime.st_mode))
            continue; /*子目录跳过*/

     // printf("文件:%s\n",pdirent->d_name);
      if (match_lrc(lrc_name,pdirent->d_name))
      {printf("文件:%s\n",pdirent->d_name);
        sprintf(match_name, "%s/%s", path, pdirent->d_name);
        return true;
      }
    }
    
  
    return false;
}

void remove_str_blank(char *buf)
{
    char *head=NULL, *tail=NULL;
    
    if (NULL == buf)
        return;
        
    head = buf;
    tail = buf + strlen(buf) - 1;
    
    while (' ' == *head)head++;
    
    while (' ' == *tail && tail > head)tail--;
    
    if (tail < head)
    {
        buf[0] = '\0';
    }
    
    *(tail+1) = '\0';
    
    memmove(buf,head,tail-head+2);
}


void savefile(char *file, int length, char *filename)
{
	FILE * fp=NULL;

	fp=fopen(filename,"w");
	if(fp == NULL)
	{
	 	printf("\nopen error:%s\n",filename);
	}
	else
	{
		fwrite(file,length,1,fp);
		fclose(fp);
		printf("\nsave fiel %s\n",filename);
	}
}

void urlencode(const char *from, char *to)
{
	int i=0;
	char hex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	
	printf("\n%s,len=%d\n",from,strlen(from));
	while(*from != '\0')
	{
		printf("%d:",(unsigned char)*from);
		if((unsigned char)*from < 128)
		{
		    if (' ' == *from)
		    {
		        to[i++] = '+';
		    }
		    else
		    {
			    to[i++] = *from;
			}
		}
		else
		{
			to[i++] = '%';
			to[i++] = hex[(unsigned char)(*from)/16];
			to[i++] = hex[(unsigned char)(*from)%16];
		}
		from++;
	}
	to[i] = '\0';
	//#ifdef test
	 printf("\n%s\n",to);
	//#endif
}

void unicodetostr(wchar_t *from, char *to, char hightolow)
{
	int i=0;
	char hex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	
	//printf("\n%s,len=%d\n",from,strlen(from));
	while(*from != 0x0000)
	{
		//printf("%d:",(unsigned char)*from);
		if(hightolow)
		{
			to[i++] = hex[(*from & 0xf000) >> 12];
			to[i++] = hex[(*from & 0x0f00) >> 8];
			to[i++] = hex[(*from & 0x00f0) >> 4];
			to[i++] = hex[*from & 0x000f];
		}
		else
		{
			to[i++] = hex[(*from & 0x00f0) >> 4];
			to[i++] = hex[*from & 0x000f];
			to[i++] = hex[(*from & 0xf000) >> 12];
			to[i++] = hex[(*from & 0x0f00) >> 8];
		}
		from++;
	}
	to[i] = '\0';
	//#ifdef test
	 printf("\n%s\n",to);
	//#endif
}

int convert(char *from)
{
    char *p=NULL,*frombuf=from;
	char tobuf[MAX_PATH_LENGTH],*to=NULL;
	iconv_t   h;
	int size_from=0,size_to=0;

    printf("\nconvert string=%s\n",from);
  	p=getenv("LC_CTYPE");

	if ( !p ) {
		p=getenv("LC_ALL");
		if ( !p)
			p=getenv("LANG");
	}

	if(p){
	 
	 if (memcmp(p,"zh_CN.UTF-8",11)==0)
	    { printf("\n%s\n",p);
	      h   =   iconv_open("GB2312", "UTF-8");  
	      if   ((iconv_t)-1   ==   h)  
		{  
		  perror("iconv_open");  
		  exit(1);  
		}  
   
	      size_from =   strlen(from);  
	      size_to   =   MAX_PATH_LENGTH;
	      to = tobuf;
	      memset(to,0,size_to);
	      if (-1 == iconv(h, &from, &size_from, &to, &size_to))
		printf("\nconver error"),exit(1);  
	      iconv_close(h);   
	    }
	    else
	    	{
	    		return 1;
	    	}
	} 

	printf("\nconvert:");
	int i= 0;
	while('\0' != tobuf[i])
	{
	    printf("%2X",(unsigned char)tobuf[i++]);
	    if (i%2 == 0)
	    printf(" ");
	}
	printf("sizeof(to)=%d\n",strlen(tobuf));
	
	memcpy(frombuf,tobuf,MAX_PATH_LENGTH);
	
}
