#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DEV_MAJOR 50
#define DEV_MINOR 0
#define DEV_MAJOR_2 100
#define DEV_MINOR_2 51
#define DEV_NAME "phstore"
#define DEV_NAME_2 "comstore"
static const char *mod_name = DEV_NAME;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Evgeny Karsskiy");
MODULE_DESCRIPTION(DEV_NAME);

#define MAXNUMS 100
#define MAXDATA 256
#define BIGBUFFER 10000

struct Person
{
    char name[MAXDATA];
    char addr[MAXDATA];
    char phone[MAXDATA];
};

static char field[10] = {0};
static char value[MAXDATA] = {0};

static int cnt = 0;
static struct Person persons[MAXNUMS];
static char buffer[BIGBUFFER] = {0};

static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static ssize_t dev_write_cmd(struct file *, const char *, size_t, loff_t *);

int mystrlen(char * str);
void mystrtok(char * str,char * buf, int start, char del);

static struct file_operations fops =
{
	.read = dev_read,
	.write = dev_write
};

static struct file_operations com_fops =
{
	.write = dev_write_cmd
};

static ssize_t dev_read(struct file * filp, char * buff, size_t  len, loff_t * off)
{
	printk(KERN_NOTICE "device readed\n");
	buffer[0] = '\0'; //типа чистка буфера
	int i=0;
	for(i = 0; i<cnt; i++){
		int add = 0;
		if(!strcmp("name",field)){
			if(!strcmp(persons[i].name,value))
				add = 1;
		} else if(!strcmp("addr",field)){
			if(!strcmp(persons[i].addr,value))
				add = 1;
		} else if(!strcmp("phone",field)){
			if(!strcmp(persons[i].phone,value))
				add = 1;
		} else {
			add = 1;
		}
		if(add){
			strcat(buffer, "Prson:{name=");
			strcat(buffer, persons[i].name);
			strcat(buffer,";addr=");
			strcat(buffer,persons[i].addr);
			strcat(buffer,";phone=");
			strcat(buffer,persons[i].phone);
			strcat(buffer,"}\n");
		}
	}
	
	printk(KERN_INFO "Persons: %s \n", buffer);
    int bufLen = strlen(buffer);
    if(!bufLen || *off>=bufLen ) return 0;
	int sz = len >= strlen(buffer)?strlen(buffer):len;
	if(copy_to_user(buff, buffer, sz)){
        return -EFAULT;
    }
	*off += sz;
	return len;
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
	strncpy(persons[cnt].name, name, MAXDATA);
	strncpy(persons[cnt].addr, addr, MAXDATA);
	strncpy(persons[cnt++].phone, phone, MAXDATA);
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
	return t;
}

static void __exit  my_exit(void)
{
	printk (KERN_NOTICE "[%s]Try unload module %s\n", mod_name, mod_name);
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
	unregister_chrdev(DEV_MAJOR_2, DEV_NAME_2);
};


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
