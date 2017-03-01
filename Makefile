INCDIR =
LIBDIR =

BIN = flxd
OBJS = main.o flx.o config.o shift.o binary.o
LIBS = -lm -lubox -lubus -luci -lmosquitto -ljson-c
CSTD = -std=gnu99
WARN = -Wall -pedantic

CFLAGS += -O2 -ggdb3 $(CSTD) $(WARN) $(INCDIR)
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
