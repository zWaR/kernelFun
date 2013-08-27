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
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xc5a107fb, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xc677f48, "__register_chrdev" },
	{ 0x5e09ca75, "complete" },
	{ 0x83636ee3, "wait_for_completion" },
	{ 0xb72397d5, "printk" },
	{ 0x87b087b5, "current_task" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

