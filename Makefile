INCDIR =
LIBDIR =

BIN = flxd
OBJS = main.o flx.o
LIBS = -lubox -luci -lmosquitto
CSTD = -std=gnu99
WARN = -Wall -pedantic

CFLAGS += -O2 $(CSTD) $(WARN) $(INCDIR)
LDFLAGS += $(CSTD) $(LIBDIR)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)
