#ifndef __IOCTL_H__
#define __IOCTL_H__

#define IOCTLTEST_MAGIC 't'	// 임의로 지정한 ASCII 코드 값. 다른걸로 해도 무방
typedef struct
{
	unsigned long size;
	unsigned char buff[128];
} __attribute__((packed)) ioctl_test_info;	// 구조체 정렬 4 + 128 = 132 byte

#define IOCTLTEST_KEYLEDINIT 	_IO(IOCTLTEST_MAGIC, 0)	// _IO : 3번째 매개변수가 필요 없음. 구분 번호 : 0 
#define IOCTLTEST_KEYLEDFREE 	_IO(IOCTLTEST_MAGIC, 1)
#define IOCTLTEST_LEDOFF		_IO(IOCTLTEST_MAGIC, 2)
#define IOCTLTEST_LEDON			_IO(IOCTLTEST_MAGIC, 3)
#define IOCTLTEST_GETSTATE		_IO(IOCTLTEST_MAGIC, 4)	// key값 return
#define IOCTLTEST_READ			_IOR(IOCTLTEST_MAGIC, 5,ioctl_test_info)	// 매개변수 받아옴
#define IOCTLTEST_WRITE			_IOW(IOCTLTEST_MAGIC, 6,ioctl_test_info)
#define IOCTLTEST_WRITE_READ	_IOWR(IOCTLTEST_MAGIC, 7,ioctl_test_info)	// 호출할 때write, return할 때 read
#define IOCTLTEST_MAXNR			8	// MAXNR : 구분번호 + 1
#endif
