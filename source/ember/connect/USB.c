#include <stdint.h>
#include <stddef.h>

#define USB_MAX_DEVICES 16
#define USB_MAX_ENDPOINTS 8
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define USB_CLASS_CODE 0x0C
#define USB_SUBCLASS_CODE 0x03

typedef enum {
    USB_CONTROLLER_NONE = 0,
    USB_CONTROLLER_UHCI = 1,
    USB_CONTROLLER_OHCI = 2,
    USB_CONTROLLER_EHCI = 3,
    USB_CONTROLLER_XHCI = 4
} usb_controller_type_t;

typedef enum {
    USB_DEVICE_STATE_DISCONNECTED = 0,
    USB_DEVICE_STATE_CONNECTED = 1,
    USB_DEVICE_STATE_ADDRESSED = 2,
    USB_DEVICE_STATE_CONFIGURED = 3
} usb_device_state_t;

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t base_address;
    usb_controller_type_t type;
    int present;
} usb_controller_t;

typedef struct {
    uint8_t address;
    uint8_t port;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t max_packet_size;
    usb_device_state_t state;
    int used;
} usb_device_t;

static usb_controller_t controllers[4];
static int controller_count = 0;

static usb_device_t devices[USB_MAX_DEVICES];
static int device_count = 0;

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (1U << 31) |
        ((uint32_t) bus << 16) |
        ((uint32_t) device << 11) |
        ((uint32_t) function << 8) |
        (offset & 0xFC)
    );
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t pci_read_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint16_t)(pci_config_read(bus, device, function, 0x00) & 0xFFFF);
}

static uint16_t pci_read_device_id(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint16_t)((pci_config_read(bus, device, function, 0x00) >> 16) & 0xFFFF);
}

static uint8_t pci_read_class(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint8_t)((pci_config_read(bus, device, function, 0x08) >> 24) & 0xFF);
}

static uint8_t pci_read_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint8_t)((pci_config_read(bus, device, function, 0x08) >> 16) & 0xFF);
}

static uint8_t pci_read_prog_if(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint8_t)((pci_config_read(bus, device, function, 0x08) >> 8) & 0xFF);
}

static uint32_t pci_read_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {
    return pci_config_read(bus, device, function, (uint8_t)(0x10 + bar_index * 4));
}

static usb_controller_type_t detect_controller_type(uint8_t prog_if) {
    switch (prog_if) {
        case 0x00: return USB_CONTROLLER_UHCI;
        case 0x10: return USB_CONTROLLER_OHCI;
        case 0x20: return USB_CONTROLLER_EHCI;
        case 0x30: return USB_CONTROLLER_XHCI;
        default: return USB_CONTROLLER_NONE;
    }
}

void usb_init(void) {
    controller_count = 0;
    device_count = 0;

    for (int i = 0; i < 4; i++) {
        controllers[i].present = 0;
    }

    for (int i = 0; i < USB_MAX_DEVICES; i++) {
        devices[i].used = 0;
        devices[i].state = USB_DEVICE_STATE_DISCONNECTED;
    }
}

int usb_scan_pci(void) {
    int found = 0;

    for (uint16_t bus = 0; bus < 256 && controller_count < 4; bus++) {
        for (uint8_t device = 0; device < 32 && controller_count < 4; device++) {
            for (uint8_t function = 0; function < 8 && controller_count < 4; function++) {
                uint16_t vendor = pci_read_vendor_id((uint8_t) bus, device, function);
                if (vendor == 0xFFFF) {
                    continue;
                }

                uint8_t class_code = pci_read_class((uint8_t) bus, device, function);
                uint8_t subclass_code = pci_read_subclass((uint8_t) bus, device, function);

                if (class_code != USB_CLASS_CODE || subclass_code != USB_SUBCLASS_CODE) {
                    continue;
                }

                uint8_t prog_if = pci_read_prog_if((uint8_t) bus, device, function);
                usb_controller_type_t type = detect_controller_type(prog_if);

                if (type == USB_CONTROLLER_NONE) {
                    continue;
                }

                usb_controller_t* controller = &controllers[controller_count];
                controller->bus = (uint8_t) bus;
                controller->device = device;
                controller->function = function;
                controller->vendor_id = vendor;
                controller->device_id = pci_read_device_id((uint8_t) bus, device, function);
                controller->base_address = pci_read_bar((uint8_t) bus, device, function, 4) & 0xFFFFFFFC;
                controller->type = type;
                controller->present = 1;

                controller_count++;
                found++;
            }
        }
    }

    return found;
}

int usb_get_controller_count(void) {
    return controller_count;
}

const usb_controller_t* usb_get_controller(int index) {
    if (index < 0 || index >= controller_count) {
        return NULL;
    }
    return &controllers[index];
}

static int usb_find_free_device_slot(void) {
    for (int i = 0; i < USB_MAX_DEVICES; i++) {
        if (!devices[i].used) {
            return i;
        }
    }
    return -1;
}

int usb_register_device(uint8_t port, uint16_t vendor_id, uint16_t product_id, uint8_t device_class) {
    int slot = usb_find_free_device_slot();
    if (slot < 0) {
        return -1;
    }

    devices[slot].address = (uint8_t)(slot + 1);
    devices[slot].port = port;
    devices[slot].vendor_id = vendor_id;
    devices[slot].product_id = product_id;
    devices[slot].device_class = device_class;
    devices[slot].max_packet_size = 8;
    devices[slot].state = USB_DEVICE_STATE_CONNECTED;
    devices[slot].used = 1;

    device_count++;
    return slot;
}

int usb_remove_device(int index) {
    if (index < 0 || index >= USB_MAX_DEVICES || !devices[index].used) {
        return -1;
    }

    devices[index].used = 0;
    devices[index].state = USB_DEVICE_STATE_DISCONNECTED;
    device_count--;

    return 0;
}

const usb_device_t* usb_get_device(int index) {
    if (index < 0 || index >= USB_MAX_DEVICES || !devices[index].used) {
        return NULL;
    }
    return &devices[index];
}

int usb_get_device_count(void) {
    return device_count;
}

int usb_set_device_state(int index, usb_device_state_t state) {
    if (index < 0 || index >= USB_MAX_DEVICES || !devices[index].used) {
        return -1;
    }
    devices[index].state = state;
    return 0;
}
