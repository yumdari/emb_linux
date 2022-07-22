#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "ioctl_test.h"

#define DEVICE_FILENAME  "/dev/ledkey_block"

int main()
{
    int dev;
    char buff = 15;
    int ret;
//    int key_old = 0;	// blocking I/O에서 안씀
	int cnt = 0;
	ioctl_test_info info = {0};

//    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
//    dev = open( DEVICE_FILENAME, O_RDWR|O_NONBLOCK);
      dev = open( DEVICE_FILENAME, O_RDWR);   //block mode
	if(dev<0)
	{
		perror("open()");
		return 1;
	}
    ret = write(dev,&buff,sizeof(buff));
	if(ret < 0)
		perror("write()");
	buff = 0;
	do {
    	ret = read(dev,&buff,sizeof(buff));              
//  		printf("ret : %d, cnt : %d\n",ret,cnt++);
		if(ret < 0)
		{
  			perror("read()");
			return 1;
		}
		if(ret == 0)
			printf("Time out\n");
		ret = ioctl(dev,IOCTLTEST_INTVAL, &info);
  		printf("intval : %lu %.2lf\n",info.intval, info.intval/100.0);
//		if(buff == 0) //sw_no : 0. 0이면 잠들자. blocking I/O 쓰면 0 절대 안나옴. 이전 예제에서나 씀.polling은 0을 계속 return하니까 그걸 처리해주려고 이렇게 씀
//			continue;
//		if(buff != key_old)	// polling에서만 씀. 무조건 잠들어
//		{
			printf("key_no : %d\n",buff);
			write(dev,&buff,sizeof(buff));
			if(buff == 8)
				break;
			key_old = buff;
//		}
	} while(1);
    close(dev);
    return 0;
}
