/*
 * Audacious: A cross-platform multimedia player.
 * Copyright (c) 2005  Audacious Team
 */

//#include "config.h"
#include "iconv.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <audacious/misc.h>
#include <audacious/drct.h>
#include <audacious/i18n.h>
#include <audacious/plugin.h>
#include <audacious/preferences.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>

#include "utility.h"

static const PluginPreferences preferences;

static gboolean init (void);
static void cleanup(void);
static void mylyric_playback_begin(gpointer unused, gpointer unused2);
static void mylyric_playback_end(gpointer unused, gpointer unused2);
static void mylyric_playlist_eof(gpointer unused, gpointer unused2);


static char lrc_content[4000] ={0},lrc2[4000]={0};
static char cur_font[50];
static int lrcpos_left = 0;
static double lrcpos_top = 0;
static int cur_win_height;
static int cur_win_width;
static gboolean auto_resize = TRUE;
static gboolean user_resize ;
typedef struct _Time_List
{
	int time;
	int offset;
	int len; 
	int line;
struct _Time_List *next;
}Time_List;

static char *lyric_position = NULL;
static char *lyric_font = NULL;
static char *lyric_forecolor = NULL;
static char *lyric_backcolor = NULL;
static char *lyric_focuscolor = NULL;

static GtkWidget *configure_vbox = NULL;
static GtkWidget *pos_entry, *font_btn, *cmd_end_entry, *cmd_ttc_entry;
static GtkWidget  *fore_color_btn,*back_color_btn,*focus_color_btn;
static GtkWidget *cmd_warn_label, *cmd_warn_img;
static GtkWidget *window,*textview,*scrolled_win,*layout;
static GtkTextBuffer *buffer;
static guint scroll_lrc_timer_id;
static Time_List *lrc_time_list,*cur_time_list;
static Time_List *pre_time;

AUD_GENERAL_PLUGIN
(
    .name = N_("My Lyric"),
    .domain = N_("audacious-plugins"),
    .prefs = & preferences,
    .init = init,
    .cleanup = cleanup
)

/**
 * Escapes characters that are special to the shell inside double quotes.
 *
 * @param string String to be escaped.
 * @return Given string with special characters escaped. Must be freed with g_free().
 */
static gchar *escape_shell_chars(const gchar * string)
{
    const gchar *special = "$`\"\\";    /* Characters to escape */
    const gchar *in = string;
    gchar *out, *escaped;
    gint num = 0;

    while (*in != '\0')
        if (strchr(special, *in++))
            num++;

    escaped = g_malloc(strlen(string) + num + 1);

    in = string;
    out = escaped;

    while (*in != '\0') {
        if (strchr(special, *in))
            *out++ = '\\';
        *out++ = *in++;
    }
    *out = '\0';

    return escaped;
}

void mylyric_mv_lrc_file(char *name)
{
	if (NULL == name || NULL == lyric_position)
	{
		return;
	}

	size_t len = strlen(name) + strlen(lyric_position) + 10;
	char *cmd_str = malloc(len);

	if (NULL == cmd_str)
	{
		return;
	}

	memset(cmd_str, 0, len);
	sprintf(cmd_str,"mv \"%s\" %s/", name, lyric_position);

	DEBUG_TRACE("\ncmd_str=%s",cmd_str);
	system(cmd_str);
	free(cmd_str);
}

static void bury_child(int signal)
{
    waitpid(-1, NULL, WNOHANG);
}

static void execute_command(char *cmd)
{
    char *argv[4] = {"/bin/sh", "-c", NULL, NULL};
    int i;
    argv[2] = cmd;
    signal(SIGCHLD, bury_child);
    if (fork() == 0)
    {
        /* We don't want this process to hog the audio device etc */
        for (i = 3; i < 255; i++)
            close(i);
        execv("/bin/sh", argv);
    }
}

void closetimer (GtkWidget *window, gpointer data)
{
	DEBUG_TRACE("close timer");
	gtk_timeout_remove(scroll_lrc_timer_id);
	g_source_remove(scroll_lrc_timer_id);
}

gboolean resize_lrc_win(GtkWidget* widget, GdkDragContext* config,char * host)//GdkEventConfigure
{
	gint x,y;
	GtkRequisition requisition;
	GtkAllocation allocation;

	
	gtk_window_get_size(GTK_WINDOW(window),&x,&y);
	gtk_widget_size_request(textview,&requisition);


	if (x != cur_win_width)
	{
		if (requisition.width < x)
		{
			allocation.x = (x-requisition.width)/2;
			allocation.y = 0;
			allocation.width = requisition.width;
			allocation.height = y;//requisition.height;

			gtk_widget_size_allocate(GTK_WIDGET(textview),&allocation);
			gtk_layout_move(GTK_LAYOUT(layout),textview, allocation.x, lrcpos_top);
			lrcpos_left = allocation.x;
		}
		cur_win_width = x;
	}

	user_resize = TRUE;

	return 0;
}

static void read_config(void)
{
	if (lyric_position != NULL)
		g_free(lyric_position);

	if (lyric_font != NULL)
		g_free(lyric_font);

	if (lyric_forecolor != NULL)
		g_free(lyric_forecolor);

	if (lyric_backcolor != NULL)
		g_free(lyric_backcolor);

	if (lyric_focuscolor != NULL)
		g_free(lyric_focuscolor);

    lyric_position = aud_get_string("my_lyric", "lyric_position");
    lyric_font = aud_get_string("my_lyric", "lyric_font");
    lyric_forecolor = aud_get_string("my_lyric", "lyric_forecolor");
    lyric_backcolor = aud_get_string("my_lyric", "lyric_backcolor");
    lyric_focuscolor = aud_get_string("my_lyric", "lyric_focuscolor");

    DEBUG_TRACE("========================read_config begin==================\n");
    DEBUG_TRACE("lyric_position:%s\n",lyric_position);
    DEBUG_TRACE("lyric_font:%s\n",lyric_font);
    DEBUG_TRACE("lyric_forecolor:%s\n",lyric_forecolor);
    DEBUG_TRACE("lyric_backcolor:%s\n",lyric_backcolor);
    DEBUG_TRACE("lyric_focuscolor:%s\n",lyric_focuscolor);
    DEBUG_TRACE("========================read_config end==================\n");
}

static void save_config(void)
{
	aud_set_string("my_lyric", "lyric_position", lyric_position);
	aud_set_string("my_lyric", "lyric_font", lyric_font);
	aud_set_string("my_lyric", "lyric_forecolor", lyric_forecolor);
	aud_set_string("my_lyric", "lyric_backcolor", lyric_backcolor);
    aud_set_string("my_lyric", "lyric_focuscolor",lyric_focuscolor);

    DEBUG_TRACE("========================save_config begin==================\n");
    DEBUG_TRACE("lyric_position:%s\n",lyric_position);
    DEBUG_TRACE("lyric_font:%s\n",lyric_font);
    DEBUG_TRACE("lyric_forecolor:%s\n",lyric_forecolor);
    DEBUG_TRACE("lyric_backcolor:%s\n",lyric_backcolor);
    DEBUG_TRACE("lyric_focuscolor:%s\n",lyric_focuscolor);
    DEBUG_TRACE("========================save_config end==================\n");

}

static void cleanup(void)
{
    hook_dissociate("playback begin", mylyric_playback_begin);
    hook_dissociate("playback end", mylyric_playback_end);
    hook_dissociate("playlist end reached", mylyric_playlist_eof);
    // hook_dissociate( "playlist set info" , mylyric_playback_ttc);

	g_free(lyric_position);
	g_free(lyric_font);
	g_free(lyric_forecolor);
	g_free(lyric_backcolor);
	g_free(lyric_focuscolor);
	lyric_position = NULL;
	lyric_font = NULL;
	lyric_forecolor = NULL;
	lyric_backcolor = NULL;
	lyric_focuscolor= NULL;
    signal(SIGCHLD, SIG_DFL);
}

static int check_command(char *command)
{
    const char *dangerous = "fns";
    char *c;
    int qu = 0;

    for (c = command; *c != '\0'; c++)
    {
        if (*c == '"' && (c == command || *(c - 1) != '\\'))
            qu = !qu;
        else if (*c == '%' && !qu && strchr(dangerous, *(c + 1)))
            return -1;
    }
    return 0;
}

static void applet_save_and_close(GtkWidget *w, gpointer data)
{
	char *lyric_pos, *new_font, *fore_color, *back_color;
        char focus_color[20]={0};
	GdkColor color;
        static char cur_forecolor[20];
        static char cur_backcolor[20];
        static char cur_focuscolor[20];


    if (0 != strcmp(cur_font, lyric_font) && NULL != textview)
	{
                PangoFontDescription *font;

                font = pango_font_description_from_string (lyric_font);
                strcpy(cur_font, lyric_font);
                gtk_widget_modify_font(textview, font);
	}


    if (0 != strcmp(cur_forecolor, lyric_forecolor) && NULL != textview)
	{
                strcpy(cur_forecolor, lyric_forecolor);
                gdk_color_parse(lyric_forecolor,&color);
                gtk_widget_modify_fg(textview,GTK_STATE_NORMAL,&color);
	}

    if (0 != strcmp(cur_focuscolor, lyric_focuscolor) && NULL != textview)
	{
                GtkTextIter start,end;
                GtkTextTagTable *tagtable;
                GtkTextTag *tag;

                buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
                strcpy(cur_focuscolor, lyric_focuscolor);

                tagtable = gtk_text_buffer_get_tag_table(buffer);
                tag = gtk_text_tag_table_lookup(tagtable,"forecolors");
        
                if (NULL != pre_time && pre_time->len > 2)
	           {//
                        DEBUG_TRACE("\nchange focus color ok");
						gtk_text_buffer_get_iter_at_line (buffer,&start,pre_time->line-1);
						gtk_text_buffer_get_iter_at_line_index (buffer,&end,pre_time->line-1,pre_time->len-1);
						gtk_text_buffer_remove_tag_by_name (buffer,"forecolors",&start,&end);
                        gtk_text_tag_table_remove(tagtable,tag);
                        gtk_text_buffer_create_tag (buffer, "forecolors","foreground", lyric_focuscolor, NULL);
                        gtk_text_buffer_apply_tag_by_name (buffer,"forecolors",&start,&end);
                }  
                else
                {      
                        gtk_text_tag_table_remove(tagtable,tag);
                        gtk_text_buffer_create_tag (buffer, "forecolors","foreground", lyric_focuscolor, NULL);
                }
 	}
	
	if (0 != strcmp(cur_backcolor, lyric_backcolor) && NULL != textview && NULL != layout)
	{
                strcpy(cur_backcolor, lyric_backcolor);
                gdk_color_parse(lyric_backcolor,&color);
                gtk_widget_modify_bg(textview,GTK_STATE_NORMAL,&color);
                gtk_widget_modify_bg(layout,GTK_STATE_NORMAL,&color);
                //gtk_widget_modify_base(textview,GTK_STATE_NORMAL,&color);
	}
	
}

void closeApp ( GtkWidget *window, gpointer data)
{

}

void mylyric_free_time_list(void)
{
	Time_List * cur_time_list = lrc_time_list;

	while (NULL != cur_time_list)
	{
		lrc_time_list = cur_time_list->next;
		free(cur_time_list);
		cur_time_list = lrc_time_list;
	}
}

void mylyric_set_one_line_lrc_content(char *lrc)
{
	gint time,x,y;
	GtkRequisition requisition;
	GtkTextIter start,end;


	if (NULL == buffer || NULL == lrc)
	{
		return;
	}

	mylyric_free_time_list();

	gtk_text_buffer_set_text (buffer, lrc, -1);

	gtk_window_get_size(GTK_WINDOW(window),&x,&y);
	gtk_widget_size_request(textview,&requisition);
	gtk_window_resize(GTK_WINDOW(window),requisition.width,y);
	gtk_layout_move(GTK_LAYOUT(layout),textview, lrcpos_left, y/2);
}

void mylyric_show_window(void)
{
	GdkColor color;
	PangoFontDescription *font;

	gdk_color_parse(lyric_forecolor,&color);
	gtk_widget_modify_fg(textview,GTK_STATE_NORMAL,&color);
	gdk_color_parse(lyric_backcolor,&color);
	gtk_widget_modify_bg(textview,GTK_STATE_NORMAL,&color);
	gtk_widget_modify_bg(layout,GTK_STATE_NORMAL,&color);

	font = pango_font_description_from_string (lyric_font);
	strcpy(cur_font, lyric_font);
	gtk_widget_modify_font (textview, font);

	gtk_text_view_set_justification (GTK_TEXT_VIEW (textview), GTK_JUSTIFY_CENTER);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (textview), 5);
	gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (textview), 5);
	gtk_text_view_set_pixels_inside_wrap (GTK_TEXT_VIEW (textview), 5);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), 10);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (textview), 10);

	gtk_text_buffer_create_tag (buffer, "back","background",lyric_backcolor, NULL);
	gtk_text_buffer_create_tag (buffer, "forecolors","foreground", lyric_focuscolor, NULL);

	gtk_widget_show_all(window);
}

static gboolean init (void)
{
    read_config();

    hook_associate("playback begin", mylyric_playback_begin, NULL);
    hook_associate("playback end", mylyric_playback_end, NULL);
    hook_associate("playlist end reached", mylyric_playlist_eof, NULL);

	gtk_init (NULL, NULL);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size ( GTK_WINDOW(window), 300, 400);
	textview = gtk_text_view_new ();
	layout = gtk_layout_new(NULL,NULL);
	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				                                GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	GdkColor bgcolor = {333,255,255,255};
	gtk_widget_modify_bg(window,GTK_STATE_NORMAL,&bgcolor);
	gtk_container_add (GTK_CONTAINER (layout), textview);
//	gtk_container_add (GTK_CONTAINER (scrolled_win), layout);
	gtk_container_add (GTK_CONTAINER (window), layout);
	g_signal_connect ( GTK_OBJECT (window), "destroy",GTK_SIGNAL_FUNC ( closeApp), NULL);
	g_signal_connect(GTK_OBJECT(window),"destroy", GTK_SIGNAL_FUNC(closetimer),NULL);
	g_signal_connect(GTK_OBJECT(window),"drag-end", G_CALLBACK(resize_lrc_win),NULL);//configure-event expose-event	
	applet_save_and_close(NULL, NULL);
	mylyric_show_window();

	DEBUG_TRACE("\n=======init======\n");
    return TRUE;
}

void parse_lrc(Time_List **phead, char *inbuf, char **outbuf)
{
	int offset = 0;
	int len = 0;
	int time = 0;
	int i = 0;
	_Bool timestartflag = false;
	typedef enum
	{
		TITLE,
		ARTIST,
		ALBUM,
		LRC_MAKER
	}HEAD;
	int head_offset[4]={0};
	int head_len[4]={0};
	Time_List *un_fill_offset_time = NULL,*p=NULL,*pre,*q,*head,listhead;
	long	int lrc_cnt=0;

	if ('\0' == inbuf[0])
	{
		*phead = NULL;
		*outbuf = NULL;
		return;
	}

	p = (Time_List *)malloc(sizeof(Time_List));
	assert(NULL != p);
	*phead = p;
	un_fill_offset_time = p;
	p->next =NULL;
	for(i=0;'\0' != inbuf[i];i++)
	{
		if ('[' == inbuf[i])
		{
			if (i>offset)
			{
				char temp[100]={0};
				while (NULL != un_fill_offset_time)
				{
					un_fill_offset_time->offset = offset;
					un_fill_offset_time->len = i-offset;
					lrc_cnt += (un_fill_offset_time->len);
					DEBUG_TRACE(" %d",lrc_cnt);
					un_fill_offset_time =un_fill_offset_time->next;
				}
			un_fill_offset_time = p;
				memcpy(temp, inbuf+offset,i-offset);
					DEBUG_TRACE("i=%d,offset=%d,%s=======================================\n",i,offset,temp);
			}
			offset = i;
			timestartflag = true;
		}
		else if (']'==inbuf[i] && timestartflag)
		{
			if (i-offset<5)
			{
				timestartflag = false;
			}
			else if ((0 == memcmp(&inbuf[offset+1],"ar:",3))
					|| (0 == memcmp(&inbuf[offset+1],"ti:",3))
					|| (0 == memcmp(&inbuf[offset+1],"al:",3))
					|| (0 == memcmp(&inbuf[offset+1],"by:",3)))
			{
				p->time = -1;
				p->offset = offset + 4;
				p->len = i-p->offset;
				lrc_cnt +=p->len;

				p->next = (Time_List*)malloc(sizeof(Time_List));
				p = p->next;
				assert(NULL != p);
				p->next = NULL;
				un_fill_offset_time = p;
				offset = i+1;
			}
			else if(6 == i-offset)
			{
				if (inbuf[offset+1]>='0' && inbuf[offset+1]<='9' 
					&&inbuf[offset+2]>='0' && inbuf[offset+2]<='9'
					&&inbuf[offset+3]==':'
					&&inbuf[offset+4]>='0' && inbuf[offset+4]<='9'
					&&inbuf[offset+5]>='0' && inbuf[offset+5]<='9')
				{
					p->time = (inbuf[offset+1]-'0')*10*60*1000+(inbuf[offset+2]-'0')*60*1000+
								(inbuf[offset+4]-'0')*10*1000+(inbuf[offset+5]-'0')*1000;
					p->next = (Time_List*)malloc(sizeof(Time_List));
					p = p->next;
					assert(NULL != p);
					p->next = NULL;
					offset = i+1;
				}
				DEBUG_TRACE("\n6 time\n");
			}
			else if(9 == i-offset)
			{
				if (inbuf[offset+1]>='0' && inbuf[offset+1]<='9' 
					&&inbuf[offset+2]>='0' && inbuf[offset+2]<='9'
					&&inbuf[offset+3]==':'
					&&inbuf[offset+4]>='0' && inbuf[offset+4]<='9'
					&&inbuf[offset+5]>='0' && inbuf[offset+5]<='9'
					&&inbuf[offset+6]=='.'
					&&inbuf[offset+7]>='0' && inbuf[offset+7]<='9'
					&&inbuf[offset+8]>='0' && inbuf[offset+8]<='9')
				{
					p->time = (inbuf[offset+1]-'0')*10*60*1000+(inbuf[offset+2]-'0')*60*1000+
								(inbuf[offset+4]-'0')*10*1000+(inbuf[offset+5]-'0')*1000
								+(inbuf[offset+7]-'0')*100+(inbuf[offset+8]-'0')*10;
					p->next = (Time_List*)malloc(sizeof(Time_List));
					assert(NULL != p->next);
					p = p->next;
					p->next = NULL;
					offset = i+1;
				DEBUG_TRACE("\n9 time offset=%d\n",offset);
				}
				
			}

			timestartflag = false;
		}
		 
	}

	do
	{
		un_fill_offset_time->offset = offset;
		un_fill_offset_time->len = i-offset;
		lrc_cnt += un_fill_offset_time->len;
	}while (p != un_fill_offset_time->next && NULL != (un_fill_offset_time =un_fill_offset_time->next));

	free(p);
	un_fill_offset_time->next=NULL;

	p=*phead;
	int cnt=0;
	while (NULL != p)
	{
		char temp[100]={0};
		cnt++;
		memset(temp,0,100);
		memcpy(temp,inbuf+p->offset,p->len);
		DEBUG_TRACE("\nline=%d,time=%d,%s",cnt,p->time,temp);
		p= p->next;
	}
	DEBUG_TRACE("\n end of raw data\n");
	head = &listhead;
	 head->next = NULL;
	 head->time=-1000;
	p =	(*phead);
	DEBUG_TRACE("\nbefore sort\n");
	while(NULL != p)
	{
		pre = head;
		q = head;

		while(NULL != q)
		{
			if (p->time > q->time)
			{
				pre = q;
				q = q->next;
			}
			else
			{
				break;
			}
		}
		pre->next = p;
		pre = p->next;
		p->next = q;
		p = pre;
	}
	*phead = head->next;
	p=*phead;
	DEBUG_TRACE("\nafter sort\n");

	DEBUG_TRACE("lrc_cnt = %d", lrc_cnt);
	*outbuf = (char *)malloc((lrc_cnt+10)*sizeof(char));
	assert(NULL != *outbuf);

	lrc_cnt = 0;
	int line=1;

	while (NULL != p)
	{
		int i = 0,linelen,offset=p->offset;
		char temp[100]={0};
		gboolean flag= FALSE;

		memcpy(temp,inbuf+p->offset,p->len);
		memcpy(*outbuf+lrc_cnt,inbuf+p->offset,p->len);
		p->offset = lrc_cnt;
		p->line = line;
		lrc_cnt += p->len;
		linelen = p->len;
		for (i=0;i<linelen;i++)
		{
			if (0x0d == inbuf[offset+i] && 0x0a == inbuf[offset+i+1])
			{
				line++;
				if (!flag)
				{
					flag =TRUE;
					p->len = i+2;
				}
				 DEBUG_TRACE("****1*******linelen=%d,p->len=%d*********",linelen,p->len);
				i++;
			}
			else if (0x0a == inbuf[offset+i] || 0x0d == inbuf[offset+i])
			{
				line++;
				if (!flag)
				{
					flag = TRUE;
					p->len = i+1;
				}
				 DEBUG_TRACE("*****2********linelen=%d,p->len=%d**************",linelen,p->len);
			}
		}
		if (!flag)
		{
			line++;
			memcpy(*outbuf+lrc_cnt,"\r\n",2);
			lrc_cnt += 2;
			p->len+=1;
		}

		DEBUG_TRACE("\nline begin=============time=%d,line=%d,len=%d,%s================line end",p->time,p->line,p->len,temp);
		p= p->next;
	}
	*(*outbuf+lrc_cnt)='\0';

}

gboolean ScrollLyric(gpointer data)
{
	gint time,x,y;
	GtkAllocation allocation;
	int i=0;
	Time_List *temp =NULL;
	GtkRequisition requisition;
	GtkTextIter start,end;

	
	gtk_window_get_size(GTK_WINDOW(window),&x,&y);
	gtk_widget_size_request(textview,&requisition);

	if (auto_resize && !user_resize)
	{
		//DEBUG_TRACE("check width\n");
		if (requisition.width != x)
		{
			gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
			gtk_window_resize(GTK_WINDOW(window),requisition.width,y);

			x= requisition.width;
		}
		/*else if (x == requisition.width)
		{
			new_song = FALSE;
		}
		cur_win_width = requisition.width;*/
	}
	/*else if (x != cur_win_width)
	{
		if (requisition.width < x)
		{
			allocation.x = (x-requisition.width)/2;
			allocation.y = 0;
			allocation.width = requisition.width;
			allocation.height = y;//requisition.height;

			gtk_widget_size_allocate(GTK_WIDGET(textview),&allocation);
			gtk_layout_move(GTK_LAYOUT(layout),textview, allocation.x, lrcpos_top);
			lrcpos_left = allocation.x;
		}
		cur_win_width = x;
	}*/

	time =  aud_drct_get_time();
	cur_time_list = NULL;
	temp = lrc_time_list;
	if (NULL != pre_time && time >= pre_time->time)
	{
		temp = pre_time;
		cur_time_list = temp;
	}
	while(NULL != temp)
	{
		if (temp->time <= time)
		{
			cur_time_list = temp;
			temp = temp->next;
		}
		else
		{
			break;
		}
	}


	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

	if (NULL != pre_time && cur_time_list != pre_time && pre_time->len > 2)
	{
		DEBUG_TRACE("offset=%d,len=%d,line=%d",pre_time->offset,pre_time->len,pre_time->line);
		gtk_text_buffer_get_iter_at_line (buffer,&start,pre_time->line-1);
		gtk_text_buffer_get_iter_at_line_index (buffer,&end,pre_time->line-1,pre_time->len-1);
		gtk_text_buffer_remove_tag_by_name (buffer,"forecolors",&start,&end);
	}


	if (NULL == cur_time_list)
	{
		DEBUG_TRACE("\n NULL time\n");
		gtk_layout_move(GTK_LAYOUT(layout),textview, lrcpos_left, y/2);
	}
	else if (NULL != cur_time_list && (cur_time_list != pre_time || y != cur_win_height))
	{
		gint top,i;
		static gint height=0;

		gtk_text_buffer_get_iter_at_line (buffer,&start,cur_time_list->line-1);

		
		if (cur_time_list->len > 2)
		{
			gtk_text_buffer_get_iter_at_line_index (buffer,&end,cur_time_list->line-1,cur_time_list->len-1);
			DEBUG_TRACE("\noffset=%d,len=%d,line=%d-------",cur_time_list->offset,cur_time_list->len,cur_time_list->line);
			//		gtk_text_buffer_get_iter_at_offset(buffer,&end ,cur_time_list->offset+cur_time_list->len);
			gtk_text_buffer_apply_tag_by_name (buffer,"forecolors",&start,&end);
		
			gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(textview),&start, &top, &height);
		}

		lrcpos_left = x <= requisition.width ? 0 : (x-requisition.width)/2;
		lrcpos_top =  y/2-height*(cur_time_list->line-0.5);
		gtk_layout_move(GTK_LAYOUT(layout),textview, lrcpos_left, lrcpos_top);

		DEBUG_TRACE("\nx=%d,width=%d,y=%d,height=%d,line=%d",x,requisition.width,y,height,cur_time_list->line);
		DEBUG_TRACE("pos=%f",y/2 - height*(cur_time_list->line - 0.5));
	}

	pre_time = cur_time_list;
	cur_win_height = y;

	gtk_window_set_resizable(GTK_WINDOW(window),TRUE);

	return 1;
}

 void mylyric_play_lrc(gpointer unused, gpointer unused2)
{
	int pos;
	char *current_file;
	char *file_name, *temp;
	FILE *fp = NULL;
	char newname[255] = {0};
	char lrcname[255] ={0};
	int i=0;
	size_t file_len;
	GtkTextMark *mark;
	GtkTextIter iter;
	const gchar *text;
	char *mylrc;
	char *lrc_after_parse;
	gint x,y;
	GtkRequisition requisition;


    int playlist = aud_playlist_get_playing ();
    pos = aud_playlist_get_position (playlist);

    current_file = aud_playlist_entry_get_filename (playlist, pos);

        DEBUG_TRACE("\n============================play begin=====================n");
	closetimer(NULL,NULL);


	file_name = current_file + strlen(current_file) - 1;
	while (*file_name-- != '/');
	file_name= file_name +2;
		
	while ('\0' != *file_name)
	{
		if ('%' != *file_name)
		{
			newname[i++] = *file_name++;
		}
		else
		{
			file_name++;
			newname[i]= *file_name >= 'A' ? 10+*file_name-'A' : *file_name -'0';
			newname[i] <<= 4;
			file_name++;
			newname[i] |= *file_name >= 'A' ? 10+*file_name-'A' : *file_name-'0';
			file_name++;
			i++;
		}
	}
	gtk_window_set_title (GTK_WINDOW (window), newname);

	temp = newname + strlen(newname)-1;
	while(*temp-- != '.');
	setbuf(stdout,NULL);
	*(temp+1) = '\0';
	temp = file_name;

//	file_name = (char *)malloc(strlen(newname) + 17);
//	memset(file_name, 0, strlen(newname)+17);
	strcpy(lrcname, lyric_position);
	if (lrcname[strlen(lrcname)-1] != '/')
	{
		strcat(lrcname,"/");
	}
	
	strcat(newname,".lrc");
	strcat(lrcname, newname);

	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	file_len =0;
	temp = 	NULL;
	DEBUG_TRACE("\n========%s==================",lrcname);
	setbuf(stdout,NULL);
	
	fp =fopen(lrcname,"r");
	
	if (NULL == fp)
	{
		printf("error open lrc file=%s\n",lrcname);
		//if (find_lrc(lyric_position, lrcname,newname))
		{
		   fp =fopen(newname,"r");
		}
	}
	
	strcpy(newname, lrcname);
	
	if (NULL == fp)
	{
		printf("error open lrc file=%s",newname);
		if (find_lrc(lyric_position,newname, lrcname))
		{
		    fp =fopen(lrcname,"r");

		   	memset(lrcname, 0, sizeof(lrcname));
		    //sprintf(lrcname,"mv *.lrc %s/",lyric_position);
		    //system(lrcname);
		}
	}
	
	if (NULL != fp)
	{
		fseek(fp,0,SEEK_END);
		file_len = ftell(fp);
//		temp = (char *)malloc(file_len +10);
		
		memset(lrc_content, 0, 1000);
	//	if (NULL != temp)
		{
			*(lrc_content+file_len) = '\0';
			fseek(fp,0,SEEK_SET);
			fread(lrc_content, 1,file_len, fp);
//			DEBUG_TRACE("\n%s",lrc_content);
			int i=0;
//			while(i<50)
			{
//				DEBUG_TRACE("%d   ",((unsigned char)lrc_content[i]));i++;
			}
//			setbuf(stdout,NULL);
			fclose(fp);
		}
	}
	
	setbuf(stdout,NULL);

	mylrc = lrc_content;

	size_t utf_len =4000;

	char *plrc2 = lrc2;

	convert("GB18030","UTF-8",mylrc,file_len,plrc2,utf_len);

	i=0;

	DEBUG_TRACE("\n before parse lrc");
	mylyric_free_time_list();

	pre_time = NULL;
	lrcpos_left = 0;
	lrcpos_top = 0;
	user_resize = FALSE;

	parse_lrc(&lrc_time_list,lrc2,&lrc_after_parse);
	cur_time_list = lrc_time_list;
	DEBUG_TRACE("after lrc parse");
	if (NULL == lrc_after_parse)
	{
		mylyric_set_one_line_lrc_content("未找到歌词!\r\n");
	}
	else
	{
		gtk_text_buffer_set_text (buffer, lrc_after_parse, -1);

		scroll_lrc_timer_id = gtk_timeout_add(10, (GtkFunction)ScrollLyric,0);
	}
}

static void mylyric_playback_begin(gpointer unused, gpointer unused2)
{
	    int pos;
		char *current_file;
		char *file_name, *temp;
		FILE *fp = NULL;
		char newname[255] = {0};
		char lrcname[255] ={0};
		int i=0;
		int file_len;
		GtkTextMark *mark;
		GtkTextIter iter;
		const gchar *text;
		char *mylrc;
		char *lrc_after_parse;
		GdkColor color;
		gint x,y;
		GtkRequisition requisition;
		PangoFontDescription *font;

	    int playlist = aud_playlist_get_playing ();
	    pos = aud_playlist_get_position (playlist);

	    current_file = aud_playlist_entry_get_filename (playlist, pos);

	        DEBUG_TRACE("\n============================play begin=====================n");
		closetimer(NULL,NULL);


		file_name = current_file + strlen(current_file) - 1;
		while (*file_name-- != '/');
		file_name= file_name +2;

		while ('\0' != *file_name)
		{
			if ('%' != *file_name)
			{
				newname[i++] = *file_name++;
			}
			else
			{
				file_name++;
				newname[i]= *file_name >= 'A' ? 10+*file_name-'A' : *file_name -'0';
				newname[i] <<= 4;
				file_name++;
				newname[i] |= *file_name >= 'A' ? 10+*file_name-'A' : *file_name-'0';
				file_name++;
				i++;
			}
		}
		gtk_window_set_title (GTK_WINDOW (window), newname);

		temp = newname + strlen(newname)-1;
		while(*temp-- != '.');
		setbuf(stdout,NULL);
		*(temp+1) = '\0';
		temp = file_name;

	//	file_name = (char *)malloc(strlen(newname) + 17);
	//	memset(file_name, 0, strlen(newname)+17);
		strcpy(lrcname, lyric_position);
		if (lrcname[strlen(lrcname)-1] != '/')
		{
			strcat(lrcname,"/");
		}

		strcat(newname,".lrc");
		strcat(lrcname, newname);


		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
		file_len =0;
		temp = 	NULL;
		DEBUG_TRACE("\n========%s==================",lrcname);
		setbuf(stdout,NULL);

		fp =fopen(lrcname,"r");

		if (NULL == fp)
		{
			printf("error open lrc file=%s\n",lrcname);
			//if (find_lrc(lyric_position, lrcname,newname))
			{
			   fp =fopen(newname,"r");
			}
		}

		//strcpy(newname, lrcname);
		if (NULL == fp)
		{
		    //auto download lrc
		    char *argv[4];
		    char artist[100]={0},title[255]={0};
		    temp = lrcname + strlen(lrcname) - 1;

		    while ('/' != *temp && '-' != *temp)
		        temp--;

		    if ('-' == *temp)
		    {
		        strcpy(title, temp+1);
		        *temp = '\0';

		        while ('/' != *temp)
		        temp--;

		        strcpy(artist, temp+1);

		        printf("\ntitle=%s,artist=%s\n",title,artist);
		        temp = title + strlen(title) - 4;
		        *temp = '\0'; //remove ".lrc"

		        remove_str_blank(title);
		        remove_str_blank(artist);
		          printf("\ntitle=%s,artist=%s\n",title,artist);
		    }
		    else
		    {
		        strcpy(title, temp+1);
		        temp = title + strlen(title) - 4;
		        *temp = '\0'; //remove ".lrc"

		        remove_str_blank(title);
		    }
		    argv[0] = "download_lrc";
		    argv[1] = artist  ;
		    argv[2] = title;
		    argv[3] = newname;
		    download_lrc(2, (char **)argv);
		    return;
		}
		else
		{
			mylyric_play_lrc(NULL,NULL);
		}

}

static void mylyric_playback_end(gpointer unused, gpointer unused2)
{
	int pos;
	char *current_file;
    int playlist = aud_playlist_get_playing ();
    pos = aud_playlist_get_position (playlist);

    //current_file = aud_playlist_entry_get_filename (playlist, pos);

    DEBUG_TRACE("\n ============================play end=====================n");

	//g_free(current_file);
}

static void mylyric_playlist_eof(gpointer unused, gpointer unused2)
{
	gchar *current_file;
    int playlist = aud_playlist_get_playing ();
    int pos = aud_playlist_get_position (playlist);

    current_file = aud_playlist_entry_get_filename (playlist, pos);

	//g_free(current_file);
}

typedef struct {
    gchar * lyric_position;
    gchar * lyric_font;
    gchar * lyric_forecolor;
    gchar * lyric_backcolor;
    gchar * lyric_focuscolor;
} SongChangeConfig;

static SongChangeConfig config = {NULL};

static void configure_ok_cb()
{
	if (lyric_position != NULL)
		g_free(lyric_position);

	if (lyric_font != NULL)
		g_free(lyric_font);

	if (lyric_forecolor != NULL)
		g_free(lyric_forecolor);

	if (lyric_backcolor != NULL)
		g_free(lyric_backcolor);

	if (lyric_focuscolor != NULL)
		g_free(lyric_focuscolor);

    lyric_position   = g_strdup(config.lyric_position);
    lyric_font       = g_strdup(config.lyric_font);
    lyric_forecolor  = g_strdup(config.lyric_forecolor);
    lyric_backcolor  = g_strdup(config.lyric_backcolor);
    lyric_focuscolor = g_strdup(config.lyric_focuscolor);

    DEBUG_TRACE("lyric_position:%s\n",lyric_position);
    DEBUG_TRACE("lyric_font:%s\n",lyric_font);
    DEBUG_TRACE("lyric_forecolor:%s\n",lyric_forecolor);
    DEBUG_TRACE("lyric_backcolor:%s\n",lyric_backcolor);
    DEBUG_TRACE("lyric_focuscolor:%s\n",lyric_focuscolor);
}

static void forecolor_set_cb (GtkWidget * chooser)
{
	GdkColor color;
    gtk_color_button_get_color ((GtkColorButton *) chooser, & color);

    if (NULL != lyric_forecolor)
    {
    	g_free(lyric_forecolor);
    }
    lyric_forecolor = gdk_color_to_string(&color);
}

static void * mylyric_get_forecolor_chooser (void)
{
	GdkColor color;

	gdk_color_parse(config.lyric_forecolor,&color);

    GtkWidget * chooser = gtk_color_button_new_with_color (& color);

    g_signal_connect (chooser, "color-set", (GCallback) forecolor_set_cb, NULL);

    return chooser;
}

static void focuscolor_set_cb (GtkWidget * chooser)
{
	GdkColor color;
    gtk_color_button_get_color ((GtkColorButton *) chooser, & color);

    if (NULL != lyric_focuscolor)
    {
    	g_free(lyric_focuscolor);
    }
    lyric_focuscolor = gdk_color_to_string(&color);
}

static void * mylyric_get_focuscolor_chooser (void)
{
	GdkColor color;

	gdk_color_parse(config.lyric_focuscolor,&color);

    GtkWidget * chooser = gtk_color_button_new_with_color (& color);

    g_signal_connect (chooser, "color-set", (GCallback) focuscolor_set_cb, NULL);

    return chooser;
}

static void backcolor_set_cb (GtkWidget * chooser)
{
	GdkColor color;
    gtk_color_button_get_color ((GtkColorButton *) chooser, & color);

    if (NULL != lyric_backcolor)
    {
    	g_free(lyric_backcolor);
    }
    lyric_backcolor = gdk_color_to_string(&color);
}

static void * mylyric_get_backcolor_chooser (void)
{
	GdkColor color;

	gdk_color_parse(config.lyric_backcolor,&color);

    GtkWidget * chooser = gtk_color_button_new_with_color (& color);

    g_signal_connect (chooser, "color-set", (GCallback) backcolor_set_cb, NULL);

    return chooser;
}

static const PreferencesWidget elements[] = {
    {WIDGET_LABEL, N_("歌词存放位置:"),
        .data = {.label = {.single_line = TRUE}}},
    {WIDGET_ENTRY, N_(" "), .cfg_type = VALUE_STRING,
        .cfg = & config.lyric_position, .callback = configure_ok_cb},
    {WIDGET_SEPARATOR, .data = {.separator = {TRUE}}},

    {WIDGET_LABEL, N_("字体:"),
        .data = {.label = {.single_line = TRUE}}},
    {WIDGET_FONT_BTN, N_(" "), .cfg_type = VALUE_STRING,
        .cfg = & config.lyric_font, .callback = configure_ok_cb},
    {WIDGET_SEPARATOR, .data = {.separator = {TRUE}}},

    {WIDGET_LABEL, N_("正常歌词颜色:"), .data = {.label = {.single_line = TRUE}}},
    {WIDGET_CUSTOM, .data = {.populate = mylyric_get_forecolor_chooser}},
    {WIDGET_SEPARATOR, .data = {.separator = {TRUE}}},

    {WIDGET_LABEL, N_("播放歌词颜色:"), .data = {.label = {.single_line = TRUE}}},
    {WIDGET_CUSTOM, .data = {.populate = mylyric_get_focuscolor_chooser}},
    {WIDGET_SEPARATOR, .data = {.separator = {TRUE}}},

    {WIDGET_LABEL, N_("歌词背景颜色:"), .data = {.label = {.single_line = TRUE}}},
    {WIDGET_CUSTOM, .data = {.populate = mylyric_get_backcolor_chooser}},
};

static const PreferencesWidget settings[] = {
    {WIDGET_BOX, N_("Commands"), .data = {.box = {elements, G_N_ELEMENTS (elements), .frame = TRUE}}},
    };

static void configure_init(void)
{
    read_config();

    config.lyric_position = g_strdup(lyric_position);
    config.lyric_font = g_strdup(lyric_font);
    config.lyric_forecolor = g_strdup(lyric_forecolor);
    config.lyric_backcolor = g_strdup(lyric_backcolor);
    config.lyric_focuscolor = g_strdup(lyric_focuscolor);
}

static void configure_cleanup(void)
{
    g_free(config.lyric_position);
	g_free(config.lyric_font);
	g_free(config.lyric_forecolor);
	g_free(config.lyric_backcolor);
	g_free(config.lyric_focuscolor);

    config.lyric_position = NULL;
    config.lyric_font = NULL;
    config.lyric_forecolor = NULL;
    config.lyric_backcolor = NULL;
    config.lyric_focuscolor = NULL;

    DEBUG_TRACE("\n===============configure_cleanup=================\n");
    save_config();

	applet_save_and_close(NULL, NULL);
}

static const PluginPreferences preferences = {
    .widgets = settings,
    .n_widgets = G_N_ELEMENTS (settings),
    .init = configure_init,
    .cleanup = configure_cleanup,
};
