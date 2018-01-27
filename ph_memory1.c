#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#define DEV_MAJOR 50
#define DEV_MINOR 0
#define DEV_MAJOR_2 100
#define DEV_MINOR_2 51
#define DEV_NAME "phstore"
#define DEV_NAME_2 "comstore"
static const char *mod_name = DEV_NAME;

#define MAXNUMS 100
#define MAXDATA 256
#define BIGBUFFER 10000

static int pcount = 0;
static char *names[MAXNUMS];
static char *address[MAXNUMS];
static char *phones[MAXNUMS];


static char field[10] = {0};
static char value[MAXDATA] = {0};
static int cnt = 0;
static char buffer[BIGBUFFER] = {0};

static struct workqueue_struct *my_wq;
int poll_delay = 1000;
struct timer_list my_timer;
static int rezolution;
int die = 0;


struct my_work_t{
    struct delayed_work my_work;
    int size;
    int shift;
};

int delay2;
struct my_work_t wrk;

int mystrlen(char * str);
void mystrtok(char * str,char * buf, int start, char del);

static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static ssize_t dev_write_cmd(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.read = dev_read,
	.write = dev_write
};

static struct file_operations com_fops =
{
	.write = dev_write_cmd
};

static void timer_function_workquery(struct work_struct *work){
    struct my_work_t * str = (struct my_work_t *)(work);
    int size = str->size;
    int shift = str->shift;
    int i;
    for(i = shift; i<shift+size; i++){
		int add = 0;
		if(!strcmp("name",field)){
			if(!strcmp(names[i],value))
				add = 1;
		} else if(!strcmp("addr",field)){
			if(!strcmp(address[i],value))
				add = 1;
		} else if(!strcmp("phone",field)){
			if(!strcmp(phones[i],value))
				add = 1;
		} else {
			add = 1;
		}
		if(add){
            printk("Person{name=%s, addr=%s, phone=%s}\n",names[i],address[i],phones[i]);
		}
	}
}

static ssize_t dev_read(struct file * filp, char * buff, size_t  len, loff_t * off)
{
    printk(KERN_NOTICE "device readed\n");
    wrk.my_work.timer = my_timer;
    INIT_DELAYED_WORK(((struct delayed_work *) &wrk),
                      timer_function_workquery);
    queue_delayed_work(my_wq, (struct delayed_work *) &wrk, 
                       delay2);
    
	//printk(KERN_INFO "Persons: %s \n", buffer);
	strncpy(buffer,"use dmesg -wH command", MAXDATA);
    int bufLen = strlen(buffer);
    if(!bufLen || *off>=bufLen ) return 0;
	int sz = len >= strlen(buffer)?strlen(buffer):len;
	if(copy_to_user(buff, buffer, sz)){
        return -EFAULT;
    }
	*off += sz;
	return len;
	return 0;
}

static ssize_t dev_write(struct file * fil, const char * buff, size_t len, loff_t * off)
{
    printk(KERN_NOTICE "dev write \n");
	char msg[256];
	if (len > sizeof(msg) - 1)
		return -EINVAL;
	unsigned long ret = copy_from_user(msg, buff, len);
	if (ret)
		return -EFAULT;
	msg[len] = '\0';

	if(cnt >= MAXNUMS)
		return -EFAULT;
    
    printk(KERN_NOTICE "readed %s \n",msg);
    
    int namelen = 0;
    int addrlen = 0;
    int phlen = 0;
    char name[MAXDATA] = {0};
    char addr[MAXDATA] = {0};
    char phone[MAXDATA] = {0};
    
    mystrtok(msg, name, 0, ';');
    if(name && (namelen = mystrlen(name))){
        mystrtok(msg, addr, namelen+1, ';');
        if(addr && (addrlen = mystrlen(addr))){
            mystrtok(msg, phone, namelen+addrlen+2, ';');
            phlen = mystrlen(phone);
        }
    }
    if(!namelen && !addrlen && !phlen){
        return -EFAULT;
    }
    
    names[pcount] = (char *)vmalloc((namelen+1)*sizeof(char));
    address[pcount] = (char *)vmalloc((addrlen+1)*sizeof(char));
    phones[pcount] = (char *)vmalloc((phlen+1)*sizeof(char));
    
    strncpy(names[pcount], name, MAXDATA);
    strncpy(address[pcount], addr, MAXDATA);
    strncpy(phones[pcount++], phone, MAXDATA);
    
    printk(KERN_INFO "readed name %s\n", name);
    printk(KERN_INFO "readed addr %s\n", addr);
    printk(KERN_INFO "readed phone %s\n", phone);
	return len;
}

static ssize_t dev_write_cmd(struct file * fil, const char * buff, size_t len, loff_t * off){
    printk(KERN_NOTICE "write command \n");
    char command[266];
	if (len > sizeof(command) - 1)
		return -EINVAL;
	unsigned long ret = copy_from_user(command, buff, len);
	if (ret)
		return -EFAULT;
	command[len] = '\0';

	char fld[MAXDATA] = {0};
    char vle[MAXDATA] = {0};

    mystrtok(command,fld,0,';');
    int fieldlen = strlen(fld);
    mystrtok(command,vle,fieldlen+1,';');
    printk(KERN_NOTICE "get command field=%s value=%s",fld,vle);
    strncpy(field, fld, MAXDATA);
	strncpy(value, vle, MAXDATA);
	return len;
}


/*
	инсталяция модуля
 */
static int __init my_init(void)
{    
    printk (KERN_NOTICE "[%s] Try init module %s\n", mod_name, mod_name);
	int t = register_chrdev(DEV_MAJOR, DEV_NAME, &fops);
	if (t < 0){
		printk(KERN_ALERT "First device registration Failed");
		return t;
	} else {
		printk(KERN_ALERT "First device registered \n");
	}
	t = register_chrdev(DEV_MAJOR_2, DEV_NAME_2, &com_fops);
	if (t < 0){
		printk(KERN_ALERT "Second device registration Failed");
		return t;
	} else {
		printk(KERN_ALERT "Second device registered \n");
	}
	
	rezolution = 1000 / HZ;
    delay2 = poll_delay / rezolution;
    my_wq = create_workqueue("rr_queue");
	return t;
}

static void __exit  my_exit(void)
{
    printk (KERN_NOTICE "[%s]Try unload module %s\n", mod_name, mod_name);
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
	unregister_chrdev(DEV_MAJOR_2, DEV_NAME_2);
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    int i;
    for(i=0; i<pcount; i++){
        vfree(names[i]);
        vfree(address[i]);
        vfree(phones[i]);
    }
}

int mystrlen(char * str){
    int i;
    for(i = 0; str!=NULL && *(str+i)!='\0' && i < MAXDATA; i++);
    return i;
}

void mystrtok(char * str,char * buf, int start, char del){
    if(str == NULL || buf == NULL)
        return NULL;
    int i;
    int ri = 0;
    for(i = start; i<MAXDATA; i++){
        if(*(str+i) == del || *(str+i) == '\0')
            break;
        *(buf+ri++) = *(str+i);
    }
    *(buf+ri)='\0';
}

module_init(my_init);
module_exit(my_exit);
