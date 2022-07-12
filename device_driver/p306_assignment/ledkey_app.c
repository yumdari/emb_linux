#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "ioctl_test.h"
#define DEVICE_FILENAME "/dev/ioctldev"
int main()
{
	ioctl_test_info info={0,{0}};	// 128 바이트를 모두 0으로 초기화
	int dev;
	int state;
	int cnt;
	int ret;

	dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY ); // 장치파일 open
	if( dev >= 0 )
	{
		ioctl(dev, IOCTLTEST_KEYLEDINIT );
		printf("IOCTLTEST_KEYLEDINIT : %x\n",IOCTLTEST_KEYLEDINIT);
		printf( "wait... input1\n" );
		ioctl(dev, IOCTLTEST_LEDON );
		while(1)
		{
			state = ioctl(dev, IOCTLTEST_GETSTATE );	//key값return. return 값이 key값임
			if(state == 1) break;
		}
		ioctl(dev, IOCTLTEST_LEDOFF );
		sleep(1);
		printf( "wait... input2\n" );
		while(1)
		{
			info.size = 0;
			ioctl(dev, IOCTLTEST_READ, &info );	//IOCTLTEST_READ 키 값을 받아서 info에 넣음
			if( info.size > 0 )
			{
				printf("key : %d\n",info.buff[0]);

				if(info.buff[0] == 1) break;	// 스위치 값이 1이면 while 문 벗어남
			}
		}
		info.size = 1;
		info.buff[0] = 0x0F;
		for( cnt=0; cnt<10; cnt++ )
		{
			ioctl(dev, IOCTLTEST_WRITE, &info );
			info.buff[0] = ~info.buff[0];	// 불꺼짐
			usleep( 500000 );	// 0.5초 단위
		}
		printf( "wait... input3\n" );
		cnt = 0;
		state = 0xFF;
		while(1)
		{
			info.size = 1;
			info.buff[0] = state;
			ret = ioctl(dev, IOCTLTEST_WRITE_READ, &info );	// W, R 둘 다 가능
			if(ret < 0)
			{
				printf("ret : %d\n",ret);
				perror("ioctl()");
			}
			if( info.size > 0 )
			{
				printf("key : %d\n",info.buff[0]);	// key값 출력
				if(info.buff[0] == 1) break;
			}
			state = ~state;	// 0x00
			usleep( 100000 );	// 0.1초 지연
		}
		ioctl(dev, IOCTLTEST_LEDOFF );	// 1번 스위치가 눌리면 종료
		ioctl(dev, IOCTLTEST_KEYLEDFREE );
  		close(dev);
	}
	else
	{
		perror("open");
		return 1;
	}
	return 0;
}
