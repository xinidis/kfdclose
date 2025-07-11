#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/pid.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Kernel module to close a file descriptor of a process");

static int pid = -1;
static int fd_to_close = -1;

module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Target process PID");

module_param(fd_to_close, int, 0);
MODULE_PARM_DESC(fd_to_close, "File descriptor to close");

static int __init kfdclose_init(void)
{
    struct task_struct *task;
    struct files_struct *files;
    struct fdtable *fdt;
    struct file *file;

    if (pid <= 0 || fd_to_close < 0) {
        pr_err("Invalid PID or FD\n");
        return -EINVAL;
    }

    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        pr_err("Could not find task with PID %d\n", pid);
        rcu_read_unlock();
        return -ESRCH;
    }

    task_lock(task);
    files = task->files;
    if (!files) {
        pr_err("No file descriptor table\n");
        task_unlock(task);
        rcu_read_unlock();
        return -EFAULT;
    }

    spin_lock(&files->file_lock);
    fdt = files_fdtable(files);

    if (fd_to_close >= fdt->max_fds) {
        pr_err("FD %d is out of range\n", fd_to_close);
        spin_unlock(&files->file_lock);
        task_unlock(task);
        rcu_read_unlock();
        return -EBADF;
    }

    file = fdt->fd[fd_to_close];
    if (file) {
        fdt->fd[fd_to_close] = NULL;
        filp_close(file, files);
        pr_info("Closed FD %d of PID %d\n", fd_to_close, pid);
    } else {
        pr_info("FD %d of PID %d was already NULL\n", fd_to_close, pid);
    }

    spin_unlock(&files->file_lock);
    task_unlock(task);
    rcu_read_unlock();

    return 0;
}

static void __exit kfdclose_exit(void)
{
    pr_info("FD closer module exited\n");
}

module_init(kfdclose_init);
module_exit(kfdclose_exit);
