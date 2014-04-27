src =  mylyric.c baidu_lrc.c utility.c utility.h
dest =  mylyric.so
flag = `pkg-config --cflags --libs gtk+-2.0 audacious libxml-2.0`
cflag = -fpic -shared -DAUDACIOUS -g -Wl,--no-as-needed

all:
	gcc ${cflag} ${flag}  ${src} -o ${dest} 

install:
	sh install
.PHONY: install
