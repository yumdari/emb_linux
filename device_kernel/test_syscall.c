#include <linux/gpio.h>
#define DEBUG 1
#define IMX_GPIO_NR(bank, nr)       (((bank) - 1) * 32 + (nr))

int led[] = {
	IMX_GPIO_NR(1, 16),   //16
	IMX_GPIO_NR(1, 17),	  //17
	IMX_GPIO_NR(1, 18),   //18
	IMX_GPIO_NR(1, 19),   //19
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
		gpio_direction_output(led[i],0);	// i+1번째 led를 초기화하며 0으로 출력(pinMode(led[i]OUTPUT);
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

void led_write(unsigned long data)
{
	int i;
	for(i = 0; i <= ARRAY_SIZE(led); i++){
//		gpio_direction_output(led[i], (data >> i ) & 0x01);
	
		
		gpio_set_value(led[i], (data >> i ) & 0x01);
		//gpio_set_value(led[0], (0x01>>i)&0x01);
		//gpio_set_value(led[1], (0x02>>i)&0x01);
		//gpio_set_value(led[2], (0x04>>i)&0x01);
		//gpio_set_value(led[3], (0x08>>i)&0x01);
		//usleep(100000);
	}
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
}
void led_read(unsigned long * led_data)
{
	int i;
	unsigned long data=0;
	unsigned long temp;
	for(i=0;i<4;i++)
	{
  		gpio_direction_input(led[i]); //error led all turn off
		temp = gpio_get_value(led[i]) << i;
		data |= temp;
	}
/*	
	for(i=3;i>=0;i--)
	{
  		gpio_direction_input(led[i]); //error led all turn off
		temp = gpio_get_value(led[i]);
		data |= temp;
		if(i==0)
			break;
		data <<= 1;  //data <<= 1;
	}
*/
#if DEBUG
	printk("#### %s, data = %ld\n", __FUNCTION__, data);
#endif
	*led_data = data;
	//led_write(data);
	return;
}
asmlinkage long sys_mysyscall(unsigned long led_data)
{
	led_init();
	led_write(led_data);
  	//led_read(&led_data);
	led_exit();
	return (long)led_data;
}
