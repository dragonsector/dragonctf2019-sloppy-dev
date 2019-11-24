#define pr_fmt(x) "sloppy: " x

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_DESCRIPTION("Sloppy Dev");
MODULE_AUTHOR("redfodr <redford@dragonsector.pl>");
MODULE_VERSION("1.0");

struct list_elem {
    char                key[8];
    char                value[256];
    struct list_head    list;
};

static DEFINE_MUTEX(list_mutex);
static LIST_HEAD(list_head);

struct sloppy_insert {
    char    key[8];
    char    value[256];
};

struct sloppy_get_first {
    char    key[8];
    char    value[256];
};

struct sloppy_delete {
    char    key[8];
};

#define SLOPPY_IOCTL_INSERT    _IOR('s', 1, struct sloppy_insert)
#define SLOPPY_IOCTL_GET_FIRST _IOW('s', 2, struct sloppy_get_first)
#define SLOPPY_IOCTL_DELETE    _IOR('s', 3, struct sloppy_delete)

int sloppy_open (struct inode * inode, struct file * file)
{
    int res = 0;
    return 0;
}

struct list_elem * get_by_key (char key[8])
{
    struct list_head * last = NULL;
    struct list_head * it = NULL;
    list_for_each(it, &list_head) {
        if(memcmp(container_of(it, struct list_elem, list)->key, key, 8) > 0)
            break;
        last = it;
    }
    if(last)
        return container_of(last, struct list_elem, list);
    else
        return NULL;
}

long sloppy_insert (struct file * file, void * arg_)
{
    long res = 0;
    struct sloppy_insert * arg = (struct sloppy_insert * ) arg_;
    bool dealloc = false;

    struct list_elem * new_elem = kmalloc(sizeof * new_elem, GFP_KERNEL);
    if(! new_elem)
        return -ENOMEM;

    memcpy(new_elem->key, arg->key, sizeof new_elem->key);
    memcpy(new_elem->value, arg->value, sizeof new_elem->value);

    res = mutex_lock_interruptible(&list_mutex);
    if(res < 0)
        goto fail;

    struct list_elem * insertion_point = get_by_key(new_elem->key);
    if(insertion_point)
    {
        list_add(&new_elem->list, &insertion_point->list);
    }
    else
    {
        list_add(&new_elem->list, &list_head);
    }

    mutex_unlock(&list_mutex);
    return 0;

fail:
    kfree(new_elem);
    return res;
}

long sloppy_get_first (struct file * file, void * arg_)
{
    struct sloppy_get_first * arg = (struct sloppy_get_first * ) arg_;
    long retval = 0;

    retval = mutex_lock_interruptible(&list_mutex);
    if(retval < 0)
        goto out;

    memset(arg, 0, sizeof sloppy_get_first);
    if(! list_empty(&list_head))
    {
        struct list_elem * first = list_first_entry(&list_head, struct list_elem, list);
        memcpy(arg->key, first->key, sizeof arg->key);
        memcpy(arg->value, first->value, sizeof arg->value);
    }

    mutex_unlock(&list_mutex);
out:
    return retval;
}

long sloppy_delete (struct file * file, void * arg_)
{
    long res = 0;
    struct sloppy_delete * arg = (struct sloppy_delete * ) arg_;
    bool dealloc = false;

    res = mutex_lock_interruptible(&list_mutex);
    if(res < 0)
        goto fail;

    struct list_elem * elem = get_by_key(arg->key);
    if(elem && !memcmp(elem->key, arg->key, sizeof elem->key))
    {
        list_del(&elem->list);
        dealloc = true;
    }
    else
    {
        res = -ENOENT;
    }

    mutex_unlock(&list_mutex);
    if(dealloc)
        kfree(elem);
fail:
    return res;
}

static long sloppy_ioctl (struct file * file, unsigned int cmd, unsigned long arg)
{
    char data[max(max(_IOC_SIZE(SLOPPY_IOCTL_INSERT),_IOC_SIZE(SLOPPY_IOCTL_GET_FIRST)),_IOC_SIZE(SLOPPY_IOCTL_DELETE))];
    long ( * handler)(struct file * filp, void * arg) = NULL;
    long res;

    switch (cmd)
    {
        case SLOPPY_IOCTL_INSERT:
            handler = sloppy_insert;
            break;
        case SLOPPY_IOCTL_GET_FIRST:
            handler = sloppy_get_first;
            break;
        case SLOPPY_IOCTL_DELETE:
            handler = sloppy_delete;
            break;
        defualt:
            return -ENOTTY;
    }

    if((cmd & IOC_OUT) && copy_from_user(data, (void __user * )arg, _IOC_SIZE(cmd)))
        return -EFAULT;

    res = handler(file, (void * )data);

    if(!res && (cmd & IOC_IN))
    {
        if(copy_to_user((void __user * )arg, data, _IOC_SIZE(cmd)))
            return -EFAULT;
    }

out:
    return res;
}

static const struct file_operations sloppy_fops = {
    .owner = THIS_MODULE,
    .open  = sloppy_open,
    .unlocked_ioctl    = sloppy_ioctl,
};

static struct miscdevice sloppy_dev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "sloppy",
    .fops   = &sloppy_fops,
    .mode   = 0666,
};

static int __init sloppy_init (void)
{
    int res;

    pr_info("sloppy_dev loaded\n");
    res = misc_register(&sloppy_dev);
    if(res) {
        pr_err("misc_register() failed\n");
        return res;
    }

out:
    return 0;
}

static void __exit sloppy_exit (void)
{
    misc_deregister(&sloppy_dev);
}

module_init(sloppy_init);
module_exit(sloppy_exit);
MODULE_LICENSE("GLP");
