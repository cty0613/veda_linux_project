CC = gcc
CFLAGS = -fPIC -Wall
LDFLAGS = -shared

SUBDIRS = 7seg_fnd buzzer_music cds_led led_pwm
TARGETS = $(patsubst %, lib%.so, $(SUBDIRS))

.PHONY: all clean

all: $(TARGETS)

server:	
	$(CC) -o server server.c -lwiringPi -lpthread

http_server:
	$(CC) -o http_server http_server.c 

client:
	$(CC) -o client client.c -lpthread
# 각 라이브러리 생성 규칙
lib%.so:
	$(CC) $(CFLAGS) -c $(abspath $*/$*.c) -o $*/$*.o
	$(CC) $(LDFLAGS) -o $@ $*/$*.o

clean:
	rm -f */*.o *.so server client http_server