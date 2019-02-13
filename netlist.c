// The following code is a derivative work of the code from Github user Robopol86  
// which is licensed GPLv2. This code therefore is also licensed under the terms 
// of the GNU Public License, verison 2.

// Resources:
// * https://raw.githubusercontent.com/Robpol86/libnl/master/example_c/scan_access_points.c

#include "nwns.h"

#define MAX_SSID 50

static struct netNode * headPtr = NULL;
static struct netNode * tailPtr = NULL;
static int numElements = 0;

//Adapted from scan_access_points.c main() line 268
void initList(char * wInt){

    struct netNode * netList = NULL;

    int if_index = if_nametoindex(wInt); // Use this wireless interface for scanning.

    printf("Scanning for networks...\n");
    // Open socket to kernel.
    struct nl_sock *socket = nl_socket_alloc();  // Allocate new netlink socket in memory.
    genl_connect(socket);  // Create file descriptor and bind socket.
    int driver_id = genl_ctrl_resolve(socket, "nl80211");  // Find the nl80211 driver ID.

    // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
    int err = do_scan_trigger(socket, if_index, driver_id, NULL);
    if (err != 0) {
        printf("do_scan_trigger() failed with %d.\n", err);
        return;
    }

    // Now get info for all SSIDs detected.
    struct nl_msg *msg = nlmsg_alloc();  // Allocate a message.
    genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);  // Setup which command to run.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);  // Add message attribute, which interface to use.

    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, callback_dump, &netList);  // Add the callback.
    int ret = nl_send_auto(socket, msg);  // Send the message.

    //printf("NL80211_CMD_GET_SCAN sent %d bytes to the kernel.\n", ret);
    ret = nl_recvmsgs_default(socket);  // Retrieve the kernel's answer. callback_dump() prints SSIDs to stdout.
    nlmsg_free(msg);
    if (ret < 0) {
        printf("ERROR: nl_recvmsgs_default() returned %d (%s).\n", ret, nl_geterror(-ret));
        return;
    }

    //Save the head of the netNode linked list
    headPtr = netList;
    struct netNode * temp = NULL;

    //Count the number of detected netwoks and find the last element (tailPtr)
    while(netList){
      temp = netList;
      netList = netList->next;
      numElements++;
    }
    tailPtr = temp;

}

//Returns the list node in the linked list
struct netNode * getNodeTail(){
  return tailPtr;
}

//Returns the first node in the linked list
struct netNode * getNodeHead(){
  return headPtr;
}

//Returns the number of nodes in the linked list (Number of detected networks)
int getNumElements(){
  return numElements;
}

//Converts frequency (MHz) to channel
int getChannel(int freq){

  if (freq == 2484)
    return 14;

  if (freq < 2484)
    return (freq - 2407) / 5;

  return freq/5 - 1000;

}
