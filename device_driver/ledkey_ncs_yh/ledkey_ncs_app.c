#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include "ioctl_test.h"

#define DEVICE_FILENAME "/dev/ledkey_ncs"

int main(int argc, char *argv[])
{
	int dev;
	char key_no;
	char led_no;
	char timer_val;
	int ret;
	int cnt = 0;
	int loopFlag = 1;
	struct pollfd Events[2];	// 입출력 다중화
	char inputString[80];
	keyled_data info;

	if(argc != 3)
	{
        printf("Usage : %s [led_val(0x0~0xf)] [timer_val(1/100)]\n",argv[0]);
		return 1;
	}
	led_no = (char)strtoul(argv[1],NULL,16);	// argv[1] : led_value
												// strtoul : 16진수 변환
												// char 형변환
	if(!((0 <= led_no) && (led_no <= 15)))
	{
		printf("Usage : %s [led_data(0x0~0xf)]\n",argv[0]);
		return 2;
	}
	printf("Author:염훈\n");
    timer_val = atoi(argv[2]);	// 3번째 arg : time value
	info.timer_val = timer_val;

//	dev = open(DEVICE_FILENAME, O_RDWR | O_NONBLOCK);
	dev = open(DEVICE_FILENAME, O_RDWR );	// device file open
	if(dev < 0)
	{
		perror("open");
		return 2;
	}

	ioctl(dev,TIMER_VALUE,&info);	// arg로 받은 값음 info에 넣음
    write(dev,&led_no,sizeof(led_no));
    ioctl(dev,TIMER_START);

	memset( Events, 0, sizeof(Events));

	Events[0].fd = dev;	// poll 처리.
	Events[0].events = POLLIN;	// 읽기에 대한 event 처리. poll init
	Events[1].fd = fileno(stdin);
	Events[1].events = POLLIN;	// 일기에 대한 event 처리

	while(loopFlag)
	{

		ret = poll(Events, 2, 1000);
		if(ret==0)	// Event가 리턴할 때 revent 변수에 값을 담아오는데 이게 0일 경우
		{
//  		printf("poll time out : %d\n",cnt++);
			continue;
		}
		if(Events[0].revents & POLLIN)  //dev : keyled. // timeout되기 전에 event가 발생했을 때
		{
    		read(dev,&key_no,sizeof(key_no));
			printf("key_no : %d\n",key_no);
			switch(key_no) 
			{
				case 1:
            		printf("TIMER STOP! \n");
            		ioctl(dev,TIMER_STOP);
					break;
				case 2:
            		ioctl(dev,TIMER_STOP);	// timer value 변경
            		printf("Enter timer value! \n");	// 엔터 치면 poll에 의해Revent로 가서 값 입력 받음
					break;
				case 3:
            		ioctl(dev,TIMER_STOP);	// led 변경
            		printf("Enter led value! \n");
					break;
				case 4:
            		printf("TIMER START! \n");
            		ioctl(dev,TIMER_START);
					break;
				case 8:
            		printf("APP CLOSE ! \n");
            		ioctl(dev,TIMER_STOP);
					loopFlag = 0;
				break;

			}
		}
		else if(Events[1].revents & POLLIN) //keyboard
		{	// 엔터 치면 여기로 옴
    		fflush(stdin);	// 지우고
			fgets(inputString,sizeof(inputString),stdin); // 입력될 때까지 기다림.
			if((inputString[0] == 'q') || (inputString[0] == 'Q'))
				break;
			inputString[strlen(inputString)-1] = '\0';	// fgets는 new line도 들어오기 때문에 NULL을 넣어줌
           
			if(key_no == 2) //timer value
			{
				timer_val = atoi(inputString);
				info.timer_val = timer_val;
				ioctl(dev,TIMER_VALUE,&info);	// timer_val 값 넘겨줌
            	ioctl(dev,TIMER_START);
				
			}
			else if(key_no == 3) //led value
			{
				led_no = (char)strtoul(inputString,NULL,16);
    			write(dev,&led_no,sizeof(led_no));	// write 와 ioctl 사용 이유 차이 확인할 것
            	ioctl(dev,TIMER_START);
			}
			key_no = 0;
		}
	}
	close(dev);
	return 0;
}
