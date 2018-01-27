#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Evgeny Karsskiy");
MODULE_DESCRIPTION(DEV_NAME);

static double fumction(double x){
	return 2*x;
}

static double integrate(double a, double b, unsigned steps){
	double sum = 0.0,
    double step;
	int i;
	if (!step_count)
		return sum;

	step = (b - a) / (1.0 * step_count);
	for ( i = 1 ; i < step_count ; ++i ) {
		sum += fumction (a + i * step);
	}

	sum += (fumction(a) + fumction(b)) / 2;
	sum *= step;
	return sum;
}

/*
	инсталяция модуля
 */
static int __init my_init(void)
{
	return 0;
}

static void __exit  my_exit(void)
{
};

module_init(my_init);
module_exit(my_exit);
