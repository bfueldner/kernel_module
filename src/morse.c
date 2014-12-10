// morse.c: Verwendung von Dig-Output zum Morsen
//
// Mechanismen:
//  * Dateischnittstelle: open(), close(), write()
//  * GPIO's
//  * dynamisch oder statisch vergebene Major-Number
//  * Generierung eines /sys-FS-Eintrages inkl. UEVENT für udevd bzw. mdev
//

#include <linux/version.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/delay.h>
//#include <linux/class.h>

#define MODULE_NAME "morse"

#define MORSE_INPUT 20
#define MORSE_OUTPUT
#define MORSE_MEMORY_PAGES 8
#define MORSE_MEMORY_ORDER 3

struct morse_zeichen
{
	char c;
	char* z;
};

static struct morse_zeichen morse_table[] = {
	{'a', ".-S"},
	{'b', "-...S"},
	{'c', "-.-.S"},
	{'d', "-..S"},
	{'e', ".S"},
	{'f', "..-.S"},
	{'g', "--.S"},
	{'h', "....S"},
	{'i', "..S"},
	{'j', ".---S"},
	{'k', "-.-S"},
	{'l', ".-..S"},
	{'m', "--S"},
	{'n', "-.S"},
	{'o', "---S"},
	{'p', ".--.S"},
	{'q', "--.-S"},
	{'r', ".-.S"},
	{'s', "...S"},
	{'t', "-S"},
	{'u', "..-S"},
	{'v', "...-S"},
	{'w', ".--S"},
	{'x', "-..-S"},
	{'y', "-.--S"},
	{'z', "--..S"},
	{'1', ".----S"},
	{'2', "..---S"},
	{'3', "...--S"},
	{'4', "....-S"},
	{'5', ".....S"},
	{'6', "-....S"},
	{'7', "--...S"},
	{'8', "---..S"},
	{'9', "----.S"},
	{'0', "-----S"},
	{0, NULL},
};


struct morse_string
{
	int len;
	char s[100];
};

struct interrupt_data
{
    int data;
};

static struct interrupt_data idata;

static int irqnr = 0;

static int index = 0;
static int morse_led = 103;

static char* morse_mem;

static unsigned int morse_dot_unit   = 500;  // [ms]
//static const int morse_led_yellow = 62;
static const int morse_led_yellow = 103;
static const int morse_led_orange = 63;
static const int morse_btn1 = 103;
static const int morse_btn2 = 104;

static dev_t morse_dev;
static struct cdev morse_cdev;

static struct class* morse_class;
static struct device* morse_device;

static void morse_lang(void)
{
	gpio_set_value (morse_led_yellow, 1);
    msleep(3 * morse_dot_unit);
	gpio_set_value (morse_led_yellow, 0);
    msleep(morse_dot_unit);
}

static void morse_kurz(void)
{
    gpio_set_value (morse_led_yellow, 1);
    msleep(morse_dot_unit);
	gpio_set_value (morse_led_yellow, 0);
    msleep(morse_dot_unit);
}

static void morse_letter_space(void)
{
	// Pause: 3 Dot-Spaces
	// 1 Dot-Space von kurz-/lang-Morse-Zeichen vorhanden
	msleep(2 * morse_dot_unit);
}

static void morse_word_space(void)
{
	// Pause: 7 Dot-Spaces
	// 1 Dot-Space von kurz-/lang-Morse-Zeichen vorhanden
	// 2 Dot-Spaces von Letter-Space vorhanden
    msleep(4 * morse_dot_unit);
}

size_t led_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    morse_led = simple_strtol(buf, NULL, 10);
    return count;
}

size_t led_show(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    sprintf(buf, "GPIO = %d\n", morse_led);
    return strlen(buf);
}

static void morse_char(char ch)
{
	int i = 0;

	while ((morse_table[i].c) && (morse_table[i].c != tolower(ch)))
		i++;

	if (morse_table[i].c)
	{
		int j = 0;
		while (morse_table[i].z[j] != 'S')
		{
			switch (morse_table[i].z[j])
			{
				case '.':
					morse_kurz();
					break;
				case '-':
					morse_lang();
					break;
			}
			j++;
		}
		morse_letter_space();
	}
	else
	{
		// Vereinfachung: 
		// eigentlich für Space, jedoch auch bei allen anderen nicht gefundenen Zeichen
		morse_word_space();
	}
}



int morse_fkt (void* data)
{
	int i;
	struct morse_string* ms = (struct morse_string*)data;

	printk (KERN_INFO "morse_fkt(START): (%d) %s\n", ms->len, ms->s);

	for (i = 0; i < ms->len; i++)
	{
		morse_char(ms->s[i]);
	}

	kfree (data);

	printk (KERN_INFO "morse_fkt(ENDE): (%d) %s\n", ms->len, ms->s);

	return 0;
}

static ssize_t morse_write(struct file *file, const char *buf, size_t count, loff_t *offset)
{
	int nLen;
	struct morse_string* ms = NULL;

	ms = kmalloc (sizeof(struct morse_string), GFP_KERNEL | __GFP_ZERO);

	nLen = sizeof(ms->s);
	if (count < nLen)
		nLen = count;

	if ( !copy_from_user( ms->s, buf, nLen) )
	{
		ms->len = nLen;
		printk (KERN_INFO "Morse: (%d) %s\n", ms->len, ms->s);

		morse_fkt ((void*)ms);

		return (count);
	}
	else
	{
		kfree (ms);
		return (-ENOMEM);
	}
}


static int morse_open(struct inode *node, struct file *file)
{
	return 0;
}


static int morse_close(struct inode *node, struct file *file)
{
	return 0;
}

int morse_mmap(struct file *filep, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start;

    if (length > MORSE_MEMORY_PAGES * PAGE_SIZE)
        return -EIO;

    if ((ret = remap_pfn_range(vma,
                  vma->vm_start,
                               virt_to_phys((void *)morse_mem) >> PAGE_SHIFT,
                               length,
                               vma->vm_page_prot)) < 0)
    {
        return ret;
    }
    return 0;
}

static struct file_operations morse_fops = {
	.owner   = THIS_MODULE,
	.open    = morse_open,
	.release = morse_close,
    .write   = morse_write,
    .mmap    = morse_mmap
};



DEVICE_ATTR(led, 0644, led_show, led_store);

void morse_task(unsigned long data)
{
    printk(KERN_INFO "morse_task: data %d\n", data);
    ((unsigned long *)morse_mem)[index] = index++;
}

static DECLARE_TASKLET (morse_tasklet, morse_task, 0);

irqreturn_t morse_interrupt(int irq, void* dev_id)
{
    struct interrupt_data* data = (struct interrupt_data *)dev_id;
    printk(KERN_INFO "ISR() %d %d\n", irq, data->data);

    morse_tasklet.data = 999;
    tasklet_schedule(&morse_tasklet);

    return IRQ_HANDLED;
}

static int __init morse_init(void)
{
	int ret = 0;

    printk(KERN_INFO "morse_init()\n");

    morse_dev = MKDEV(240, 0);
	if ((ret = register_chrdev_region(morse_dev, 1, MODULE_NAME)) < 0) 
	{
		printk(KERN_ERR "morse_init(): register_chrdev_region(): ret=%d\n", ret);
		ret = -1;
		goto out;
	}

	cdev_init(&morse_cdev, &morse_fops);
	if ((ret = cdev_add(&morse_cdev, morse_dev, 1)) < 0) 
	{
		printk(KERN_ERR "morse_init(): cdev_add(): ret=%d\n", ret);
		ret = -2;
		goto out_unalloc_region;
	}


	ret = gpio_request (morse_led_yellow, MODULE_NAME);
	if (ret)
	{
		printk(KERN_ERR "morse_init(): gpio_request: ret=%d\n", ret);
		ret = -6;
		goto out_device_remove_file;
	}

	ret = gpio_direction_output (morse_led_yellow, 0);
	if (ret)
	{
		printk(KERN_ERR "morse_init(): gpio_direction_output: ret=%d\n", ret);
		ret = -7;
		goto out_gpio_free;
	}

    idata.data = 13;
    irqnr = gpio_to_irq(20);
    printk(KERN_INFO "Interrupt for gpio is %d\n", irqnr);

    if (request_irq(irqnr, morse_interrupt, IRQF_SHARED | IRQF_TRIGGER_RISING, "morse", &idata))
    {
        printk(KERN_ERR "morse_init(): Interrupt belegt\n");
    }

    if ((morse_mem = (char *)__get_free_pages(GFP_KERNEL, MORSE_MEMORY_ORDER)) == NULL)
    {
        return -ENOMEM;
    }

	/*
	   ret = gpio_request (103, MODULE_NAME);
	   if (ret)
	   {
	   printk(KERN_ERR "morse_init(): gpio_request(103): ret=%d\n", ret);
	   }
	   ret = gpio_direction_input (103);
	   if (ret)
	   {
	   printk(KERN_ERR "morse_init(): gpio_direction_input(103): ret=%d\n", ret);
	   }
	   printk (KERN_INFO "GPIO(103): %d\n", gpio_get_value(103));

	   ret = gpio_request (104, MODULE_NAME);
	   if (ret)
	   {
	   printk(KERN_ERR "morse_init(): gpio_request(104): ret=%d\n", ret);
	   }
	   ret = gpio_direction_input (104);
	   if (ret)
	   {
	   printk(KERN_ERR "morse_init(): gpio_direction_input(104): ret=%d\n", ret);
	   }
	   printk (KERN_INFO "GPIO(104): %d\n", gpio_get_value(104));
	 */

    morse_class = class_create(THIS_MODULE, "morse");
    if (IS_ERR(morse_class))
    {
        printk(KERN_ERR "class_create() - err: %d\n", PTR_ERR(morse_class));
        return -PTR_ERR(morse_class);
    }

    morse_device = device_create(morse_class, NULL, morse_dev, NULL, "morse%d", MINOR(morse_dev));
    ret = device_create_file(morse_device, &dev_attr_led);
    if (ret)
    {
        printk(KERN_ERR "device_create_file(): ret = %d\n", ret);
    }


	return ret;

out_device_remove_file:


out_gpio_free:
	gpio_free(morse_led_yellow);



out_cdev_del:
	cdev_del(&morse_cdev);
out_unalloc_region:
	unregister_chrdev_region(morse_dev, 1);
out:
	return ret;
}

static void __exit morse_exit(void)
{
    free_pages((long)morse_mem, MORSE_MEMORY_ORDER);

    free_irq(irqnr, &idata);

    device_remove_file(morse_device, &dev_attr_led);
    device_destroy(morse_class, morse_dev);
    class_destroy(morse_class);

	gpio_free(morse_led_yellow);

	cdev_del(&morse_cdev);
	unregister_chrdev_region(morse_dev, 1);

	printk (KERN_INFO "morse_exit()\n");
}

module_init(morse_init);
module_exit(morse_exit);
module_param(morse_led, int, S_IRUGO);

MODULE_DESCRIPTION("Morsezeichen mittels GPIO");
MODULE_AUTHOR("bfueldner");
MODULE_LICENSE("GPL");
