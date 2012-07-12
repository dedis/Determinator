/*
 * ACPI definitions
 * partial implementation, just for finding LAPIC and IOAPIC
 */

#ifndef PIOS_DEV_ACPI_H
#define PIOS_DEV_ACPI_H
#ifndef PIOS_KERNEL
# error "This is a kernel header; user programs should not #include it"
#endif

struct acpi_rsdp {
	uint8_t signature[8];		// "RSD PTR "
	uint8_t checksum;		// checksum for first 20 bytes
	uint8_t oemid[6];
	uint8_t reserved;
	uint32_t rsdtaddr;		// 32bit address for RSDT
} gcc_packed;

struct acpi2_rsdp {			// RSDP structure
	uint8_t signature[8];		// "RSD PTR "
	uint8_t checksum;		// checksum for first 20 bytes
	uint8_t oemid[6];
	uint8_t revision;
	uint32_t rsdtaddr;		// 32bit address for RSDT
	uint32_t length;		// length of entire table
	uint64_t xsdtaddr;		// 64bit address for XSDT
	uint8_t extchecksum;		// checksum for entire table
	uint8_t reserved[3];
} gcc_packed;

struct acpi_sdt_hdr {			// SDT header
	uint8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;		// checksum for entire table
	uint8_t oemid[6];
	uint64_t oemtid;
	uint32_t oemrev;
	uint32_t creatorid;
	uint32_t creatorrev;
} gcc_packed;

struct acpi_rsdt {
	struct acpi_sdt_hdr header;	// 'RSDT'
	uint32_t ent[0];		// 32bit address entries
} gcc_packed;

struct acpi2_xsdt {
	struct acpi_sdt_hdr header;	// 'XSDT'
	uint64_t ent[0];		// 64bit address entries
} gcc_packed;

struct acpi_madt {
	struct acpi_sdt_hdr header;	// 'APIC'
	uint32_t lapicaddr;		// 32bit local interrupt controller address
	uint32_t flag;
	uint8_t ent[0];			// vary-length interrupt controller structures
} gcc_packed;

#define PCAT_COMPAT 0x0001		// PC-AT-compatible dual-8259 setup

#define MADT_LAPIC	0x00
#define MADT_IOAPIC	0x01

struct acpi_madt_lapic {
	uint8_t type;			// MADT_LAPIC
	uint8_t length;
	uint8_t pid;			// APIC processor id
	uint8_t lapicid;		// LAPIC id
	uint32_t flag;
} gcc_packed;

#define MADT_LAPIC_ENABLE	0x0001

struct acpi_madt_ioapic {
	uint8_t type;			// MADT_IOAPIC
	uint8_t length;
	uint8_t ioapicid;		// IOAPIC id
	uint8_t reserved;
	uint32_t ioapicaddr;		// 32bit IOAPIC address
	uint32_t gsibase;		// 32bit global system interrupt base
} gcc_packed;

void acpi_init(void);

#endif // !PIOS_DEV_ACPI_H
