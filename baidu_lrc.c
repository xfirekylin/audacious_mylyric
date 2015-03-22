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
static bool_t download_lrc_step3(void *buf, int64_t len, void *requri);

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

#define MYLYRIC_MAX_URLS 5
typedef struct {
	gint url_cnt;
	gchar *name[MYLYRIC_MAX_URLS];
	gchar *artist[MYLYRIC_MAX_URLS];
	gchar *url[MYLYRIC_MAX_URLS];
	GtkWidget *window;
}MYLYRIC_LRC_SELECT_T;

MYLYRIC_LRC_SELECT_T mylyric_urls = {0};

void mylyric_release_lrc_urls(void)
{
	int i = 0;

	for (i=0;i<mylyric_urls.url_cnt;i++)
	{
		g_free(mylyric_urls.artist[i]);
		g_free(mylyric_urls.name[i]);
		g_free(mylyric_urls.url[i]);
	}

	if (NULL != mylyric_urls.window)
	{
		gtk_widget_destroy(mylyric_urls.window);
		mylyric_urls.window = NULL;
	}

	memset (&mylyric_urls, 0, sizeof(mylyric_urls));
}

gboolean mylyric_parse_lrc_url(const char *buf, int64_t len)
{
	gchar *uri = NULL;
	gchar  pattern[200] = {0};
	gint     i = 0;
	GRegex *reg,*reg2;
	GMatchInfo *match_info,*match_info2;

	sprintf(pattern, "<span class=\"song-title\">歌曲:.* title=\"(.*)\".*<span class=\"artist-title\">歌手:.*title=\"(.*)\".*down-lrc-btn \{ \'href\':\'(.*)\' \}\" href=\"#\">下载LRC歌词");
	reg = g_regex_new(pattern, (G_REGEX_MULTILINE | G_REGEX_DOTALL | G_REGEX_UNGREEDY), 0, NULL);
	  g_regex_match (reg, buf, 0, &match_info);
	    while (g_match_info_matches (match_info))
	    {
	      gchar *word = g_match_info_fetch (match_info, 1);
	      g_print ("Found: %s\n", word);
	      mylyric_urls.name[i] = g_strdup(word);
	      g_free (word);

	      word = g_match_info_fetch (match_info, 2);
	      g_print ("Found: %s\n", word);
	      mylyric_urls.artist[i] = g_strdup(word);
	      g_free (word);

	      word = g_match_info_fetch (match_info, 3);
	      g_print ("Found: %s\n", word);
	      mylyric_urls.url[i] = g_strdup(word);
	      g_free (word);

	      i++;

	      if (i >= MYLYRIC_MAX_URLS)
	      {
	    	  break;
	      }
	      g_match_info_next (match_info, NULL);
	    }

	    mylyric_urls.url_cnt = i;

	  g_match_info_free (match_info);
	  g_regex_unref(reg);

	  return i > 0;
}

void mylyric_button_cancel_clicked(GtkWidget *widget,gpointer data )
{
	//gtk_clist_clear( (GtkCList *) data);
	gtk_widget_destroy(mylyric_urls.window);
	mylyric_urls.window = NULL;
	DEBUG_TRACE("mylrric_button_cancel_clicked");
}

void mylyric_button_download_clicked(GtkWidget *widget,gpointer data )
{
	GtkTreeIter iter;
	  GtkTreeModel *model;
	  int value;

	  GtkTreeSelection *select;

	  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));

	  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(select), &model, &iter))
	  {

	        gtk_tree_model_get(model, &iter, 0, &value,  -1);

	        DEBUG_TRACE("mylyric_button_download_clicked select index = %d",value);

	    	gchar *uri = str_printf("http://music.baidu.com%s",mylyric_urls.url[value-1]);
	    	str_unref(g_download_url);
	    	g_download_url = uri;
	    	DEBUG_TRACE("\n==get_lyrics_step_2=state.uri=%s===",g_download_url);

	    	mylyric_set_one_line_lrc_content("正在下载歌词。。。");
	    	vfs_async_file_get_contents(g_download_url, download_lrc_step3, str_ref(g_download_url));
	  }

	gtk_widget_destroy(mylyric_urls.window);
	mylyric_urls.window = NULL;
}

enum{
col_index = 0,
col_name ,
col_artist,
n_cols
};

void model_data_new(GtkTreeModel* store,gint index,
		const gchar* name, const gchar* artist) {
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(store), &iter);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter,
									col_index,index,
									col_name, name,
									col_artist, artist,
									-1);
}

GtkTreeModel* create_model() {
		GtkListStore *store;
		store = gtk_list_store_new (n_cols,G_TYPE_INT,
				G_TYPE_STRING,G_TYPE_STRING);
		return GTK_TREE_MODEL(store);
}

void arrange_tree_view(GtkWidget* view) {
	GtkCellRenderer* renderer;

	// col 1: name
		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_insert_column_with_attributes(
				GTK_TREE_VIEW(view), -1, "序号", renderer, "text", col_index, NULL);

	// col 2: name
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes(
			GTK_TREE_VIEW(view), -1, "歌手", renderer, "text", col_artist, NULL);

	// col 3: artist
	gtk_tree_view_insert_column_with_attributes(
			GTK_TREE_VIEW(view), -1, "歌名", renderer, "text",  col_name, NULL);
}


void mylyric_open_lrc_search_result(GtkWidget *widget,gpointer data)
{
	GtkWidget *vbox, *hbox;
	GtkWidget *scrolled_window, *clist,*view;
	GtkWidget *button_download, *button_cancel;
     gint result;
     gint i = 0;

      if (NULL != mylyric_urls.window)
      {
    	  gtk_widget_destroy(mylyric_urls.window);
      }

     GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
     mylyric_urls.window = window;
 	gtk_window_set_default_size ( GTK_WINDOW(window), 300, 400);

       vbox=gtk_vbox_new(FALSE, 2);
       gtk_container_add(GTK_CONTAINER(window), vbox);

       scrolled_window = gtk_scrolled_window_new (NULL, NULL);
       gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

       view = gtk_tree_view_new();
       gtk_container_add( GTK_CONTAINER(scrolled_window), view);

       // arrange view columns
       arrange_tree_view(view);

       // set model
       GtkTreeModel* store = create_model();
       gtk_tree_view_set_model ( GTK_TREE_VIEW(view), store);
       GtkTreeSelection *select;
       select = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
       gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

       for (i=0;i<mylyric_urls.url_cnt;i++)
       {
    	   model_data_new(store, i+1,mylyric_urls.name[i], mylyric_urls.artist[i]);
       }

       hbox = gtk_hbox_new(FALSE, 0);
       gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

       button_download = gtk_button_new_with_label("下载");
       button_cancel = gtk_button_new_with_label("取消");
       gtk_box_pack_start(GTK_BOX(hbox), button_download, TRUE, TRUE, 0);
       gtk_box_pack_start(GTK_BOX(hbox), button_cancel, TRUE, TRUE, 0);

       g_signal_connect(GTK_OBJECT(button_download), "clicked",
    		   G_CALLBACK (mylyric_button_download_clicked) ,(gpointer) view);

       g_signal_connect(GTK_OBJECT(button_cancel), "clicked",
    		   G_CALLBACK (mylyric_button_cancel_clicked) ,(gpointer) view);

        gtk_widget_show_all(window);

}

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

	sprintf(pattern, "<span class=\"song-title\">歌曲:.* title=\"%s\".*<span class=\"artist-title\">歌手:.*title=\"%s\".*down-lrc-btn \{ \'href\':\'(.*)\' \}\" href=\"#\">下载LRC歌词",g_title,g_artist);
	reg = g_regex_new(pattern, (G_REGEX_MULTILINE | G_REGEX_DOTALL | G_REGEX_UNGREEDY), 0, NULL);
	  g_regex_match (reg, buf, 0, &match_info);
	    while (g_match_info_matches (match_info))
	    {
			gchar *word = g_match_info_fetch (match_info, 1);
			g_print ("Found: %s\n", word);

			uri = str_printf("http://music.baidu.com%s",word);
			DEBUG_TRACE(uri);
			g_free (word);

			break;
	     // g_match_info_next (match_info, NULL);
	    }

	  g_match_info_free (match_info);
	  g_regex_unref(reg);

	  mylyric_release_lrc_urls();
	  if (NULL == uri)
	  {
		  if (mylyric_parse_lrc_url(buf, len))
		  {
			  mylyric_open_lrc_search_result(NULL, NULL);
		  }
	  }
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


