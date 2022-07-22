#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>		
#include <asm/uaccess.h>	// copy_from_user, copy_to_user
#include <asm/io.h>
#include <linux/time.h>
#include <linux/timer.h>	// timer
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#define TIME_STEP	timeval

#define   KERNELTIMER_DEV_MAJOR            240
#define   KERNELTIMER_DEV_NAME            "kerneltimer"

#define DEBUG 0
#define DEBUG1 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))
static int timeval = 100;	//f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec
							// 1초를 저장하기 위함
module_param(timeval,int ,0);
static int ledval = 0;
module_param(ledval,int ,0);
typedef struct
{
	struct timer_list timer;
	unsigned long 	  key;
	unsigned long 	  led;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;	// 구조체 정렬 

//static KERNEL_TIMER_MANAGER* ptrmng = NULL;
void kerneltimer_timeover(unsigned long arg);

int key[] = {
    IMX_GPIO_NR(1, 20),
    IMX_GPIO_NR(1, 21),
    IMX_GPIO_NR(4, 8),
    IMX_GPIO_NR(4, 9),
    IMX_GPIO_NR(4, 5),
    IMX_GPIO_NR(7, 13),
    IMX_GPIO_NR(1, 7),
    IMX_GPIO_NR(1, 8),
};

int led[] = {
    IMX_GPIO_NR(1, 16),
    IMX_GPIO_NR(1, 17),
    IMX_GPIO_NR(1, 18),
    IMX_GPIO_NR(1, 19),
};

static int key_init(void)   // GPIO pin (Switch) 초기화
{
    int ret = 0;
    int i;

    for (i = 0; i < ARRAY_SIZE(key); i++) {
        ret = gpio_request(key[i], "gpio key");
        if(ret<0){
            printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
        }
        else
            gpio_direction_input(key[i]);  // GPIO를 입력 모드로 변경
                                           //gpio_direction_input( gp_nr );
    }
    return ret;
}

static void key_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
}

static void key_read(char * key_data)	// 
{
    int i;
    char data=0;
//  char temp;
    for(i=0;i<ARRAY_SIZE(key);i++)
    {
        if(gpio_get_value(key[i]))  // GPIO 핀을 읽음
        {
            data = i+1; // 1 ~ 8 사이의 값을 return
            break;
        }
//      temp = gpio_get_value(key[i]) << i;
//      data |= temp;
    }
#if DEBUG
    printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
    *key_data = data;
    return;
}

static int led_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else {
			gpio_direction_output(led[i], 0);
		}
	}
	return ret;
}
static void led_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);
	}
}

void led_write(char data)
{
    int i;
    for(i = 0; i < ARRAY_SIZE(led); i++){
        gpio_set_value(led[i], ((data >> i) & 0x01));
#if DEBUG
        printk("#### %s, data = %d(%d)\n", __FUNCTION__, data,(data>>i));
#endif
    }
}
void kerneltimer_registertimer(KERNEL_TIMER_MANAGER *pdata, unsigned long timeover)
{
	init_timer( &(pdata->timer) );
	pdata->timer.expires = get_jiffies_64() + timeover;  //10ms *100 = 1sec. jiffies 값이 100번 count 되면 종료. 1초 뒤에 kernel timer의 timeover 함수를 호출
	pdata->timer.data	 = (unsigned long)pdata;	// pdata는 32바이트를 가리키는 포인터였는데, 주소값(unsigned long)만 가져옴
	pdata->timer.function = kerneltimer_timeover;
	add_timer( &(pdata->timer) );	// kernel이 관리하고 있는 timer list에 등록
}
void kerneltimer_timeover(unsigned long arg)
{
	KERNEL_TIMER_MANAGER* pdata = NULL;
	if( arg )
	{
		pdata = ( KERNEL_TIMER_MANAGER *)arg;	// 32 byte를 가리키는 포인터로 변환
		led_write(pdata->led & 0x0f);	// 32bit 중에 하위 4bit 값만 가져와서 제어 (led는 4bit만 있음)

#if DEBUG1
		printk("led : %#04x\n, key : %04x\n",(unsigned int)(pdata->led & 0x0000000f),(unsigned int)(pdata->key));	// 1초마다 led 및 key값 표시
#endif
		pdata->led = ~(pdata->led);	// led 반전 
		kerneltimer_registertimer( pdata, TIME_STEP);
	}
}

static int ledkey_open (struct inode *inode, struct file *filp)
{
	KERNEL_TIMER_MANAGER * ptrmng = NULL;
    int num0 = MAJOR(inode->i_rdev);
    int num1 = MINOR(inode->i_rdev);
    printk( "call open -> major : %d\n", num0 );    // 주번호 출력
    printk( "call open -> minor : %d\n", num1 );    // 부번호 출력

	ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);	// 동적할당
//	filp->private_data = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
	if(ptrmng == NULL) return -ENOMEM;
	memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER));
	ptrmng->led = ledval;
	kerneltimer_registertimer( ptrmng, TIME_STEP);
	filp->private_data = ptrmng;					// file pointer의 private_date에 ptrmng 대입
    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)	// kernel -> user. read 먼저하고 write
{   																					// 디바이스의 f_pos 위치에서 count 바이트만큼을 읽어서 사용자 영역인 buf로 저장해주는 기능
    char kbuf;
    int ret;  

	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER *)filp -> private_data;

	key_read(&kbuf);
	if(kbuf) ptrmng->key = kbuf;
//  put_user(kbuf,buf);
    ret = copy_to_user(buf,&kbuf,count);	// copy_to_user() 함수가 반환형이 있는데, 이거 안 받으면 warning 뜸
    return count;
}


static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
// 사용자 영역인 buf에서 count 바이트 만큼 읽은 후  device의 f_pos 위치로 저장
// struct file *flip   읽기와 쓰기에 전달되는 flip은 device file이 어떤 형식으로열렸는가에 대한 정보를 저장
// loff_t f_ops        f_pos 필드 변수에는 현재의 읽기 / 쓰기 위치를 저장
{
    char kbuf;  // kernel buff
    int ret;
	KERNEL_TIMER_MANAGER * ptrmng = (KERNEL_TIMER_MANAGER *)filp->private_data;
//  get_user(kbuf,buf); // kbuf 변수에 buf의 user memory 값을 대입
    ret = copy_from_user(&kbuf,buf,count);
    // kernel 영역과 user 영역은 서로 접근하지 못하는 메모리 영역이기에 pointer>를 이용하지 못함
    // 데이터를 전달하는 copy_from_user 함수를 이용
    // user 영역의 buf 값을 kernel 영역의 kbuf에 전달

//    led_write(kbuf);
	ptrmng->led = kbuf;
    return count;
//  return -EFAULT;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
	KERNEL_TIMER_MANAGER * ptrmng = (KERNEL_TIMER_MANAGER *)filp->private_data;
    printk( "call release \n" );
	if(timer_pending(&(ptrmng->timer)))	/// 평가할 때 꼭 사용
										// timer가 구동 중인지 체크
		del_timer(&(ptrmng->timer));	// add timer 되어 있다면 해제
	if(ptrmng != NULL)	// 해제가 안되어있다면
	{
		kfree(ptrmng);	// 해제
	}
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,        // 파일 오퍼레이션의 소유자
    .open     = ledkey_open,        // device driver가 처음 열렸을 때 H/W 초기화
    .read     = ledkey_read,        // device의 f_pos에서 count 바이트만큼을 읽>어서 user 영역인 buf에 저장해주는 기능
    .write    = ledkey_write,       // 사용자 영역인 buf에서 count 바이트 만큼 >읽은 후 device의 f_pos에 저장
    .release  = ledkey_release,     // open이 flip->private data에 할당한 데이터의 할당 삭제
                                    // 마지막 close 호출 시 device 종료
};

int kerneltimer_init(void)
{
	int result;
	key_init();
	led_init();	// led 초기화 
	result = register_chrdev( KERNELTIMER_DEV_MAJOR, KERNELTIMER_DEV_NAME, &ledkey_fops); 
	if (result < 0) return result;
	printk("timeval : %d , sec : %d\n",timeval,timeval/HZ);

	return 0;
}
void kerneltimer_exit(void)
{
	unregister_chrdev( KERNELTIMER_DEV_MAJOR, KERNELTIMER_DEV_NAME );
	led_write(0);
	key_exit();
	led_exit();
}
module_init(kerneltimer_init);
module_exit(kerneltimer_exit);
MODULE_LICENSE("Dual BSD/GPL");
