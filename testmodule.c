#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

#define MODULE_NAME "workquery module"

static const char *mod_name = MODULE_NAME;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("folk");
MODULE_DESCRIPTION(MODULE_NAME);
//--------------------------------------------------------------------
static struct workqueue_struct *my_wq;
int poll_delay = 1000; // задержка перед планированием задачи
struct timer_list my_timer;
static int rezolution;
int die = 0;
//структура – расширение структуры для отложенной работы с параметром
struct my_work_t {
    struct delayed_work my_work;
    int param;
};

int delay2;
struct my_work_t wrk;
//--------------------------------------------------------------------
//Планируемая функция
static void timer_function_workquery(struct work_struct *work) {
    struct my_work_t * str = (struct my_work_t *) (work) ;
    int param = (str->param) ++; //получаем параметр и ++
    printk("poll with param = %d...\n", param);//выводим
    //планируем себя вновь (или нет)

    if (!die)
        queue_delayed_work(my_wq, (struct delayed_work*) work,delay2);
 return;
}

//--------------------------------------------------------------------
static int __init my_init(void)
{
    printk("module's been loaded\n");
    //преобразование задержки в нужный формат
    rezolution = 1000 / HZ;
    delay2 = poll_delay / rezolution;
    my_wq = create_workqueue("rr_queue");
    if (my_wq) {
        wrk.my_work.timer = my_timer;
        wrk.param = 777;
        INIT_DELAYED_WORK(((struct delayed_work*) &wrk),timer_function_workquery);
        queue_delayed_work(my_wq, (struct delayed_work*) &wrk,delay2);
    }
    return 0;
}

//--------------------------------------------------------------------
static void __exit my_exit(void)
{
    die = 1;
    printk("cancel delayed work\n") ;
    cancel_delayed_work_sync((struct delayed_work *)&wrk);
    printk("flush wq\n") ;
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    printk("module's been unloaded\n");
};

//--------------------------------------------------------------------
module_init(my_init);
module_exit(my_exit);
