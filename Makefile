# 공유 라이브러리 생성용 Makefile

CC = gcc
CFLAGS = -fPIC -Wall
LDFLAGS = -shared

SUBDIRS = 7seg_fnd buzzer_music cds_led led_pwm server
TARGETS = $(patsubst %, lib%.so, $(SUBDIRS))

.PHONY: all clean

all: $(TARGETS)

server:	
	$(CC) -o server server.c -lwiringPi -pthread

# 각 라이브러리 생성 규칙
lib%.so:
	$(CC) $(CFLAGS) -c $(abspath $*/$*.c) -o $*/$*.o
	$(CC) $(LDFLAGS) -o $@ $*/$*.o

clean:
	rm -f */*.o *.so
