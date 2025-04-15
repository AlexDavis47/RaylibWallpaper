CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lgdi32 -luser32

all: desktop_access desktop_window_test

desktop_access: desktop_access.c
	$(CC) $(CFLAGS) -o desktop_access.exe desktop_access.c $(LDFLAGS)

desktop_window_test: desktop_window_test.c
	$(CC) $(CFLAGS) -o desktop_window_test.exe desktop_window_test.c $(LDFLAGS) -mwindows

clean:
	rm -f desktop_access.exe desktop_window_test.exe 