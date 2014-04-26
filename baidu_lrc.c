#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libaudcore/vfs_async.h>
#include "utility.h"

#define LINUX_OS

#ifdef WIN_OS
 #include <winsock.h>
typedef __int64 INT64;
#endif

#ifdef LINUX_OS
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/un.h>
	#include <unistd.h>
   #include <glib.h>
   #include <gtk/gtk.h>

    typedef long long  INT64;
#endif
//#define test
//#define html
//#define typebuf
#define FILELENGTH 400000


//char chbuf[FILELENGTH]={0};
char *file=NULL;

bool parsehtml(const char *buf, long int buf_len, const char *artist,const char *title, char *presult)
{
    char song[333]={0};
    char arti[333]= {0};
    char href[333]={0};
    char gb2312_str2[100]={0};
    int  len_song,len_arti,len_href;
    char *cmp_str[3] = {song,arti,href};
    int  len[3];
    int  i=0;
    char gb2312_str[111]={0};
  long  int index=0;// *buf_head = buf;
    
    printf("\nstart parse html");
    

    strcpy(gb2312_str2, "\xB8\xE8\xC7\xFA\xA3\xBA\0");//歌曲：
    //convert(gb2312_str2);
    sprintf(song,"%s%s%s%s%s",
            "<div class=\"BlueBG\"><strong>",
             gb2312_str2,
            "</strong><B><font color=\"#c60a00\">",
            title,
            "</font></B></div>");
            

    strcpy(gb2312_str2, "\xB8\xE8\xCA\xD6\xA3\xBA\0");//歌手：\xB8\xE8\xCA\xD6\xA3\xBA
    //convert(gb2312_str2);
    urlencode(artist, gb2312_str);
   sprintf(arti,"%s%s%s%s",         
        "<strong>",
        gb2312_str2,        
        "</strong><A href=\"http://mp3.baidu.com/m?tn=baidump3&ct=134217728&lm=-1&word=",
        gb2312_str);
        
    sprintf(href,"<div style=\"word-break:break-all\"><a href=");
    
    len[0] = strlen(song);
    len[1] = strlen(arti);
    len[2] = strlen(href);
    
    printf("\n index = %ld,buf_len=%ld,\n song=%s,\narti=%s,\n href=%s \n",index, buf_len,song,arti,href);
    while (index < buf_len - 30)
    {
        if (0 == memcmp(buf+index, 0==i ? song : (1==i ? arti : href), len[i]))
        {
            if (2 == i)
            {
                const char *ch;
                
                index += len[i]+1;
                ch = buf+index;
                while ('"' != *ch++);
                memcpy(presult, buf+index, ch-buf-index-1);
                presult[ch-buf-index-1] = '\0';
                return true;
            }
            printf ("\n match %s", cmp_str[i]);
            {
                int j=0;
                //while (j<10)
               // printf("%2X",(unsigned char)buf[index+len[i]+j++]);
            }
            index += len[i];
            i++; 
        }
        else
        {//char temp[11]={0};memcpy(temp,buf,10);
           // printf("\n no match =%s",temp);
           // i = 0;
            index++;
           // printf("\n no match -%d",buf[index]);
        }
    }
    
    //printf("\n parse html end");
    return false;
}

char *g_download_url = NULL;
char *g_filename = NULL;
char *g_title = NULL;
char *g_artist = NULL;

static char *scrape_uri_from_download_search_result(const char *buf, int64_t len)
{
	gchar *uri = NULL;
	gchar  pattern[200] = {0};

	/*
	 * workaround buggy lyricwiki search output where it cuts the lyrics
	 * halfway through the UTF-8 symbol resulting in invalid XML.
	 */
	GRegex *reg,*reg2;
	GMatchInfo *match_info,*match_info2;

	sprintf(pattern, "<span class=\"song-title\">歌曲:.* title=\"%s\".*<span class=\"artist-title\">歌手:.*title=\"%s\".*下载LRC歌词",g_title,g_artist);
	reg = g_regex_new(pattern, (G_REGEX_MULTILINE | G_REGEX_DOTALL | G_REGEX_UNGREEDY), 0, NULL);
	  g_regex_match (reg, buf, 0, &match_info);
	    while (g_match_info_matches (match_info))
	    {
	      gchar *word = g_match_info_fetch (match_info, 0);
	      g_print ("Found: %s\n", word);

	      {
	    	  reg2 = g_regex_new("down-lrc-btn.*\.lrc\'", (G_REGEX_MULTILINE | G_REGEX_DOTALL | G_REGEX_UNGREEDY), 0, NULL);
	    	  	  g_regex_match (reg2, word, 0, &match_info2);
	    	  	    while (g_match_info_matches (match_info2))
	    	  	    {
	    	  	      gchar *word2 = g_match_info_fetch (match_info2, 0);
	    	  	    word2[strlen(word2)-1] = '\0';
	    	  	      g_print ("Found: %s\n", word2);

	    	  	      uri = str_printf("http://music.baidu.com%s",word2+strlen("down-lrc-btn { 'href':'"));
	    	  	      DEBUG_TRACE(uri);
	    	  	      g_free (word2);
	    	  	      break;
	    	  	    }
	    	  	  g_match_info_free (match_info2);
	    	  	  g_regex_unref(reg2);
	      }
	      g_free (word);

	      break;
	     // g_match_info_next (match_info, NULL);
	    }
	  g_match_info_free (match_info);
	  g_regex_unref(reg);

	return uri;
}

static bool_t download_lrc_step3(void *buf, int64_t len, void *requri)
{
	FILE *fp=NULL;
	size_t convert_len = 0;

	if (!g_download_url || strcmp(g_download_url, requri))
	{
		free(buf);
		str_unref(requri);
		mylyric_set_one_line_lrc_content("没有找到歌词！");
		return FALSE;
	}

	if(!len)
	{
		free(buf);
		mylyric_set_one_line_lrc_content("没有找到歌词！");
		return FALSE;
	}

	DEBUG_TRACE("\n==get_lyrics_step_3=len=%d===",len);

		if(NULL == fp)
		{
			if(NULL == (fp=fopen(g_filename,"wb")))
			{
				mylyric_set_one_line_lrc_content("没有找到歌词！");
				return 0;
			}
			char *g_convert_buf2 = g_malloc(len+1);
			memset(g_convert_buf2, 0, len+1);
			size_t buf1_len = len;
			char *buf1 = (char *)buf;
			char *buf2 = g_convert_buf2;
			size_t buf2_len = len;
			DEBUG_TRACE("\n==get_lyrics_step_3==before conv==");
			convert_len = convert("UTF-8","GB18030",buf1,buf1_len,buf2,buf2_len);

			fwrite(g_convert_buf2, convert_len,1,fp);
			fclose(fp);
			DEBUG_TRACE("\n==get_lyrics_step_3==save file==convert_len=%d==",convert_len);
			mylyric_mv_lrc_file(g_filename);
			mylyric_play_lrc(NULL,NULL);
			free(buf);
			g_free(g_convert_buf2);
			//free(buf2);
	}

	return TRUE;
}

static bool_t download_lrc_step2(void *buf, int64_t len, void *requri)
{
	if (strcmp(g_download_url, requri))
	{
		free(buf);
		str_unref(requri);
		mylyric_set_one_line_lrc_content("没有找到歌词！");
		return FALSE;
	}
	str_unref(requri);

	if(!len)
	{
		free(buf);
		mylyric_set_one_line_lrc_content("没有找到歌词！");
		return FALSE;
	}

	char *uri = scrape_uri_from_download_search_result(buf, len);

	if(!uri)
	{
		free(buf);
		mylyric_set_one_line_lrc_content("没有找到歌词！");
		return FALSE;
	}

	str_unref(g_download_url);
	g_download_url = uri;
	DEBUG_TRACE("\n==get_lyrics_step_2=state.uri=%s===",g_download_url);

	mylyric_set_one_line_lrc_content("正在下载歌词。。。");
	vfs_async_file_get_contents(g_download_url, download_lrc_step3, str_ref(g_download_url));

	free(buf);
	return TRUE;
}

int download_lrc(int argc, char **argv)
{
	long int downlength=0;
	char lrchttp[MAX_PATH_LENGTH]={0};
	char downhttp[MAX_PATH_LENGTH]={0};
	char urlcode[MAX_PATH_LENGTH]={0};
	char filename[MAX_PATH_LENGTH]={0};
	char filenameutf[MAX_PATH_LENGTH]={0};
    char msgbody[300]={0};
    char *lrcbuffer;
    
	strcpy(lrchttp, argv[1]);
	strcat(lrchttp," ");
	strcat(lrchttp, argv[2]);

	mylyric_set_one_line_lrc_content("正在搜索歌词。。。");

	urlencode(lrchttp, urlcode);
	sprintf(downhttp, "%s%s",
				 "http://music.baidu.com/search/lrc?key=",
				 urlcode);

	sprintf(filename, "%s", argv[1]);
	sprintf(lrchttp,"%s",argv[2]);
	sprintf(filenameutf, "%s - %s.lrc", argv[1], argv[2]);

	g_filename = g_strdup(argv[3]);
	g_artist = g_strdup(argv[1]);
	g_title   = g_strdup(argv[2]);

	str_unref(g_download_url);
	g_download_url = str_printf("%s",downhttp);
	vfs_async_file_get_contents(g_download_url, download_lrc_step2, str_ref(g_download_url));

	return;
}


