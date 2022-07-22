#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME  "/dev/block_led"

int main()
{
    int dev;
    char buff = 15;
    int ret;
    int key_old = 0;

    //dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY ); //원문
	dev = open( DEVICE_FILENAME, O_RDWR|O_NONBLOCK );
	//dev = open( DEVICE_FILENAME, O_RDWR);		      //block mode(뒤에 인자값 생략)
	//가상파일시스템을 거쳐서 open한다
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
		//printf("ret : %d", ret);
  		if(ret < 0) {
  			perror("read()");
			return 1;
		}
		if(buff == 0) //sw_no :0
			continue;
		if(buff != key_old)
		{	
			//if(buff)
			//{
				printf("key_no : %d\n",buff);
				write(dev,&buff,sizeof(buff));
			//}
			if(buff == 8)
				break;
			key_old = buff;
		}
	} while(1);
    close(dev);
    return 0;
}
