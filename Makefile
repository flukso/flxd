INCDIR =
LIBDIR =

BIN = flxd
OBJS = main.o flx.o config.o
LIBS = -lm -lubox -lubus -luci -lmosquitto
CSTD = -std=gnu99
WARN = -Wall -pedantic

CFLAGS += -O2 $(CSTD) $(WARN) $(INCDIR)
LDFLAGS += $(CSTD) $(LIBDIR)

ifeq ($(WITH_YKW),yes)
    LIBS += -lykw
    CFLAGS += -DWITH_YKW
endif

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)
