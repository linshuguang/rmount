#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x89fac617, "struct_module" },
	{ 0xa292685b, "blk_init_queue" },
	{ 0x106e84a8, "alloc_disk" },
	{ 0x8b1fa63b, "blk_cleanup_queue" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x89b301d4, "param_get_int" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xc7552a84, "malloc_sizes" },
	{ 0xa45d08e3, "sock_release" },
	{ 0x20000329, "simple_strtoul" },
	{ 0xc8be7b15, "_spin_lock" },
	{ 0x51ccf6eb, "sock_recvmsg" },
	{ 0x4e830a3e, "strnicmp" },
	{ 0x2fd1d81c, "vfree" },
	{ 0x98bd6f46, "param_set_int" },
	{ 0x1b7d4074, "printk" },
	{ 0x2189e32b, "paravirt_ops" },
	{ 0x546f0b06, "_spin_lock_irq" },
	{ 0xe3534054, "del_gendisk" },
	{ 0xdcad9011, "sock_sendmsg" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x71a50dbc, "register_blkdev" },
	{ 0xd79b5a02, "allow_signal" },
	{ 0xeac1c4af, "unregister_blkdev" },
	{ 0x38eb528a, "blk_queue_hardsect_size" },
	{ 0xa3bc2e15, "kmem_cache_alloc" },
	{ 0x7c496698, "elv_next_request" },
	{ 0x4292364c, "schedule" },
	{ 0x17d59d01, "schedule_timeout" },
	{ 0xf93f6169, "put_disk" },
	{ 0x14090044, "force_sig" },
	{ 0x8e4c5880, "wake_up_process" },
	{ 0xffd3c7, "init_waitqueue_head" },
	{ 0xbaadbd11, "__wake_up" },
	{ 0xd2965f6f, "kthread_should_stop" },
	{ 0x37a0cba, "kfree" },
	{ 0x4ce9efa9, "kthread_create" },
	{ 0x7c9049bf, "prepare_to_wait" },
	{ 0x76e9fa3b, "add_disk" },
	{ 0xba32cdfc, "end_request" },
	{ 0xee2d147e, "sock_create" },
	{ 0x83cbeea8, "set_user_nice" },
	{ 0x5e1389a, "finish_wait" },
	{ 0x25da070, "snprintf" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "65EF262C469A7DF997AB8CB");
