APPS = g-rays2-cmd
CFLAGS = -O2 -Wall -Werror
CFLAGS +=  `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

all: $(APPS)

clean:
	rm -f $(APPS) *.o
