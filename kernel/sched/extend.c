#include <linux/kobject.h>
#include <linux/pagemap.h>
#include <linux/sysfs.h>
#include <linux/init.h>

#ifdef CONFIG_SYSFS
static ssize_t extend_sched_read(struct file *file, struct kobject *kobj,
				 struct bin_attribute *bin_attr,
				 char *buf, loff_t off, size_t len)
{
	static const char output[] = "Extend scheduling time slice\n";

	if (off >= sizeof(output))
		return 0;

	strscpy(buf, output + off, len);
	return min((ssize_t)len, (ssize_t)(sizeof(output) - off - 1));
}

static ssize_t extend_sched_write(struct file *file, struct kobject *kobj,
				  struct bin_attribute *bin_attr,
				  char *buf, loff_t off, size_t len)
{
	return -EINVAL;
}

static vm_fault_t extend_sched_mmap_fault(struct vm_fault *vmf)
{
	vm_fault_t ret = VM_FAULT_SIGBUS;

	/* Only has one page */
	if (vmf->pgoff || !current->extend_map)
		return ret;

	vmf->page = virt_to_page(current->extend_map);

	get_page(vmf->page);
	vmf->page->mapping = vmf->vma->vm_file->f_mapping;
	vmf->page->index   = vmf->pgoff;

	return 0;
}

static void extend_sched_mmap_open(struct vm_area_struct *vma)
{
	WARN_ON(!current->extend_map);
}

static const struct vm_operations_struct extend_sched_vmops = {
	.open		= extend_sched_mmap_open,
	.fault		= extend_sched_mmap_fault,
};

static int extend_sched_mmap(struct file *file, struct kobject *kobj,
			     struct bin_attribute *attr,
			     struct vm_area_struct *vma)
{
	if (current->extend_map)
		return -EBUSY;

	current->extend_map = page_to_virt(alloc_page(GFP_USER | __GFP_ZERO));
	if (!current->extend_map)
		return -ENOMEM;

	vm_flags_mod(vma, VM_DONTCOPY | VM_DONTDUMP | VM_MAYWRITE, 0);
	vma->vm_ops = &extend_sched_vmops;

	return 0;
}

static struct bin_attribute extend_sched_attr = {
	.attr = {
		.name = "extend_sched",
		.mode = 0777,
	},
	.read = &extend_sched_read,
	.write = &extend_sched_write,
	.mmap = &extend_sched_mmap,
};

static __init int extend_init(void)
{
	return sysfs_create_bin_file(kernel_kobj, &extend_sched_attr);
}
late_initcall(extend_init);
#endif
