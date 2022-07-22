KDIR = /home/ubuntu/udoo_linux_bsp/kernel/3.14.1
INCLUDEDIRS += -I$(KDIR)/arch/arm/include
INCLUDEDIRS += -I$(KDIR)/include/generated
CC = arm-linux-gnueabihf-gcc
CFLAGS += $(INCLUDEDIRS)
CFLAGS += -Wall -O2
TARGET =syscall_app
OBJS = $(TARGET).o
SRCS = $(TARGET).c
$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
clean :
	rm -rf $(OBJS) $(TARGET) 
