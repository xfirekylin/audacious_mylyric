src =  mylyric.c formatter.h formatter.c download_lrc.c downloadbyhttp.c utility.c utility.h config.h
dest =  mylyric.so
flag = `pkg-config --cflags --libs gtk+-2.0 audacious`
cflag = -fpic -shared -DAUDACIOUS -g -Wl,--no-as-needed

all:
	gcc ${cflag} ${flag}  ${src} -o ${dest} 

install:
	sh install
.PHONY: install
