#ifndef __HELLO_WORLD_EMULATOR_H__
#define __HELLO_WORLD_EMULATOR_H__

/* debug flags */
#define DEBUG_OID		0


/* Hello World MIB OIDs */
#define SNMPTRAP_OID_BASE			1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0

/*
 * Magic number definitions.
 * These must be unique for each object implemented within a
 *  single mib module callback routine.
 *
 * Typically, these will be the last OID sub-component for
 *  each entry, or integers incrementing from 1.
 *  (which may well result in the same values anyway).
 *
 * Here, the second and third objects are form a 'sub-table' and
 *   the magic numbers are chosen to match these OID sub-components.
 * This is purely for programmer convenience.
 * All that really matters is that the numbers are unique.
 */
#define HWE_PROFILE			1
#define HWE_MANUFACTURER		2
#define HWE_CONTACT			3
#define HWE_MAX_ETHERNETPORTS		4
#define HWE_LINKSPEED_ETHERNETPORTS	6
#define HWE_MGMT_MACADDRESS		8
#define HWE_MGMT_IPADDRESS		10
#define HWE_MGMT_LINKSTATUS		12
#define HWE_SYSTEM_POWERDOWN		14
#define HWE_SYSTEM_RESET		15

#define HWE_AVERSION			16
#define HWE_KVERSION			17
#define HWE_BOXID			18
#define HWE_SERIALNUMBER		19

#define HWE_DEVICE_NAME			"Hello World SNMP Test Emulator"
#define HWE_DEVICE_MANUF		"Srinivas Reddy Hallimani"
#define HWE_DEVICE_CONTACT		"penguin.bsp@gmail.com"

#define HWE_DEVICE_PROFILE_STRING_SIZE	100
#define HWE_DEVICE_MANUF_STRING_SIZE	30
#define HWE_DEVICE_CONTACT_STRING_SIZE	30
#define HWE_APP_VERSION_STRING_SIZE	30
#define HWE_KERNEL_VERSION_STRING_SIZE	30
#define HWE_SERIALNUMBER_STRING_SIZE	30
#define HWE_BOXID_STRING_SIZE		30
#define HWE_MACADDRESS_STRING_SIZE	20
#define HWE_IPADDRESS_STRING_SIZE	16

/**
 * Callback function handler to handle the Hello World Emulator requests
 */
u_char *_emulator_VarMethod
(
    struct variable *vp,
    oid *name,
    size_t *length,
    int exact,
    size_t *var_len,
    WriteMethod **write_method
);

void init_helloworld_emulator(void);

#endif
