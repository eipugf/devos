#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/semaphore.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Evgeny Karssky");
MODULE_DESCRIPTION("Kernel Packet Sniffer");

struct task_struct *twife;
struct task_struct *tfarmer;
struct task_struct *tcow;
struct task_struct *tbarn;
struct task_struct *thayfield;

#define F_GIVE_GRASS 0
#define H_TAKE_GRASS 1
#define H_NO_GRASS 2
#define F_SAVE_GRASS 3
#define GET_MILK 4
#define TAKE_MILK 5
#define NO_MILK 6

int grass_cost = 10;
int cgrass_cost = 3;
int milk_cost = 5;

int exit = 0;

struct Channel {
	int data;
	struct semaphore rmutex;
	struct semaphore wmutex;
} farmout, harout, fb, bin, bout, cowin, cowout, wifein, test;

void write(struct Channel * c, int data){
	down_interruptible(&c->wmutex);
	c->data = data;
	up(&c->rmutex);
}

void read(struct Channel * c, int * res){
	down_interruptible(&c->rmutex);
	*res = c->data;
	up(&c->wmutex);
}


int wife(void * data)
{
    printk("wife thread started...");
	while (!exit) {
		int message = 0;
		printk("(wife) - send message get milk to cow");
		write(&cowin, GET_MILK);
        msleep(500);
        if(exit)
            break;
        read(&wifein,&message);
        if(exit)
            break;
        switch (message) {
		case NO_MILK: {
			printk("(wife) - recieve message no milk, uuuuu\n");
			break;
		}
		case TAKE_MILK: {
			printk("(wife) - resieve message take milk, uuu nyama nyama\n");
			break;
		}
		default:
			printk("(wife) - get unknown message %d\n",message);
		}
		schedule();
	}
	return 0;
}

int milk_count = 0;
int cow(void * data)
{
    printk("cow thread started...");
	while (!exit) {
		int message = 0;
		read(&cowin, &message);
        msleep(500);
        if(exit)
            break;
		switch (message) {
		case H_TAKE_GRASS: {
			milk_count += 1;
            printk("(cow) - taking grass, milk counter=%d\n", milk_count);
			break;
		}
		case H_NO_GRASS: {
			printk("(cow) - cow getting no grass message\n");
			break;
		}
		case GET_MILK: {
			printk("(cow) - get milk query");
			if(milk_count >= milk_cost){
				printk("(cow) - take milk");
				write(&wifein, TAKE_MILK);
				milk_count -= milk_cost;
			} else {
				printk("(cow) - no milk");
				write(&wifein, NO_MILK);
                printk("(cow) - send message to barn, get me grass\n");
                write(&bin, F_GIVE_GRASS);
			}
			break;
		}
		default:
			printk("(cow) - get unknown message %d\n", message);
		}
		schedule();
	}
	return 0;
}


int bgrass_count = 0;
int barn(void * data)
{
    printk("barn thread started...");
    printk("barn count %d", bgrass_count);
	while (!exit) {
		int message = 0;
		read(&bin, &message);
        msleep(500);
        if(exit)
            break;
		switch (message) {
		case F_SAVE_GRASS: {
			bgrass_count += grass_cost;
            printk("(barn) - getting save grass message, count = %d\n", bgrass_count);
			break;
		}
		case F_GIVE_GRASS:{
            printk("(barn) - getting give grass message\n");
            if(bgrass_count >= cgrass_cost){
                printk("(barn) - send message to cow take grass\n");
                bgrass_count -= cgrass_cost;
				write(&cowin, H_TAKE_GRASS);
                break;
            } else {
                printk("(barn) - send message to cow no grass\n");
				write(&cowin, H_NO_GRASS);
                break;
            }
		}
		default:
			printk("(barn) - get unknown message %d\n", message);
		}
		schedule();
	}
	return 0;
}

int farmer(void * data)
{
    printk("farmer thread started...\n");
	while (!exit) {
		int message = 0;
        printk("(farmer) send message to hayfield give grass...\n");
		write(&farmout, F_GIVE_GRASS);
        msleep(500);
        if(exit)
            break;
        read(&harout, &message);
		if(exit)
            break;
        printk("(farmer) - give message from hayfield %d\n", message);
        switch (message) {
		case H_TAKE_GRASS: {
			printk("(farmer) - getting grass, save grass to barn");
			write(&bin,F_SAVE_GRASS);
			break;
		}
		case H_NO_GRASS:{
			printk("(farmer) - no grass\n");
			break;
		}
		default:
			printk("(farmer) - get unknown message %d\n",message);
		}
		schedule();
	}
	return 0;
}

int hgrass_count = 0; //на лугу сначала нет травы
int hayfield(void * data)
{
    printk("hayfield thread started...");
	while(!exit){
		int message = 0;
		read(&farmout,&message);
        mdelay(1000);
        if(exit) 
            break;
		switch(message){
		case F_GIVE_GRASS:{
			printk("(hayfield) - give message from farmer\n");
			if(hgrass_count >= grass_cost){
				printk("(hayfield) - ok, take grass\n");
				hgrass_count -= grass_cost;
				write(&harout,H_TAKE_GRASS);
			} else {
				printk("(hayfield) - the grass has not grown yet\n");
                schedule();
				write(&harout,H_NO_GRASS);
                schedule();
			}
			break;
		}
		default:
			printk("(hayfield) - get unknown message %d \n",message);
		}
		hgrass_count++;
		schedule();
	}
	return 0;
}


int install_module(void)
{
    
    printk("init module\n");
	
    
    
    sema_init(&bin.rmutex, 1);
	sema_init(&bin.wmutex, 0);
    down_interruptible(&bin.rmutex);
	up(&bin.wmutex);
    printk("init bin\n");
	
    sema_init(&bout.rmutex, 1);
	sema_init(&bout.wmutex, 0);
    down_interruptible(&bout.rmutex);
	up(&bout.wmutex);
    printk("init bout\n");
	sema_init(&cowin.rmutex, 1);
	sema_init(&cowin.wmutex, 0);
    down_interruptible(&cowin.rmutex);
	up(&cowin.wmutex);
    printk("init cowin\n");
	sema_init(&cowout.rmutex, 1);
	sema_init(&cowout.wmutex, 0);
    down_interruptible(&cowout.rmutex);
	up(&cowout.wmutex);
    printk("init cowout\n");
	sema_init(&wifein.rmutex, 1);
	sema_init(&wifein.wmutex, 0);
    down_interruptible(&wifein.rmutex);
	up(&wifein.wmutex);
    printk("init wifein\n");
    
    sema_init(&test.rmutex, 1);
	sema_init(&test.wmutex, 0);
    down_interruptible(&test.rmutex);
	up(&test.wmutex);
    printk("init wifein\n");
    
    sema_init(&farmout.rmutex, 1);
	sema_init(&farmout.wmutex, 0);
    down_interruptible(&farmout.rmutex);
	up(&farmout.wmutex);
    printk("init fh\n");
    
    sema_init(&harout.rmutex, 1);
    sema_init(&harout.wmutex, 0);
    down_interruptible(&harout.rmutex);
	up(&harout.wmutex);
	printk("init hf\n");
    
    thayfield = kthread_run(&hayfield, 0, "hayfield");
    tfarmer = kthread_run(&farmer, 0, "farmer");
    tbarn = kthread_run(&barn, 0, "barn");
    tcow = kthread_run(&cow, 0, "cow");
    twife = kthread_run(&wife, 0, "wife");
    printk("threads started\n");
    
	return 0;
}

void out_module(void)
{
    printk("start out from module\n");
    
    exit = 1;
    printk("end out from module\n");
}

module_init(install_module);
module_exit(out_module);
