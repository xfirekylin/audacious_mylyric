#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include <audacious/drct.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <audacious/plugin.h>
#include <audacious/plugins.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/vfs_async.h>
    typedef long long  INT64;
#endif
//#define test
//#define html
//#define typebuf
#define FILELENGTH 400000


char chbuf[FILELENGTH]={0};
char *file=NULL;


typedef struct {
	char *filename; /* of song file */
	char *title, *artist;
	char *uri; /* URI we are trying to retrieve */
} LyricsState;

static LyricsState state;


int parsehtml(const char *buf, const char *match, char ***presult)
{
	int len = strlen(buf);
	int i=0;
	int matchlen=0;
	int resultcnt=0;
	char **result;
	typedef struct resultlist
	{
		char *res;
		struct resultlist *next;
	}RESULT_LIST;

	RESULT_LIST *plist;
	RESULT_LIST rlist;
	RESULT_LIST *prelist;

	rlist.next = NULL;
	rlist.res = NULL;

	printf("\n%s,len=%d\n",match,len);
	while(i<len)
	{
		if (0 == memcmp(buf+i, match, strlen(match) ))
		{
			resultcnt++;
			matchlen = i+strlen(match);
			i=matchlen;
			while('"' != *(buf+i)) i++;
			
			plist = &rlist;
			prelist = plist;
			while(NULL != plist)
			{
				prelist = plist;
				plist = plist->next;
			}

			if(NULL == prelist->res)
			{
				prelist->res = (char *)malloc(i-matchlen+1);
				plist = prelist;
			}
			else
			{
				prelist->next = (RESULT_LIST *)malloc(sizeof(RESULT_LIST));
				plist = prelist->next;
				plist->next = NULL;
				plist->res = (char *)malloc(i-matchlen+1);
			}
			memset(plist->res, 0, i-matchlen+1);
			memcpy(plist->res, buf+matchlen, i-matchlen);
			
		}
		i++; 
	}

	if(resultcnt > 0)
	{
		result = (char **)malloc(sizeof(char *)*(resultcnt+1));
		plist = &rlist;
		*result = plist->res;
		plist = plist->next;
		resultcnt = 1;
		while(NULL != plist)
		{
			result[resultcnt++] = plist->res;
			prelist=plist;
			plist = plist->next;
			free(prelist);
		}
		
		result[resultcnt] = NULL;
		*presult = result;
	}
	return 0;
}




int main(int argc, char **argv)
{
	if (4 == argc)
	{
		download_by_http(argv[1],NULL, argv[3]);
	}
	else if(3 == argc)
	{
		download_lrc(argc, argv);
	}
	else if(2 == argc)
	{
		char *filename=argv[1]+strlen(argv[1]) -1;
		
		while (*filename-- != '/');
		filename = filename+2;
		download_by_http(argv[1], NULL, filename);
	}
	else
	{
		printf("useage: download url \n download artist title\n");
		return 0;
	}

	return 0;
}

INT64 create_code(int Id, wchar_t *data1)
{
    INT64 length = wcslen(data1);
	INT64 tmp2=0;
	INT64 tmp3=0;
	INT64 tmp7=0;
	INT64 i;
    INT64 ch;
	INT64 tmp1 = (Id & 0x0000FF00) >> 8;							//����8λ��Ϊ0x0000015F
															//#tmp1 0x0000005F
	char data[100];
	char *data2 = (char *)data1;

	for(i=0;i<length/2;i++)
	{
		data[i] = data2[i * 2] * 16 + data2[i * 2+1];
	}

	length = length / 2;

	if ( (Id & 0x00FF0000) == 0 )
		tmp3 = 0x000000FF & (~tmp1);							//#CL 0x000000E7
	else
		tmp3 = 0x000000FF & ((Id & 0x00FF0000) >> 16);		//#����16λ��Ϊ0x00000001
	
	tmp3 = tmp3 | ((0x000000FF & Id) << 8);					//#tmp3 0x00001801
	tmp3 = tmp3 << 8;										//#tmp3 0x00180100
	tmp3 = tmp3 | (0x000000FF & tmp1);						//#tmp3 0x0018015F
	tmp3 = tmp3 << 8;										//#tmp3 0x18015F00
	if ( (Id & 0xFF000000) == 0 )
		tmp3 = tmp3 | (0x000000FF & (~Id));					//#tmp3 0x18015FE7
	else 
		tmp3 = tmp3 | (0x000000FF & (Id >> 24));			//#����24λ��Ϊ0x00000000
	
	//#tmp3	18015FE7
	
	i=length-1;
	while(i >= 0)
	{
		ch = data[i];
		if (ch >= 0x80)
			ch = ch - 0x100;
		tmp1 = (ch + tmp2) & 0x00000000FFFFFFFF;
		tmp2 = (tmp2 << (i%2 + 4)) & 0x00000000FFFFFFFF;
		tmp2 = (tmp1 + tmp2) & 0x00000000FFFFFFFF;
		//#tmp2 = (ord(data[i])) + tmp2 + ((tmp2 << (i%2 + 4)) & 0x00000000FFFFFFFF);
		i -= 1;
	}
	
	//#tmp2 88203cc2
	i=0;
	tmp1=0;
	while(i<=length-1)
	{
		ch = data[i];
		if (ch>= 128)
			ch = ch- 256;
		tmp7 = (ch + tmp1) & 0x00000000FFFFFFFF;
		tmp1 = (tmp1 << (i%2 + 3)) & 0x00000000FFFFFFFF;
		tmp1 = (tmp1 + tmp7) & 0x00000000FFFFFFFF;
		//#tmp1 = (ord(data[i])) + tmp1 + ((tmp1 << (i%2 + 3)) & 0x00000000FFFFFFFF)
		i += 1;
	}
	
	//#EBX 5CC0B3BA
	
	//#EDX = EBX | Id
	//#EBX = EBX | tmp3
	tmp1 = (((((tmp2 ^ tmp3) & 0x00000000FFFFFFFF) + (tmp1 | Id)) & 0x00000000FFFFFFFF) * (tmp1 | tmp3)) & 0x00000000FFFFFFFF;
	tmp1 = (tmp1 * (tmp2 ^ Id)) & 0x00000000FFFFFFFF;
	
	if (tmp1 > 0x80000000)
	{
	    tmp2 = (INT64)1 << 32;//0x100000000;
		tmp1 = tmp1 - tmp2;
	}
	return tmp1;

}
static void get_lyrics_step_1(void);
int download_lrc(int argc, char **argv)
{
	char *lrcbuffer;
	wchar_t artist[20]={0};
	wchar_t title[20]={0};
	wchar_t art_title[40]={0};
	char artistunicodestr[80]={0};
	char titleunicodestr[80] = {0};
	char arttitleunicodestr[80]={0};
	char downhttp[100]={0};
	char msgbody[300]={0};
	char **lrcid=NULL;
	char **temp;
	char filename[30] = {0};

	#ifdef WIN_OS
		setlocale(LC_ALL, ".936");
	#endif

	#ifdef LINUX_OS
		setlocale(LC_ALL, "");
	#endif
	mbstowcs(artist, argv[1], sizeof(artist));
	mbstowcs(title, argv[2], sizeof(title));
    wcscpy(art_title,artist);
    wcscat(art_title,title);
    
	unicodetostr(artist, artistunicodestr, 0);
	unicodetostr(title, titleunicodestr, 0);

	sprintf(downhttp, "http://ttlrcct2.ttplayer.com/dll/lyricsvr.dll?sh?Artist=%s&Title=%s&Flags=0",
		artistunicodestr,titleunicodestr);
	sprintf(msgbody,"Accept:image/gif,image/x-xbitmap,image/jpg,text/html,*/*\r\n"
			"User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n"
			"Connection:Keep-Alive\r\n"
			"Cache-Control:no-cache\r\n"
			"Referer:http://lrcct2.ttplayer.com/\r\n\r\n");

	download_by_http_return_buffer(downhttp,msgbody,&lrcbuffer);
	parsehtml(lrcbuffer, "id=\"", &lrcid);

	#ifdef LINUX_OS
		setlocale(LC_ALL, "zh_CN.gb2312");
	#endif
	#ifdef WIN_OS
		setlocale(LC_ALL, ".936");
	#endif
		wcstombs(filename, artist, sizeof(filename));
		strcat(filename, "-");
		wcstombs(filename+strlen(filename), title, sizeof(filename)-strlen(filename));
		strcat(filename,".lrc");

#if 0
		state.filename = g_strdup(filename);
		state.artist = g_strdup(argv[1]);
		state.title = g_strdup(argv[2]);
		state.uri = NULL;
		get_lyrics_step_1();
		return;
#endif

	temp = lrcid;
	while(NULL != temp && NULL != *temp)
	{
		sprintf(downhttp, "http://ttlrcct2.ttplayer.com/dll/lyricsvr.dll?dl?Id=%s&Code=%lld&uid=01&mac=%012x",
				*temp,
				create_code(atoi(*temp),art_title),
				rand()
				);
		sprintf(msgbody,"Accept:image/gif,image/x-xbitmap,image/jpg,text/html,*/*\r\n"
			"User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n"
			"Connection:Keep-Alive\r\n"
			"Cache-Control:no-cache\r\n"
			"Referer: http://www.qianqian.com/lrcresult_frame.php?pe=1&qword=8&qfield=12&page=12\r\n\r\n");
		//sprintf(filename, "%s-%s.lrc", argv[1], argv[2]);
		if (0 ==download_by_http(downhttp,msgbody, filename))
		{
			temp++;
		}
		else
		{
			break;
		}
	}

	if(NULL != lrcbuffer)
	{
		free(lrcbuffer);
	}

	if(NULL != lrcid)
	{	
		temp = lrcid;

		while(NULL != *lrcid)
		{
			free(*lrcid);
			lrcid++;
		}
		free(temp);
	}

	return 0;
}

static void libxml_error_handler(void *ctx, const char *msg, ...)
{
}

static char *scrape_uri_from_lyricwiki_search_result(const char *buf, int64_t len)
{
	xmlDocPtr doc;
	gchar *uri = NULL;

	/*
	 * workaround buggy lyricwiki search output where it cuts the lyrics
	 * halfway through the UTF-8 symbol resulting in invalid XML.
	 */
	GRegex *reg;

	reg = g_regex_new("<(lyrics?)>.*</\\1>", (G_REGEX_MULTILINE | G_REGEX_DOTALL | G_REGEX_UNGREEDY), 0, NULL);
	gchar *newbuf = g_regex_replace_literal(reg, buf, len, 0, "", G_REGEX_MATCH_NEWLINE_ANY, NULL);
	g_regex_unref(reg);

	/*
	 * temporarily set our error-handling functor to our suppression function,
	 * but we have to set it back because other components of Audacious depend
	 * on libxml and we don't want to step on their code paths.
	 *
	 * unfortunately, libxml is anti-social and provides us with no way to get
	 * the previous error functor, so we just have to set it back to default after
	 * parsing and hope for the best.
	 */
	xmlSetGenericErrorFunc(NULL, libxml_error_handler);
	doc = xmlParseMemory(newbuf, strlen(newbuf));
	xmlSetGenericErrorFunc(NULL, NULL);

	if (doc != NULL)
	{
		xmlNodePtr root, cur;

		root = xmlDocGetRootElement(doc);

		for (cur = root->xmlChildrenNode; cur; cur = cur->next)
		{
			if (xmlStrEqual(cur->name, (xmlChar *) "url"))
			{
				xmlChar *lyric;
				gchar *basename;

				lyric = xmlNodeGetContent(cur);
				basename = g_path_get_basename((gchar *) lyric);

				uri = str_printf("http://lyrics.wikia.com/index.php?action=edit"
				 "&title=%s", basename);

				g_free(basename);
				xmlFree(lyric);
			}
		}

		xmlFreeDoc(doc);
	}

	g_free(newbuf);

	return uri;
}

static bool_t get_lyrics_step_3(void *buf, int64_t len, void *requri)
{
	FILE *fp=NULL;

	if (!state.uri || strcmp(state.uri, requri))
	{
		free(buf);
		str_unref(requri);
		return FALSE;
	}
	str_unref(requri);

	if(!len)
	{
		free(buf);
		return FALSE;
	}

	DEBUG_TRACE("\n==get_lyrics_step_3=len=%d===",len);

	if(NULL == fp)
	{
		if(NULL == (fp=fopen(state.filename,"wb")))
		{
			printf("can not open file %s",state.filename);
				return 0;
		}
		fwrite(buf, len,1,fp);
		free(buf);
	}

	return TRUE;
}

static bool_t get_lyrics_step_2(void *buf, int64_t len, void *requri)
{
	if (strcmp(state.uri, requri))
	{
		free(buf);
		str_unref(requri);
		return FALSE;
	}
	str_unref(requri);

	if(!len)
	{
		free(buf);
		return FALSE;
	}

	char *uri = scrape_uri_from_lyricwiki_search_result(buf, len);

	if(!uri)
	{
		free(buf);
		return FALSE;
	}

	str_unref(state.uri);
	state.uri = uri;
	DEBUG_TRACE("\n==get_lyrics_step_2=state.uri=%s===",state.uri);

	vfs_async_file_get_contents(uri, get_lyrics_step_3, str_ref(state.uri));

	free(buf);
	return TRUE;
}

static void get_lyrics_step_1(void)
{
	DEBUG_TRACE("\n===artist=%s,title=%s,filename=%s===",state.artist,state.title,state.filename);
	if(!state.artist || !state.title)
	{
		return;
	}

	char title_buf[strlen(state.title) * 3 + 1];
	char artist_buf[strlen(state.artist) * 3 + 1];
	str_encode_percent(state.title, -1, title_buf);
	str_encode_percent(state.artist, -1, artist_buf);

	str_unref(state.uri);
	state.uri = str_printf("http://lyrics.wikia.com/api.php?action=lyrics&"
	 "artist=%s&song=%s&fmt=xml", artist_buf, title_buf);


	vfs_async_file_get_contents(state.uri, get_lyrics_step_2, str_ref(state.uri));
}

