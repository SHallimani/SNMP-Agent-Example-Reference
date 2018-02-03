#include <signal.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <time.h>
#include <pthread.h>
#include <sys/utsname.h>

#include "hello_world_emu.h"

/*
 * This array defines the OID of the top of the mib tree that we're
 *  registering underneath.
 * Note that this needs to be the correct size for the OID being 
 *  registered, so that the length of the OID can be calculated.
 *  The format given here is the simplest way to achieve this.
 */
oid _emulator_variables_oid[] =     { 1, 3, 6, 1, 4, 1, 8073};

/* global variables for storing SNMP parameters */
char __hwe_profile_info[HWE_DEVICE_PROFILE_STRING_SIZE];
char __hwe_manuf_info[HWE_DEVICE_MANUF_STRING_SIZE];
char __hwe_contact_info[HWE_DEVICE_CONTACT_STRING_SIZE];
char __hwe_app_version[HWE_APP_VERSION_STRING_SIZE];
char __hwe_kernel_version[HWE_KERNEL_VERSION_STRING_SIZE];
char __hwe_serialnumber[HWE_SERIALNUMBER_STRING_SIZE];
char __hwe_boxid[HWE_BOXID_STRING_SIZE];
char __hwe_max_ethports[5];
char __hwe_mgmt_mac_address[HWE_MACADDRESS_STRING_SIZE];
char __hwe_mgmt_ip_address[HWE_IPADDRESS_STRING_SIZE];

/*********************
 *
 *  Initialisation & common implementation functions
 * This array structure defines a representation of the
 *  MIB being implemented.
 *
 * The type of the array is 'struct variableN', where N is
 *  large enough to contain the longest OID sub-component
 *  being loaded.  This will normally be the maximum value
 *  of the fifth field in each line.  In this case, the second
 *  and third entries are both of size 2, so we're using
 *  'struct variable2'
 *
 * The supported values for N are listed in <agent/var_struct.h>
 *  If the value you need is not listed there, simply use the
 *  next largest that is.
 *
 * The format of each line is as follows
 *  (using the first entry as an example):
 *      1: EXAMPLESTRING:
 *          The magic number defined in the example header file.
 *          This is passed to the callback routine and is used
 *            to determine which object is being queried.
 *      2: ASN_OCTET_STR:
 *          The type of the object.
 *          Valid types are listed in <snmp_impl.h>
 *      3: RONLY (or RWRITE):
 *          Whether this object can be SET or not.
 *      4: var_example:
 *          The callback routine, used when the object is queried.
 *          This will usually be the same for all objects in a module
 *            and is typically defined later in this file.
 *      5: 1:
 *          The length of the OID sub-component (the next field)
 *      6: {1}:
 *          The OID sub-components of this entry.
 *          In other words, the bits of the full OID that differ
 *            between the various entries of this array.
 *          This value is appended to the common prefix (defined later)
 *            to obtain the full OID of each entry.
 */

struct variable2 helloworld_emu_variables[] = {
	{HWE_PROFILE, 			ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_PROFILE}},
	{HWE_MANUFACTURER,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_MANUFACTURER}},
	{HWE_CONTACT,			ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_CONTACT}},
	{HWE_MAX_ETHERNETPORTS,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_MAX_ETHERNETPORTS}},
	{HWE_LINKSPEED_ETHERNETPORTS,	ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_LINKSPEED_ETHERNETPORTS}},
	{HWE_MGMT_MACADDRESS,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_MGMT_MACADDRESS}},
	{HWE_MGMT_IPADDRESS,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_MGMT_IPADDRESS}},
	{HWE_MGMT_LINKSTATUS,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_MGMT_LINKSTATUS}},
	{HWE_SYSTEM_POWERDOWN,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_SYSTEM_POWERDOWN}},
	{HWE_SYSTEM_RESET,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_SYSTEM_RESET}},
	{HWE_AVERSION,			ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_AVERSION}},
	{HWE_KVERSION,			ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_KVERSION}},
	{HWE_BOXID,			ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_BOXID}},
	{HWE_SERIALNUMBER,		ASN_OCTET_STR, RONLY, _emulator_VarMethod, 1, {HWE_SERIALNUMBER}},
};

static void mac_address(char *eth, char *mac_buffer)
{
	int skfd;
	struct ifreq buffer;

	skfd = socket(PF_INET, SOCK_DGRAM, 0);

	memset(&buffer, 0x00, sizeof(buffer));

	strcpy(buffer.ifr_name, eth);
	ioctl(skfd, SIOCGIFHWADDR, &buffer);
	sprintf(mac_buffer, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X \0",
			(unsigned char)buffer.ifr_hwaddr.sa_data[0],
			(unsigned char)buffer.ifr_hwaddr.sa_data[1],
			(unsigned char)buffer.ifr_hwaddr.sa_data[2],
			(unsigned char)buffer.ifr_hwaddr.sa_data[3],
			(unsigned char)buffer.ifr_hwaddr.sa_data[4],
			(unsigned char)buffer.ifr_hwaddr.sa_data[5]);

	close(skfd);
}

void ip_address(char *eth, char *ip_buff)
{
	int skfd;
	struct ifreq ifr;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&ifr, 0x00, sizeof(ifr));

	strcpy(ifr.ifr_name, eth);
	ioctl(skfd, SIOCGIFADDR, &ifr);
	close(skfd);
	sprintf(ip_buff, "%s\0", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

void kversion() {
#if 0
    if (uname (&version_buff) == -1) 
#endif
}

int link_status(char *ethport)
{
    struct ifreq ifr;
    struct ethtool_value edata;
    int skfd;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0 ) ) < 0)
    {
        return -1;
    }

    edata.cmd = ETHTOOL_GLINK;

    strncpy(ifr.ifr_name, ethport, sizeof(ifr.ifr_name)-1);
    ifr.ifr_data = (char *) &edata;

    if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
    {
        close(skfd);
        return -1;
    }
    close(skfd);

    return (edata.data ? 1 : 0);
}

/*********************
 *
 *  System specific implementation functions
 *
 *********************/

/*
 * Define the callback function used in the example_variables structure.
 * This is called whenever an incoming request refers to an object
 *  within this sub-tree.
 *
 * Four of the parameters are used to pass information in.
 * These are:
 *    vp      The entry from the 'example_variables' array for the
 *             object being queried.
 *    name    The OID from the request.
 *    length  The length of this OID.
 *    exact   A flag to indicate whether this is an 'exact' request
 *             (GET/SET) or an 'inexact' one (GETNEXT/GETBULK).
 *
 * Four of the parameters are used to pass information back out.
 * These are:
 *    name     The OID being returned.
 *    length   The length of this OID.
 *    var_len  The length of the answer being returned.
 *    write_method   A pointer to the SET function for this object.
 *
 * Note that name & length serve a dual purpose in both roles.
 */
u_char *_emulator_VarMethod (
		struct variable *vp,
		oid *name,
		size_t *length,
		int exact,
		size_t *var_len,
		WriteMethod ** write_method)
{
	int oid_len = *length;
	int status = 0;
	int sm_buf_size = 0;
	struct board_sts* Board_Sts;
	int tr_mode = 0, enc = 0;
	int tunnel_fd = 0;
	int rt = -1;

	/*
	 * Before returning an answer, we need to check that the request
	 *  refers to a valid instance of this object.  The utility routine
	 *  'header_generic' can be used to do this for scalar objects.
	 *
	 * This routine 'header_simple_table' does the same thing for "simple"
	 *  tables. (See the AGENT.txt file for the definition of a simple table).
	 *
	 * Both these utility routines also set up default values for the
	 *  return arguments (assuming the check succeeded).
	 * The name and length are set suitably for the current object,
	 *  var_len assumes that the result is an integer of some form,
	 *  and write_method assumes that the object cannot be set.
	 *
	 * If these assumptions are correct, this callback routine simply
	 * needs to return a pointer to the appropriate value (using 'long_ret').
	 * Otherwise, 'var_len' and/or 'write_method' should be set suitably.
	 */
	DEBUGMSGTL(("hwe", "_hwe VarMethod entered\n"));

	if (header_generic(vp, name, length, exact, var_len, write_method) ==
			MATCH_FAILED)
		return NULL;

	/*
	 * Many object will need to obtain data from the operating system in
	 *  order to return the appropriate value.  Typically, this is done
	 *  here - immediately following the 'header' call, and before the
	 *  switch statement. This is particularly appropriate if a single 
	 *  interface call can return data for all the objects supported.
	 *
	 * This example module does not rely on external data, so no such
	 *  calls are needed in this case.  
	 */

	/*
	 * Now use the magic number from the variable pointer 'vp' to
	 *  select the particular object being queried.
	 * In each case, one of the static objects is set up with the
	 *  appropriate information, and returned mapped to a 'u_char *'
	 */

	switch(vp->magic)
	{

		/* get the device name */
		case HWE_PROFILE:
			{
				strncpy(__hwe_profile_info, HWE_DEVICE_NAME, strlen(HWE_DEVICE_NAME));
				*var_len = strlen(HWE_DEVICE_NAME);
				return (u_char *)__hwe_profile_info;
			}
			break;
			/*get the manufacture */
		case HWE_MANUFACTURER:
			{
				strncpy(__hwe_manuf_info, HWE_DEVICE_MANUF, strlen(HWE_DEVICE_MANUF));
				*var_len = strlen(HWE_DEVICE_MANUF);
				return (u_char *)__hwe_manuf_info;
			}
			break;
			/*get the description*/
		case HWE_CONTACT:
			{
				strncpy(__hwe_contact_info, HWE_DEVICE_CONTACT, strlen(HWE_DEVICE_CONTACT));
				*var_len = strlen(HWE_DEVICE_CONTACT);
				return (u_char *)__hwe_contact_info;
			}
			break;

			/*get the forward version */
		case HWE_MAX_ETHERNETPORTS:
			{
				memset(__hwe_max_ethports, 0, sizeof(__hwe_max_ethports));
				strcpy(__hwe_max_ethports, "8");
				*var_len = strlen(__hwe_max_ethports);
				return (u_char *) __hwe_max_ethports;
				break;
			}
			break;
			/*get the error instruction  */
		case HWE_LINKSPEED_ETHERNETPORTS:
			{
				memset(__hwe_max_ethports, 0, sizeof(__hwe_max_ethports));
				strcpy(__hwe_max_ethports, "2");
				*var_len = strlen(__hwe_max_ethports);
				return (u_char *) __hwe_max_ethports;
				break;
			}                                                                                                    
			/* get the compress status */
		case HWE_MGMT_MACADDRESS:
			{

				memset(__hwe_mgmt_mac_address, 0, sizeof(__hwe_mgmt_mac_address));
				mac_address("eth0", __hwe_mgmt_mac_address);
				*var_len = strlen(__hwe_mgmt_mac_address);
				return (u_char *) __hwe_mgmt_mac_address;
			}
			break;
			/*get the port A status***/
		case HWE_MGMT_IPADDRESS:
			{
				memset(__hwe_mgmt_ip_address, 0, sizeof(__hwe_mgmt_ip_address));
				ip_address("eth0", __hwe_mgmt_ip_address);
				*var_len = strlen(__hwe_mgmt_ip_address);
				return (u_char *) __hwe_mgmt_ip_address;
			}
			break;
			/*get port A MAC address****/
		case HWE_MGMT_LINKSTATUS:
			{
				memset(__hwe_mgmt_mac_address, 0, sizeof(__hwe_mgmt_mac_address));
				if(link_status("eth0"))
				{
					strcpy(__hwe_mgmt_mac_address, "mgmt1: Link up");
					*var_len = strlen(__hwe_mgmt_mac_address);
				}
				else
				{
					strcpy(__hwe_mgmt_mac_address, "mgmt1: Link down");
					*var_len = strlen(__hwe_mgmt_mac_address);
				}
				return (u_char *) __hwe_mgmt_mac_address;
			}
			break;
		case HWE_SYSTEM_POWERDOWN:
			/**
			 * System Going for Down
			 */
			system("poweroff");
			break;
		case HWE_SYSTEM_RESET:
			/**
			 * System Going for power cycle the unit
			 */
			system("reboot");
			break;
		case HWE_AVERSION:
			strncpy(__hwe_app_version, "0.7.6", strlen("0.7.6"));
			*var_len = strlen("0.7.6");
			return (u_char *)__hwe_app_version;
			break;
		case HWE_KVERSION:
			strncpy(__hwe_kernel_version, "Linux 4.4.0-101-generic", strlen("Linux 4.4.0-101-generic"));
			*var_len = strlen("Linux 4.4.0-101-generic");
			return (u_char *)__hwe_kernel_version;
			break;
		case HWE_BOXID:
			strncpy(__hwe_boxid, "hwe-10g", strlen("hwe-10g"));
			*var_len = strlen("hwe-10g");
			return (u_char *) __hwe_boxid;
			break;
		case HWE_SERIALNUMBER:
			strcpy(__hwe_serialnumber, "0123456789");
			return (u_char *) __hwe_serialnumber;
			break;
			break;
		default:
			DEBUGMSGTL(("Hello World Emulator", "Unknown Item. magic# %d.\n", vp->magic));
			break;
	}
	return NULL;
}

void init_helloworld_emulator(void)
{
    int status = 0;

    /*
     * Register ourselves with the agent to handle our mib tree.
     * The arguments are:
     *    descr:   A short description of the mib group being loaded.
     *    var:     The variable structure to load.
     *                  (the name of the variable structure defined above)
     *    vartype: The type of this variable structure
     *    theoid:  The OID pointer this MIB is being registered underneath.
     */
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("                      Hello World SNMP Test Emulator\n");
    printf("                      Hello World SNMP Test Emulator\n");
    printf("                      Hello World SNMP Test Emulator\n");
    printf("                      Hello World SNMP Test Emulator\n");
    printf("                      Hello World SNMP Test Emulator\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    printf("********************************************************************************\n");
    REGISTER_MIB("helloworld-emulator", helloworld_emu_variables, variable2, _emulator_variables_oid);
}
