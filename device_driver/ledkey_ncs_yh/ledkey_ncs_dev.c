#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>

//#define TIME_STEP   timeval

#include "ioctl_test.h"

#define   LEDKEY_DEV_NAME            "ledkey_ncs"
#define   LEDKEY_DEV_MAJOR            240      
#define DEBUG 0
#define DEBUG1 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

static int timeval = 100;   //f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec:ㅈ

module_param(timeval,int ,0);
static int ledval = 0;
module_param(ledval,int ,0);
typedef struct
{
    struct timer_list timer;
    unsigned long     led;
	int time_val;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;


void kerneltimer_timeover(unsigned long arg);

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);
static int sw_irq[8] = {0};
static long sw_no = 0;
static int led[] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
};

static int key[] = {
	IMX_GPIO_NR(1, 20),
	IMX_GPIO_NR(1, 21),
	IMX_GPIO_NR(4, 8),
	IMX_GPIO_NR(4, 9),
  	IMX_GPIO_NR(4, 5),
  	IMX_GPIO_NR(7, 13),
  	IMX_GPIO_NR(1, 7),
 	IMX_GPIO_NR(1, 8),
};
static int led_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else
			gpio_direction_output(led[i], 0);  //0:led off
	}
	return ret;
}
#if 0
static int key_init(void)
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = gpio_request(key[i], "gpio key");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", key[i], ret);
		} 
		else
			gpio_direction_input(key[i]);  
	}
	return ret;
}
#endif
irqreturn_t sw_isr(int irq, void *unuse)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		if(irq == sw_irq[i]) {
			sw_no = i+1;
			break;
		}
	}
	printk("IRQ : %d, %ld\n",irq, sw_no);
  	wake_up_interruptible(&WaitQueue_Read);	// read에 의해 잠드는 경우는 없음. 현재 소스에서는 poll에 의해 잠듦
	//	waitequeue_raed에 의해 깨어남
	return IRQ_HANDLED;
}
static int key_irq_init(void)
{
	int ret=0;
	int i;
	char * irq_name[8] = {"irq sw1","irq sw2","irq sw3","irq sw4","irq sw5","irq sw6","irq sw7","irq sw8"};

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		sw_irq[i] = gpio_to_irq(key[i]);
	}
	for (i = 0; i < ARRAY_SIZE(key); i++) {
		ret = request_irq(sw_irq[i],sw_isr, IRQF_TRIGGER_RISING, irq_name[i],NULL);
		if(ret) {
			printk("### FAILED Request irq %d. error : %d\n",sw_irq[i],ret);
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
#if 0
static void key_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++){
		gpio_free(key[i]);
	}
}
#endif
static void key_irq_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++){
		free_irq(sw_irq[i],NULL);
	}
}
static void led_write(char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i ) & 0x01);
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
}
#if 0
static void key_read(char * key_data)
{
	int i;
	char data=0;
//  	char temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
  		if(gpio_get_value(key[i]))
		{
			data = i+1;
			break;
		}
  
//  		temp = gpio_get_value(key[i]) << i;
//  		data |= temp;
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}
#endif

void kerneltimer_registertimer(KERNEL_TIMER_MANAGER *pdata)
{
    init_timer( &(pdata->timer) );
    pdata->timer.expires = get_jiffies_64() + pdata->time_val;  //10ms *100 = 1sec. jiffies 값이 100번 count 되면 종료. 1초 뒤에 kernel timer의 timeover 함수를 호출
    pdata->timer.data    = (unsigned long)pdata;    // pdata는 32바이트를 가리키는 포인터였는데, 주소값(unsigned long)만 가져옴
	// pdata는 포인터 변수. 
    pdata->timer.function = kerneltimer_timeover; //만료시간 (time_val)이 되면 이거 호출
    add_timer( &(pdata->timer) );   // kernel이 관리하고 있는 timer list에 등록
}

void kerneltimer_timeover(unsigned long arg)
{
    KERNEL_TIMER_MANAGER* pdata = NULL;
    if( arg )
    {
        pdata = ( KERNEL_TIMER_MANAGER *)arg;   // 32 byte를 가리키는 포인터로 변환
        led_write(pdata->led & 0x0f);   // 32bit 중에 하위 4bit 값만 가져와서 제어 (led는 4bit만 있음)

#if DEBUG1
        printk("led : %#04x\n",(unsigned int)(pdata->led & 0x0000000f));   // 1초마다 led 및 key값 표시
#endif
        pdata->led = ~(pdata->led); // led 반전
//		kerneltimer_registertimer(pdata, TIME_STEP);
        kerneltimer_registertimer(pdata);
    }
}

static int ledkey_open (struct inode *inode, struct file *filp)
{
	KERNEL_TIMER_MANAGER* ptrmng = NULL;
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );


ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc(sizeof(KE    RNEL_TIMER_MANAGER), GFP_KERNEL);   // KERNEL_TIME_MANAGER 사이즈 만큼의 메>    모리를 동적으로 할당
	
	if(ptrmng == NULL) return -ENOMEM;
	memset(ptrmng, 0, sizeof(KERNEL_TIMER_MANAGER));
	ptrmng->led = led_val;
	ptrmng->time_val = TIME_STEP;
	filp->private_data = ptrmng;
    return 0;

}


static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	if(!(filp->f_flags & O_NONBLOCK))
	{
    	if(sw_no == 0)	
    		interruptible_sleep_on(&WaitQueue_Read);
//  		wait_event_interruptible(WaitQueue_Read,sw_no);
//    		wait_event_interruptible_timeout(WaitQueue_Read,sw_no,100);	//100 * 1/100H = 1Sec
//			if(sw_no == 0)
//				return 0;
	}
	kbuf = (char)sw_no;
	ret=copy_to_user(buf,&kbuf,count);
	sw_no = 0;
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{	
	KERNEL_TIMER_MANAGER* ptrmng = NULL;
//    printk( "call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	char kbuf;
	int ret;
	ptrmng = filp->private_data;
//	get_user(kbuf,buf);	// 1byte만 읽어옴. 교수님은 이거 썼음
	ret=copy_from_user(&kbuf,buf,count);
////	led_write(kbuf);
	ptrmng->led = kbuf;
    return count;
//	return -EFAULT;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{	// arg r/w시 주소. 주소 값만 넘기므로 원래 가리키는 주소의 형변환 해서 씀
// 주소를 unsigned long으로 강제 형변환되서 정수로 넘어옴
KERNEL_TIMER_MANAGER* ptrmng = NULL;
	keyled_data info={0};
	 ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc(sizeof(KERNEL_TIMER_MANAGER), GFP_KERNEL);
	int err, size;
    if( _IOC_TYPE( cmd ) != IOCTLTEST_MAGIC ) return -EINVAL;
    if( _IOC_NR( cmd ) >= IOCTLTEST_MAXNR ) return -EINVAL;

    size = _IOC_SIZE( cmd );
    if( size )
    {
        err = 0;
        if( _IOC_DIR( cmd ) & _IOC_READ )
            err = access_ok( VERIFY_WRITE, (void *) arg, size );
        else if( _IOC_DIR( cmd ) & _IOC_WRITE )
            err = access_ok( VERIFY_READ , (void *) arg, size );
        if( !err )
            return err;
    }
	ptrmng = filp -> private_data;
	switch ( cmd )

	{
		case TIMER_START :
			if(!timer_pending(&(ptrmng->timer)))	// timer가 멈춰져있는지 확인. start 되어있는데 또 start하면 커널 패닉
//			kerneltimer_registertimer(ptrmng);
			kerneltimer_start(filp);	// 안에 있는 private data를 이용할 수 있음. ptrmng를 넘겨도 됨. 어차피 주소를 넘기는거는 사이즈 똑같음
			break;
		case TIMER_STOP :
			if(!timer_pending(&(ptrmng->timer)))
				kerneltimer_stop(filp);
//			del_timer(&(ptrmng->timer));
			break;
		case TIMER_VALUE :
			copy_from_user((void*)&info,(void*)arg,sizeof(info));
			ptrmng->time_val = info.timer_val;	// app에서 넘겼던 timer_val값을 kenel time manager의 time_val에 저장
			break;
	}
    printk( "call ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
    return 0x53;
}
static unsigned int ledkey_poll(struct file *filp, struct poll_table_struct *wait)
{
	int mask=0;
	poll_wait(filp, &WaitQueue_Read, wait);	// 스위치 안 눌렸는데 1초 지나면 time out에 의해서 깨어남
	// WaitQueue_Read에 의해 wakeup으로 깨어날 수도 있음
	if(sw_no > 0)
		mask = POLLIN;	// 읽을 데이터 발생
	return mask;
}
static int ledkey_release (struct inode *inode, struct file *filp)
{
	KERNEL_TIMER_MANAGER* ptrmng = NULL;
	ptrmng = filp->private_data;
	if(timer_pending(&(ptrmng->timer)))
		kerneltimer_stop(filp);

	if(ptrmng != NULL) // 메모리 해제 안했으면
	{
		kfre(ptrmng);
	}
    printk( "call release \n" );
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
	.poll	  = ledkey_poll,
    .open     = ledkey_open,     
    .release  = ledkey_release,  
};

static int ledkey_init(void)	// insmod 실행 시
{
    int result;

    printk( "call ledkey_init \n" );    

	
	KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
    if(ptrmng == NULL) return -ENOMEM;
    memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER));
    ptrmng->led = ledval;
//    kerneltimer_registertimer( ptrmng, TIME_STEP);

    result = register_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
    if (result < 0) return result;

	led_init();
//	key_init();
	result = key_irq_init();
    if (result < 0) return result;
    return 0;
}

static void ledkey_exit(void)	// rmmod 실행 시
{
	 KERNEL_TIMER_MANAGER* ptrmng = (KERNEL_TIMER_MANAGER *)kmalloc( sizeof(KERNEL_TIMER_MANAGER ), GFP_KERNEL);
	 if(timer_pending(&(ptrmng->timer)))	// timer가 구동 중인지 체크
        del_timer(&(ptrmng->timer));    	// 등록되어 있다면 해제
    if(ptrmng != NULL)  					// 해제가 안되어있다면
    {
        kfree(ptrmng);  					// 해제
    }
    printk( "call ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME );
	led_exit();
//	key_exit();
	key_irq_exit();
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
