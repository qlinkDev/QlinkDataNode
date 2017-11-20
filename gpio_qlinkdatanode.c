#include <stdio.h> // snprintf sprintf
#include <unistd.h> // access write usleep close
#include <fcntl.h> // open
#include <string.h>
#include <errno.h>
#include <stdlib.h> // atoi
#include <poll.h> // poll (as a timer)

#include "common_qlinkdatanode.h"

#define MDMEmodem_GPIO_STARTUP_STATUS (85)
#define MDMEmodem_GPIO_POWER_KEY (18)
#define MDMEmodem_GPIO_POWER_CTRL (80) // Add by mxl at 20150709 // Changed by rjf at 20150818
#define MDMEmodem_GPIO_STANDBY_CTRL (19)
#define MDMEmodem_GPIO_REBOOT_CTRL (22)

static bool isGpioPwrKeyInit = false;
static bool isGpioPwrCtrlInit = false;
static bool isGpioStartupStatInit = false;
static bool isGpioStandbyCtrlInit = false;
static bool isGpioRebootCtrlInit = false;

/*
#define INIT_TIMER struct timeval tv; memset(&tv, 0, sizeof(tv));

#define SET_TIMER_VAL(sec,usec) \
{ \
	tv.tv_sec = ((sec<0)?0:sec); \
	tv.tv_usec = ((usec<0)?0:usec); \
}

#define START_TIMER(sec,usec) \
{ \
	SET_TIMER_VAL(sec,usec); \
	select(0, NULL, NULL, NULL, &tv); \
}
*/

//Return Values:
//	  1: Complete initializing gpio file.
//	-1: open() failed.
//	-2: Fail to config sfd.
//	-3: Fail to create gpio file.
static int gpio_init(const int gpio)
{
	int sfd = -1;
	char checkstr[50] = {0};
	char configstr[10] = {0};
	char path[] = "/sys/class/gpio/export";
	int ret,len;

	sprintf(checkstr, "/sys/class/gpio/gpio%d/value", gpio);
	if(0 == access(checkstr, F_OK)){
//		LOG("access() return 0.\n");
		return 1;
	}
	
	sfd = open(path, O_WRONLY);
	if(sfd < 0){
		LOG("open(%s) failed. errno=%d.\n", path, errno);
		return -1;
	}
	len = sprintf(configstr, "%d", gpio);
	ret = write(sfd, configstr, len);

	if(ret != len){
		LOG("Fail to config gpio%d. write() returns %d. errno=%d.\n", gpio, ret, errno);

		close(sfd);
		return -2;
	}
	usleep(1000);
	ret = access(checkstr, F_OK);
	if(0 > ret){
		LOG("File gpio%d not exist. errno=%d.\n", gpio, errno);
		close(sfd);
		return -3;
	}
	close(sfd);
	return 1;
}

//Return Values:
//	-1: open() failed.
//	-2: read() failed.
int gpio_get_value(unsigned int gpio)
{
	int fd;
	char buf[64];
	char read_buf[2];
	int ret = -1;

	snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		LOG("Fail to open gpio%u. errno=%s(%d).\n", gpio, strerror(errno), errno);
		return -1;
	}

	__GPIO_GET_VALUE_READ_AGAIN__:
	ret = read(fd, read_buf, 2);
	if (ret < 0 || ret == 0) {
		if(errno == EINTR || errno == EAGAIN || ret == 0){
			goto __GPIO_GET_VALUE_READ_AGAIN__;
		}
		
		LOG("read() failed. errno=%d.\n", errno);
		return -2;
	}
	read_buf[1] = 0x00;

	ret = atoi(read_buf);

	close(fd);
	
	return ret;
}

//Return Values:
//	  1: Success.
//	-1: open() failed.
int gpio_set_value(const unsigned int gpio, const unsigned int value)
{
	int fd = -1;
	char path[64] = {0};

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
	
	fd = open(path, O_WRONLY);
	if(fd < 0){
		LOG("ERROR: Fail to open gpio%u. errno=%d.\n", gpio, errno);
		return -1;
	}
	
	if(value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
	
	close(fd);
	
	return 1;
}

int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[64];

	snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		LOG("Fail to open gpio%u. errno=%d.\n", gpio, errno);
		return -1;
	}

	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);

	close(fd);
	return 1;
}

//Return Values:
//	1 : Awake
//	0 : Standby
int gpio_mdmEmodem_check_if_awake(void)
{
	if(!isGpioStandbyCtrlInit){
		gpio_init(MDMEmodem_GPIO_STANDBY_CTRL);
		gpio_set_dir(MDMEmodem_GPIO_STANDBY_CTRL, 1);
		isGpioStandbyCtrlInit = true;
	}
	
	return ((gpio_get_value(MDMEmodem_GPIO_STANDBY_CTRL)==0)?1:0);
}

//Return Values:
//	-1: gpio_set_dir() error.
//	-2: gpio_get_value() error.
int gpio_mdmEmodem_check_startup(void)
{
	int gpio, ret = -99;
	
	LOG6("~~~~ gpio_mdmEmodem_check_startup ~~~~\n");
	
	gpio = MDMEmodem_GPIO_STARTUP_STATUS;
	
	if(!isGpioStartupStatInit){
		gpio_init(gpio);

		if(0 > gpio_set_dir(gpio, 0)){
			return -1;
		}
		
		isGpioStartupStatInit = true;
	}
	
	if(0 > (ret = gpio_get_value(gpio)) ){
		return -2;
	}
	
	return ret;
}

void gpio_mdmEmodem_pwr_sw_init(void)
{
	if(!isGpioPwrKeyInit){
		gpio_init(MDMEmodem_GPIO_POWER_KEY);
		gpio_set_dir(MDMEmodem_GPIO_POWER_KEY, 1);
		isGpioPwrKeyInit = true;
	}

	if(!isGpioPwrCtrlInit){	
		//Add by mxl at 20150709
	    gpio_init(MDMEmodem_GPIO_POWER_CTRL);
		gpio_set_dir(MDMEmodem_GPIO_POWER_CTRL, 1);
		isGpioPwrCtrlInit = true;
	}
	
	return;
}

void gpio_mdmEmodem_shutdown(void)
{
	gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 1);
	poll(NULL, 0, 5000); // Wait for 5s
	
	gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 0);
	poll(NULL, 0, 1000); // Wait for 1s
	
	return;
}

void gpio_mdmEmodem_shutdown_ext(void)
{
	gpio_set_value(MDMEmodem_GPIO_POWER_CTRL, 0);
	return;
}

void gpio_mdmEmodem_startup(void)
{
	//Add by mxl at 20150915
    gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 0);
	poll(NULL, 0, 100); // Wait for 100ms
	
	//Add by mxl at 20150709 //Changed by rjf at 20150818
    gpio_set_value(MDMEmodem_GPIO_POWER_CTRL, 1);
	poll(NULL, 0, 500); // Wait for 500ms
    
	gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 0);
	poll(NULL, 0, 3000); // Wait for 3s
	
	gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 1);
	poll(NULL, 0, 5000); // Wait for 5s
	
	gpio_set_value(MDMEmodem_GPIO_POWER_KEY, 0);
	
	return;
}

void gpio_mdmEmodem_uart_keep_awake(void)
{
	gpio_init(MDMEmodem_GPIO_STANDBY_CTRL);
	gpio_set_dir(MDMEmodem_GPIO_STANDBY_CTRL, 1);
	isGpioStandbyCtrlInit = true;
	
	gpio_set_value(MDMEmodem_GPIO_STANDBY_CTRL, 0);
	poll(NULL, 0, 500);
	return;
}

void gpio_mdmEmodem_reboot(void)
{
	if(!isGpioRebootCtrlInit){
		gpio_init(MDMEmodem_GPIO_REBOOT_CTRL);
		gpio_set_dir(MDMEmodem_GPIO_REBOOT_CTRL, 1);
		isGpioRebootCtrlInit = true;
	}
	
	gpio_set_value(MDMEmodem_GPIO_REBOOT_CTRL, 0);
	poll(NULL, 0, 500);
	gpio_set_value(MDMEmodem_GPIO_REBOOT_CTRL, 1);
	poll(NULL, 0, 10000); //Add by rjf at 20151016 (It takes 10s (around >8s) for USB or serial port to be available.)
	
	return;
}

void gpio_mdmEmodem_restart_pwr_ctrl_oper(void)
{
	if(!isGpioPwrCtrlInit){
	    gpio_init(MDMEmodem_GPIO_POWER_CTRL);
		gpio_set_dir(MDMEmodem_GPIO_POWER_CTRL, 1);
		isGpioPwrCtrlInit = true;
	}
	
	gpio_set_value(MDMEmodem_GPIO_POWER_CTRL, 0);
	poll(NULL, 0, 2000);
	return;
}