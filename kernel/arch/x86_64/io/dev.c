#include <io/pci.h>

#include <devmgr.h>
#include <vsnprintf.h>
#include <string.h>

static void fill_pci_devices(void) {
	size_t n_devs = pci_get_n_devices();
	pci_dev_t *devs = pci_get_devices();
	if (!n_devs) return;

	devmgr_dev_t dev;

	handle_t hnd = devmgr_create_bus("pci");
	for (size_t i = 0; i < n_devs; i++) {
		memset(&dev, 0, sizeof(dev));
		dev.bus = hnd;
		char buf[16];
		snprintf(buf, 12, "pci%01x.%02x.%02x", 
				devs[i].desc.bus, devs[i].desc.dev, 
				devs[i].desc.fun);
		dev.name = buf;
		dev.ident = buf;
		dev.bus_vendor = devs[i].vendor;
		dev.bus_device = devs[i].device;
		dev.bus_device_type = devs[i].base_class;
		dev.bus_device_subtype = devs[i].sub_class;

		devmgr_io_reg_t regs[6];
		memset(regs, 0, sizeof(*regs) * 6);
		for (int j = 0; j < 6; j++) {
			if (!devs[i].bar[j].enabled) continue;

			regs[j].valid = 1;
			regs[j].base_addr = devs[i].bar[j].addr;
			regs[j].len = devs[i].bar[j].length;
			regs[j].type = devs[i].bar[j].type & BAR_IO ? DEV_IO_REG_PORT : DEV_IO_REG_MMIO;
		}

		dev.n_io_regions = 6;
		dev.io_regions = regs;

		devmgr_create_dev(hnd, &dev);
	}
}

void arch_devmgr_fill_devices(void) {
	fill_pci_devices();
}
