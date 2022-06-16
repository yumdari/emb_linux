CC:= gcc
#CC: arm-linux-gnueabihf-gcc

OBJECT = input.o output.o hi.o
TARGET = hi
$(TARGET): $(OBJECT)
	$(CC) -o$@ $(OBJECT)
#	cp $@ /srv/nfs_server

%.o: %.c
	$(CC) -c -o $@ $<
clean:
	rm -f *o$(TARGET)
