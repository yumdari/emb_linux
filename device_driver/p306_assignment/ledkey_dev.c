#include <linux/init.h>
#include <linux/module.h>	// THIS_MODULE 매크로 사용
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <linux/fs.h>	// 함수 포인터 집합 file_operations 사용
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/gpio.h>	// 리눅스 드라이버 레벨에서 GPIO를 제어 하기 위해 리눅스 커널에서 제공하는 API
#include "ioctl_test.h"

#define   LEDKEY_DEV_NAME            "ioctldev"
#define   LEDKEY_DEV_MAJOR            240      
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

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
static int led_init(void)	// GPIO pin (LED) 초기화
{
	int ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(led); i++) {
		ret = gpio_request(led[i], "gpio led");
		if(ret<0){
			printk("#### FAILED Request gpio %d. error : %d \n", led[i], ret);
		} 
		else
			gpio_direction_output(led[i], 0);	// GPIO 출력 모드로 변경
												// gpio_direction_ouput( gp_nr,  init_val );   
												// init_val 는 초기값이다.
												// 0 : led off
	}
	return ret;
}

static int key_init(void)	// GPIO pin (Switch) 초기화
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
static void led_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(led); i++){
		gpio_free(led[i]);	// 반드시 gpio_free 하고 종료할 것
	} 
}
static void key_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(key); i++)
	{
		gpio_free(key[i]);	// 반드시  gpio_free하고 종료할 것
	}
}

static void led_write(char data)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(led); i++)
	{
		gpio_set_value(led[i], (data >> i ) & 0x01);	// GPIO 핀[i]을 1로 올려 줌 
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
}

static void key_read(char * key_data)
{
	int i;
	char data=0;
//	char temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		if(gpio_get_value(key[i]))	// GPIO 핀을 읽음 
		{
			data = i+1;	// 1 ~ 8 사이의 값을 return
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

static void key_read_ioctl(ioctl_test_info * info){
	int i;
	char data=0;
//	char temp;
	for(i=0;i<ARRAY_SIZE(key);i++)
	{
		if(gpio_get_value(key[i]))
		{
			data = i+1;
			break;
		}
//		temp = gpio_get_value(key[i]) << i;
//		data |= temp;
	}
#if DEBUG
	printk("#### %s, data = %d\n", __FUNCTION__, data);
#endif
	info->buff[0] = data;
	return;
}

static int ledkey_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );	// 주번호 출력
    printk( "call open -> minor : %d\n", num1 );	// 부번호 출력

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{	// 디바이스의 f_pos 위치에서 count 바이트만큼을 읽어서 사용자 영역인 buf로 저장해주는 기능
	char kbuf;
	int ret;
	key_read(&kbuf);
//	put_user(kbuf,buf);
	ret = copy_to_user(buf,&kbuf,count);
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)	
// 사용자 영역인 buf에서 count 바이트 만큼 읽은 후  device의 f_pos 위치로 저장
// struct file *flip   읽기와 쓰기에 전달되는 flip은 device file이 어떤 형식으로 열렸는가에 대한 정보를 저장
// loff_t f_ops        f_pos 필드 변수에는 현재의 읽기 / 쓰기 위치를 저장
{
	char kbuf;	// kernel buff
	int ret;
//	get_user(kbuf,buf);	// kbuf 변수에 buf의 user memory 값을 대입
	ret = copy_from_user(&kbuf,buf,count);	
	// kernel 영역과 user 영역은 서로 접근하지 못하는 메모리 영역이기에 pointer를 이용하지 못함
	// 데이터를 전달하는 copy_from_user 함수를 이용
	// user 영역의 buf 값을 kernel 영역의 kbuf에 전달

	led_write(kbuf);
    return count;
//	return -EFAULT;
}

/*           ret = ioctl(int fd, int requeset, char *argp)  */
/*                                   |              |       */ 
                      /* unsgined int cmd, unsigned long arg */
static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{	// kernel 영역에서의 ioctl 함수
	// inode와 flip pointer는 응용 프로그램의  file descriptor와 일치하는 인자
	// cmd는 명령을 나타내는 응용 프로그램의 인자 전달
	// cmd는 총 32 bit
	// [0:1] 읽기/쓰기 구분
	// [2:15] 데이터 크기
	// [16:23] 매직넘버
	// [24:31] 구분 번호
	// arg는 명령 실행의 결과 데이터가 전달되는 unsigned long 형의 정수 (또는 포인터). 인자 없어서 0으로 넘어감
	// 주소값 포인터를 주소값(unsigned long)으로만 받음

	/* 1 : Validate command
	   2 : Validate memory
	*/

	ioctl_test_info ctrl_info = {0,{0}};
	int err, size;
	char key_data = 0;
	// _IOC_TYPE() : Magic Number field 값을 읽는 Macro
	// _IOC_NR() : 구분 번호 field 값을 읽는 Macro
	// EINVAL : invalid argument
	if( _IOC_TYPE( cmd ) != IOCTLTEST_MAGIC ) return -EINVAL; // IOCTLTEST_MAIGC : 't'
	if( _IOC_NR( cmd ) >= IOCTLTEST_MAXNR ) return -EINVAL; // IOCTLTEEST_MAXNR : 4

	/* IOCTLTEST_KEYLEDINIT : cmd = {'t', 0} */ 
	/* IOCTLTEST_LEDON : cmd = {'t', 1} */ 
	/* IOCTLTEST_LEDOFF : cmd = {'t', 2} */ 
	/* IOCTLTEST_KEYLEDFREE : cmd = {'t', 3} */ 

	/* Validate memory */
	// _IOC_SIZE() : Read size of data field
	size = _IOC_SIZE( cmd );
	if( size )
	{
		err = 0;
		/* _IOC_DIR : Read attribute of r/w */
		if( _IOC_DIR( cmd ) & _IOC_READ ) /* if cmd == read ? */
			/* access_ok() : Check the address of arg in user space */
			/* #define access_ok(type,addr,size)   (__range_ok(addr,size) == 0) */
			/* #define __range_ok(addr,size)   ((void)(addr),0) */
			err = access_ok( VERIFY_WRITE, (void *) arg, size ); // VERIFY_WRITE : 1
		else if( _IOC_DIR( cmd ) & _IOC_WRITE ) /* if cmd == write ? */
			err = access_ok( VERIFY_READ , (void *) arg, size ); // VERIFY_READ : 0. fail이면 return. access_ok : read/write 전에  넘어온 주소가 접근 가능한 영역인지  체크
		if( !err ) return err;
	}
	switch( cmd )	// 일반적으로 I/O Control에 관련한 작업을 수행하는 함수
	{				// 대부분의 ioctl 메소드 구현은 cmd 인자 값에 따라 올바른 동작을 선택하는 switch문으로 구성
		/* KEY, LED 초기화 */
        case IOCTLTEST_KEYLEDINIT :
            key_init();
            led_init();
            break;

		/* KEY, LED 초기화 */
        case IOCTLTEST_KEYLEDFREE :
            key_exit();
            led_exit();
            break;

		/* LED OFF */
		case IOCTLTEST_LEDOFF :
			led_write(0);
			break;

		/* LED ON */
		case IOCTLTEST_LEDON :
			led_write(15);
			break;

		/* LED WRITE user->kernel */
		case IOCTLTEST_WRITE :
			/* type cast : pointer -> unsgined long -> pointer*/
			/* unknown size, so first argument is casting to (void *) */
			err = copy_from_user((void *)&ctrl_info, (void *)arg, sizeof(ctrl_info)); // user -> kernel
			led_write(ctrl_info.buff[0]); // led on
			break;

			/* 눌러진 키 값 읽어서 유저로 전송 */
		case IOCTLTEST_READ :	
			key_read(&key_data); // key_data에 눌러진 키 값 저장
			/* 만약 눌러진 키가 있다면 */
			if(key_data){
				/* 전송 전 구조체에 값 저장 */
				ctrl_info.buff[0] = key_data;
				ctrl_info.size = 1;
				err = copy_to_user((void *)arg, (void *)&ctrl_info, sizeof(ctrl_info)); // kernel -> user. (void*) arg : 주소값(상수)을  포인터 주소로 변경
			}
			break;

			/* 현재 눌러진 키 값을 리턴 */
		case IOCTLTEST_GETSTATE :	// return 값으로 key value return
			key_read(&key_data);
			return key_data;

			/* user->kernel => led 발광 => 눌러진 키 확인 => kernel->user */
		case IOCTLTEST_WRITE_READ :
			err = copy_from_user((void *)&ctrl_info, (void *)arg, sizeof(ctrl_info));	// 132 byte 만큼 user 영역에서 복사해옴
			if(err < 0) return err; // 실패시 음수 리턴
			led_write(ctrl_info.buff[0]); // led on
			memset(&ctrl_info, 0, sizeof(ioctl_test_info)); // 구조체 0으로 초기화
			key_read(&key_data);
			if(key_data){
				ctrl_info.buff[0] = key_data;
				ctrl_info.size = 1;
				err = copy_to_user((void *)arg, (void *)&ctrl_info, sizeof(ctrl_info)); // kernel -> user
			}
			else
				err = copy_to_user((void *)arg, (void *)&ctrl_info, sizeof(ctrl_info));

			if(err < 0) return err;
			break;
		default:
			break;
	}	
	return 0;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,		// 파일 오퍼레이션의 소유자
    .read     = ledkey_read,		// device의 f_pos에서 count 바이트만큼을 읽어서 user 영역인 buf에 저장해주는 기능
    .write    = ledkey_write,		// 사용자 영역인 buf에서 count 바이트 만큼 읽은 후 device의 f_pos에 저장
	.unlocked_ioctl = ledkey_ioctl,	// 모든 CPU에서 lock을 걸던 것을 개별적인 lock을 걸 수 있도록 ioctl에서 바꾼 함수
									// 함수에 진입 시 Kernel에 대한 lock이 처리되어지지 않기 때문에 함수 내에서 별도의 동기화 기법을 사용해야 한다.
									// 이 함수를 부르면 된다.
    .open     = ledkey_open,		// device driver가 처음 열렸을 때 H/W 초기화
    .release  = ledkey_release,		// open이 flip->private data에 할당한 데이터의 할당 삭제
									// 마지막 close 호출 시 device 종료
};

static int ledkey_init(void)
{
    int result;

    printk( "call ledkey_init \n" );    

    result = register_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);	// 디바이스 등록
    if (result < 0) return result;

//	led_init();
//	key_init();
    return 0;
}

static void ledkey_exit(void)
{
    printk( "call ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME );
//	led_exit();
//	key_exit();
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
