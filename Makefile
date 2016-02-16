INCDIR =
LIBDIR =

EXE = flxd
OBJS = main.o
LIBS =
CSTD = -std=gnu11
WARN = -Wall -pedantic

CFLAGS += -O2 $(CSTD) $(WARN) $(INCDIR)
LDFLAGS += $(CSTD) $(LIBDIR)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(EXE)
