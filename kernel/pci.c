#include "u.h"

typedef struct pci_bridge_conf {
  u32 base_address_register[2];

  u8 secondary_latency_timer;
  u8 subordinate_bus_number;
  u8 secondary_bus_number;
  u8 primary_bus_number;

  u16 secondary_status;
  u8 io_limit;
  u8 io_base;
  
  u16 memory_limit;
  u16 memory_base;

  // the next four values should probably be merged.
  u16 prefetchable_memory_limit;
  u16 prefetchable_memory_base;

  u16 prefetchable_base_upper;
  u16 prefetchable_limit_upper;
  // and those with io_limit, io_base
  u8 io_limit_upper;
  u8 io_base_upper;
  
  u8  capability_ptr;
  
  u32 expansion_rom_address;
  u16 bridge_control;
  u8  interrupt_pin;
  u8  interrupt_line;
} pci_bridge_conf_t;

typedef struct pci_dev_conf {
  u32 base_address_register[6];

  u32 cardbus_cis_ptr;

  u16 subsystem;
  u16 subsystem_vendor;

  u32 expansion_rom_base_address;

  u8  capability_ptr;
  
  u8 max_latency;
  u8 min_grant;
  u8 interrupt_pin;
  u8 interrupt_line;
} pci_dev_conf_t;


typedef struct pci_conf {
  u16 device_id;
  u16 vendor_id;

  u16 status_reg;
  u16 command_reg;

  u8 class_code;
  u8 subclass;
  u8 prog_if;
  u8 revision_id;
  // BIST : Build in self test.
  // BIST Capable | Start BIST | Reserved Completion | Code
  // Bit  7       | 6          | 5-4                 | 3-0
  u8 bist_register;
  u8 header_type;
  u8 latency_timer;
  u8 cache_line_size;
  union {
    pci_bridge_conf_t bridge;
    pci_dev_conf_t dev;
  };
} pci_conf_t;




//  address dd 10000000000000000000000000000000b
//        E    Res    Bus    Dev  F  Reg   0
// Bits
// 31		Enable bit = set to 1
// 30 - 24	Reserved = set to 0
// 23 - 16	Bus number = 256 options
// 15 - 11	Device/Slot number = 32 options
// 10 - 8	Function number = will leave at 0 (8 options)
// 7 - 2        Register number = will leave at 0 (64 options) 64 x 4 bytes = 256 bytes worth of accessible registers
// 1 - 0	Set to 0
void
pci_write(u32 bus, u32 slot, u32 func, u32 reg, u32 data) {
  u32 addr = (1 << 31)
    | (bus << 16)
    | (slot << 11)
    | (func << 8)
    | reg;
  outl(pci_address_ioport, addr);
  outl(pci_data_ioport, data);
}

u32
pci_read(u32 bus, u32 slot, u32 func, u32 reg) {
  u32 addr = 0x80000000
    | (bus << 16)
    | (slot << 11)
    | (func << 8)
    | reg;
  outl(pci_address_ioport, addr);
  return inl(pci_data_ioport);
}


pci_conf_t
pci_conf_read(u8 bus, u8 slot){
  pci_conf_t conf;
  u8 func = 0;
  u32 val = pci_read(bus, slot, func, 0x00);
  conf.vendor_id = val & 0xffff;
  conf.device_id = val >> 16;
  if(conf.vendor_id == 0xffff){
    return conf;
  }
  val = pci_read(bus, slot, func, 0x04);
  conf.command_reg = val & 0xffff;
  conf.status_reg = val >> 16;
  val = pci_read(bus, slot, func, 0x08);
  conf.revision_id = val & 0xff;
  conf.prog_if = (val >> 8) & 0xff;
  conf.subclass = (val >> 16) & 0xff;
  conf.class_code = val >> 24;
  val = pci_read(bus, slot, func, 0x0C);
  conf.cache_line_size = val & 0xff;
  conf.latency_timer = (val >> 8) & 0xff;
  conf.header_type = (val >> 16) & 0xff;
  conf.bist_register = val >> 24;
  switch(conf.header_type) {
  case 0x00:
    // standard header type:
    {
    u8 reg = 0x10;
    for(uint i = 0; i<6; i++, reg += 4) {
      conf.dev.base_address_register[i] = pci_read(bus, slot, func, reg);
    }
    conf.dev.cardbus_cis_ptr = pci_read(bus, slot, func, 0x28);
    val = pci_read(bus, slot, func, 0x2c);
    conf.dev.subsystem_vendor = val & 0xffff;
    conf.dev.subsystem = val >> 16;
    conf.dev.expansion_rom_base_address = pci_read(bus, slot, func, 0x30);
    conf.dev.capability_ptr = pci_read(bus, slot, func, 0x34) & 0xff;
    val = pci_read(bus, slot, func, 0x30);
    conf.dev.interrupt_line = val & 0xff;
    conf.dev.interrupt_pin = (val >> 8) & 0xff;
    conf.dev.min_grant = (val >> 16) & 0xff;
    conf.dev.max_latency = val >> 24;
    };
    break;
  case 0x01:
    {
      /* PCI-PCI bridge */
    
    };
    break;
  case 0x08:
    {
      /* multifunction device */
      for (func = 1; func < 8; func++) {
	
      }
    }
    break;

  }
  
  return conf;
}

void
pci_scan()
{
  for(u8 bus = 0; bus < 255; bus++) {
    for(u8 dev = 0; dev < 32; dev++) {
      pci_conf_t conf = pci_conf_read(bus, dev);
      if((conf.device_id == 0xff) || (conf.vendor_id == 0xff)) {
	continue;
      }
      cprintf("pci: bus: %x, device id: %x, vendor id: %d\n",
	      bus, conf.device_id, conf.vendor_id);
      cprintf("class: %d\n", conf.class_code);
    }
  }
}
