#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#define DEVICE_FILENAME "/dev/ledkey_poll"

int main(int argc, char *argv[])
{
	int dev;
	char buff;
	int ret;
	int num = 1;
	struct pollfd Events[2];
	char keyStr[80];

    if(argc != 2)
    {
        printf("Usage : %s [led_data(0x0~0xf)]\n",argv[0]);
        return 1;
    }
    buff = (char)strtoul(argv[1],NULL,16);
//    if(!((0 <= buff) && (buff <= 15)))	// 0 ~ 15가 아니면 종료
//    if((buff < 0) || (buff < 15))	// 0 ~ 15가 아니면 종료
    if((buff >= 0) && (buff <= 15))	// 0 ~ 15가 아니면 종료
    {
        printf("Usage : %s [led_data(0x0~0xf)]\n",argv[0]);
        return 2;
    }

//  dev = open(DEVICE_FILENAME, O_RDWR | O_NONBLOCK);
  	dev = open(DEVICE_FILENAME, O_RDWR );
	if(dev < 0)
	{
		perror("open");
		return 2;
	}
	write(dev,&buff,sizeof(buff));

	fflush(stdin);	// stdin (표줄 입출력)의 buffer를 비움
	memset( Events, 0, sizeof(Events));
	Events[0].fd = dev;
	Events[0].events = POLLIN;
  	Events[1].fd = fileno(stdin);	// stdin은 file pointer.
									// fileno는 file pointer를 정수로 변환
  	Events[1].events = POLLIN;
	while(1)
	{
		ret = poll(Events, 2, 2000);
		if(ret<0)
		{
			perror("poll");
			exit(1);
		}
		else if(ret==0)
		{
  			printf("poll time out : %d Sec\n",2*num++);
			continue;
		}
		if(Events[0].revents & POLLIN) //keyled
		{
			ret = read(dev,&buff,sizeof(buff));
			printf("key_no : %d\n",buff);
			write(dev,&buff,sizeof(buff));
			if(buff == 8)
				break;
		}
		else if(Events[1].revents & POLLIN)  //stdin
		{
			fgets(keyStr,sizeof(keyStr),stdin);
			if(keyStr[0] == 'q')	// q가 눌리면 break
				break;
			keyStr[strlen(keyStr)-1] = '\0';	// \n을 업앰
			buff = (char)atoi(keyStr);
			printf("STDIN : %s\n",keyStr);
			write(dev,&buff,sizeof(buff));
		}
	}
	close(dev);
	return 0;
}
