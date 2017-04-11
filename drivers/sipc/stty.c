/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/sipc.h>
#include <linux/stty.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>


#include <linux/compat.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>

#define STTY_DEV_MAX_NR 	1
#define STTY_MAX_DATA_LEN 		4096

struct stty_device {
	struct stty_init_data	*pdata;
	struct tty_port 		*port;
	struct tty_struct 		*tty;
	struct tty_driver 		*driver;
};

static void stty_handler (int event, void* data)
{
	struct stty_device *stty = data;
	int i, cnt = 0;
	unsigned char buf[STTY_MAX_DATA_LEN] = {0};

	pr_debug("stty handler event=%d \n", event);

	switch(event) {
		case SBUF_NOTIFY_WRITE:
			break;
		case SBUF_NOTIFY_READ:
			cnt = sbuf_read(stty->pdata->dst, stty->pdata->channel,
					stty->pdata->bufid,(void *)buf, STTY_MAX_DATA_LEN, 0);
			pr_debug("stty handler read data len =%d \n", cnt);
			if (cnt > 0) {
				for(i = 0; i < cnt; i++) {
					tty_insert_flip_char(stty->port, buf[i], TTY_NORMAL);
				}
				tty_schedule_flip(stty->port);
			}
			break;
		default:
			printk(KERN_ERR "Received event is invalid(event=%d)\n", event);
	}

	return;
}

static int stty_open(struct tty_struct *tty, struct file * filp)
{
	struct stty_device *stty = NULL;
	struct tty_driver *driver = NULL;
	int rval= 0;

	if (tty == NULL) {
		printk(KERN_ERR "stty open input tty is NULL!\n");
		return -ENOMEM;
	}
	driver = tty->driver;
	stty = (struct stty_device *)driver->driver_state;

	if(stty == NULL) {
		printk(KERN_ERR "stty open input stty NULL!\n");
		return -ENOMEM;
	}

	if (sbuf_status(stty->pdata->dst, stty->pdata->channel) != 0) {
		printk(KERN_ERR "stty_open sbuf not ready to open!dst=%d,channel=%d\n"
			,stty->pdata->dst,stty->pdata->channel);
		return -ENODEV;
	}
	stty->tty = tty;
	tty->driver_data = (void *)stty;

	rval = sbuf_register_notifier(stty->pdata->dst, stty->pdata->channel,
					stty->pdata->bufid, stty_handler, stty);
	if (rval) {
		printk(KERN_ERR "regitster notifier failed (%d)\n", rval);
		return rval;
	}
	pr_debug("stty_open device success! \n");

	return 0;
}

static void stty_close(struct tty_struct *tty, struct file * filp)
{
	struct stty_device *stty = NULL;
	int rval= 0;
	if (tty == NULL) {
		printk(KERN_ERR "stty close input tty is NULL!\n");
		return;
	}
	stty = (struct stty_device *) tty->driver_data;
	if (stty == NULL) {
		printk(KERN_ERR "stty close s tty is NULL!\n");
		return;
	}

	rval = sbuf_register_notifier(stty->pdata->dst, stty->pdata->channel,
					stty->pdata->bufid, NULL, NULL);
	if (rval) {
		printk(KERN_ERR "unregitster notifier failed (%d)\n", rval);
		return;
	}

	pr_debug("stty_close device success !\n");

	return;
}

static int stty_write(struct tty_struct * tty,
	      const unsigned char *buf, int count)
{
	struct stty_device *stty = tty->driver_data;
	int cnt;
	pr_debug("stty write count=%d \n", count);

	cnt = sbuf_write(stty->pdata->dst, stty->pdata->channel,
					stty->pdata->bufid, (void *)buf, count, -1);
	return cnt;
}


static void stty_flush_chars(struct tty_struct *tty)
{
	return;
}

static int stty_write_room(struct tty_struct *tty)
{
	return INT_MAX;
}

static const struct tty_operations stty_ops = {
	.open  = stty_open,
	.close = stty_close,
	.write = stty_write,
	.flush_chars = stty_flush_chars,
	.write_room  = stty_write_room,
};

static struct tty_port *stty_port_init()
{
	struct tty_port *port = NULL;

	port = kzalloc(sizeof(struct tty_port),GFP_KERNEL);
	if (port == NULL) {
		printk(KERN_ERR "stty_port_init Failed to allocate device!\n");
		return NULL;
	}
	tty_port_init(port);
	return port;
}

static int stty_driver_init(struct stty_device *device)
{
	struct tty_driver *driver;
	int ret = 0;

	device->port = stty_port_init();
	if (!device->port) {
		return -ENOMEM;
	}

	driver = alloc_tty_driver(STTY_DEV_MAX_NR);
	if (!driver)
		return -ENOMEM;
	/*
	 * Initialize the tty_driver structure
	 * Entries in stty_driver that are NOT initialized:
	 * proc_entry, set_termios, flush_buffer, set_ldisc, write_proc
	 */
	driver->owner = THIS_MODULE;
	driver->driver_name = device->pdata->name;
	driver->name = device->pdata->name;
	driver->major = 0;
	driver->type = TTY_DRIVER_TYPE_SYSTEM;
	driver->subtype = SYSTEM_TYPE_TTY;
	driver->init_termios = tty_std_termios;
	driver->driver_state = (void*)device;
	device->driver = driver;
	 /* initialize the tty driver */
	tty_set_operations(driver, &stty_ops);
	tty_port_link_device(device->port, driver, 0);
	ret = tty_register_driver(driver);
	if (ret) {
		put_tty_driver(driver);
		tty_port_destroy(device->port);
		return ret;
	}
	return ret;
}

static void stty_driver_exit(struct stty_device *device)
{
	struct tty_driver *driver = device->driver;
	tty_unregister_driver(driver);
	tty_port_destroy(device->port);
}

static int  stty_probe(struct platform_device *pdev)
{
	struct stty_init_data *pdata = (struct stty_init_data*)pdev->dev.platform_data;
	struct stty_device *stty;
	int rval= 0;

	stty = kzalloc(sizeof(struct stty_device),GFP_KERNEL);
	if (stty == NULL) {
		printk(KERN_ERR "stty Failed to allocate device!\n");
		return -ENOMEM;
	}

	stty->pdata = pdata;
	rval = stty_driver_init(stty);
	if (rval) {
		kfree(stty);
		printk(KERN_ERR "stty driver init error!\n");
		return -EINVAL;
	}

	printk( "stty_probe init device addr: 0x%0x\n", (void *)stty);
	platform_set_drvdata(pdev, stty);

	return 0;
}

static int  stty_remove(struct platform_device *pdev)
{
	struct stty_device *stty = platform_get_drvdata(pdev);

	stty_driver_exit(stty);
	kfree(stty->port);
	kfree(stty);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver stty_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sttybt",
	},
	.probe = stty_probe,
	.remove = stty_remove,
};

static int __init stty_init(void)
{
	return platform_driver_register(&stty_driver);
}

static void __exit stty_exit(void)
{
	platform_driver_unregister(&stty_driver);
}

module_init(stty_init);
module_exit(stty_exit);

MODULE_AUTHOR("Dewu Jiang");
MODULE_DESCRIPTION("SIPC/stty driver");
MODULE_LICENSE("GPL");
