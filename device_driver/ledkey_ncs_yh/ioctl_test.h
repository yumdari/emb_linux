#ifndef __IOCTL_H__
#define __IOCTL_H__

#define IOCTLTEST_MAGIC 't'	// 임의로 지정한 ASCII 코드 값. 다른걸로 해도 무방
typedef struct	// 4 byte 짜리 구조체 변수 선언
{
	unsigned long timer_val;	// ioctl을 이용해서 데이터를 주고 받기 위한 변수
} __attribute__((packed)) keyled_data;	// 구조체 정렬 4 + 128 = 132 byte

#define TIMER_START				_IO(IOCTLTEST_MAGIC, 0)	// _IO : 3번째 매개변수가 필요 없음. 구분 번호 : 0 
#define TIMER_STOP		 	_IO(IOCTLTEST_MAGIC, 1)
#define TIMER_VALUE			_IOW(IOCTLTEST_MAGIC, 2, keyled_data)	// keyled_data라는 구조체 변수를 사용
#define IOCTLTEST_MAXNR			3	// MAXNR : 구분번호 + 1
#endif
