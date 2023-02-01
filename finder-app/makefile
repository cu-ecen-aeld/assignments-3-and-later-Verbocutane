CC=$(CROSS_COMPILE)gcc
CFLAGS=-I.
ODIR=obj

dir_guard=@mkdir -p $(@D)

_OBJ = writer.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c
	$(dir_guard)
	$(CC) -c -o $@ $< $(CFLAGS)

writer: $(OBJ)
	$(dir_guard)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o writer
