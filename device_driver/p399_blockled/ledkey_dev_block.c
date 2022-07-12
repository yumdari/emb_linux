#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/gpio.h>

#include <asm/uaccess.h> //copy_to_user, get_user 등을 이용하기 위한 헤더파일 선언

#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/sched.h>	//blocking header file (schedule 관련)
#include <linux/wait.h>	//blocking header file 

#define   LEDKEY_DEV_NAME            "block_led"
#define   LEDKEY_DEV_MAJOR            240      
#define   DEBUG 						0
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);

static int sw_irq[8] = {0};
//전역변수는 가능한 static으로 사용하는것이 좋다(해당 파일에서만 사용)
static int sw_no;

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
//**********************************************************************************//
//		                  interuppt service START 							  		//
//**********************************************************************************//
irqreturn_t sw_isr(int irq, void *unuse)
{
	int i;	

	for( i = 0; i < ARRAY_SIZE(key); i++) {
		if(irq == sw_irq[i]) {
			sw_no = 1+i;
			break;
		}
	}
	printk("IRQ : %d, %ld\n", irq, sw_no);
	wake_up_interruptible(&WaitQueue_Read);
	
	return IRQ_HANDLED;

}


static int key_irq_init(void)
{
	int ret = 0;
	int i;
	char * irq_name[8] = {"irq_sw1","irq_sw2","irq_sw3","irq_sw4","irq_sw5","irq_sw6","irq_sw7","irq_sw8"};
	//배열은 배열인데 배열의 요소가 포인터를 저장하는 배열(주소값이 캐릭터형)
	for( i = 0; i < ARRAY_SIZE(key); i++) {
		sw_irq[i] = gpio_to_irq(key[i]);					//irq gpio로 설정		
	}
	for( i = 0; i < ARRAY_SIZE(key); i++) {
		ret = request_irq(sw_irq[i], sw_isr, IRQF_TRIGGER_RISING, irq_name[i], NULL); //스위치 눌리는 시점에 인터럽트 구현되게 한다
	}
	if(ret) {
		printk(" ### FAILED Request irq %d. error : %d", sw_irq[i],ret);
	}
	/*
	sw_irq[0] = gpio_to_irq(IMX_GPIO_NR(1,20));
	sw_irq[1] = gpio_to_irq(IMX_GPIO_NR(1,21));
	sw_irq[2] = gpio_to_irq(IMX_GPIO_NR(4,8));
	sw_irq[3] = gpio_to_irq(IMX_GPIO_NR(4,9));
	sw_irq[4] = gpio_to_irq(IMX_GPIO_NR(4,5));
	sw_irq[5] = gpio_to_irq(IMX_GPIO_NR(7,13));
	sw_irq[6] = gpio_to_irq(IMX_GPIO_NR(1,7));
	sw_irq[7] = gpio_to_irq(IMX_GPIO_NR(1,8));	
	*/
	return ret;
}
static void led_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_direction_output(led[i],0);
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
	for( i = 0; i < ARRAY_SIZE(key); i++) {
		free_irq(sw_irq[i], NULL);	
	}
}
//****************************************************************************************//
//		                  interuppt service END    							  			  //
//****************************************************************************************//
static void led_write(char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++){
		gpio_set_value(led[i], (data >> i ) & 0x01);
		//1이면 1을 쓰고 2면 2를 쓴다
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
//	char temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		if(gpio_get_value(key[i])) //데이터가 참이면
		{
			data = i+1; //스위치가 낮은것을 우선권으로 주겠다
			break;
		}
		
//		temp = gpio_get_value(key[i]) << i;
//		data |= temp;
	}

#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
	*key_data = data;
	return;
}
#endif
static int ledkey_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
//    printk( "call read -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	char kbuf;
	int ret;

	if(!(filp->f_flags & O_NONBLOCK))
	{
	//key_read(buf);
	/////key_read(&kbuf);
		kbuf = (char)sw_no; //blocking 안함
	//put_user(kbuf, buf);//앞은 커널공간(앞kbuf는 값), 뒤는 사용자공간의 주소(뒤 버퍼는 포인터)
	//put_user 아래와 같이 바꾸기 가능
		ret = copy_to_user(buf, &kbuf, count);
		sw_no = 0;
	}
	else
	{
		if(sw_no == 0)
			interruptible_sleep_on(&WaitQueue_Read);
		//else
		//{
		kbuf = (char)sw_no;
		ret = copy_to_user(buf, &kbuf, count);
		sw_no = 0;
		//}
	}
	return count;
	//else
		count = 0;
	//read는 copy to user가 나중에
	/*
	char kbuf[10] = {0};
	for (int kdx = 0; kdx < 10; kdx++) {
		put_user(kbuf[kdx], buf++)
	}
	*/
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
//    printk( "call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	char kbuf;
//	get_user(kbuf, buf); //프로세스가 계속 실행되는 상태기때문에 이 상태에서는 똑같다
	//애플리케이션 입장에서는 write
	//write 에서는 copy from user
	copy_from_user(&kbuf, buf, count);
	led_write(kbuf);	
//	led_write(*buf);
    return count;
//	return -EFAULT;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

    printk( "call ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
    return 0x53;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
    .open     = ledkey_open,     
    .release  = ledkey_release,  
};

static int ledkey_init(void)
{
    int result;

    printk( "call ledkey_init \n" );    

    result = register_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
	//가상 파일 시스템을 거쳐서 사용(등록하는 과정)
    if (result < 0) return result;

	led_init();
	//key_init();
	key_irq_init();		//이제는 인터럽트 방식으로 설정해서 초기화를 해야 한다(led와 다르게 인터럽트 필요)

    return 0;
}

static void ledkey_exit(void)
{
    printk( "call ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME );	
	led_exit();
	//key_exit();
	key_irq_exit();	
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
