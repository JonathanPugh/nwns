#ifndef __NWNS_H
#define __NWNS_H

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#define ETH_ALEN 6

//Struct for detected networks
struct netNode{

  char ssid[33];
  char mac[18];
  int freq;

  struct netNode * next;
  struct netNode * prev;
};

//Defined in nwns.c
int main(int argc, char *argv[]);
void resizeHandler(int sig);
void printList(void);

//Defined in netlist.c
void initList(char *);
struct netNode * getNodeHead(void);
struct netNode * getNodeTail(void);
int getNumElements(void);
int getChannel(int);

//Defined in netops.c
int if_nametoindex(const char []);
struct trigger_results;
struct handler_args;
int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group);
void mac_addr_n2a(char *mac_addr, const unsigned char *arg);
void get_ssid(unsigned char *ie, int ielen, char * arr);
int callback_dump(struct nl_msg *msg, void *arg);
int do_scan_trigger(struct nl_sock *socket, int if_index, int driver_id, void *arg);
void insertNode(struct netNode **head);

#endif
