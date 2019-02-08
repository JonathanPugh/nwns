// The following code is a derivative work of the code from Github user Robopol86  
// which is licensed GPLv2. This code therefore is also licensed under the terms 
// of the GNU Public License, verison 2.

// Resources:
// * http://git.kernel.org/cgit/linux/kernel/git/jberg/iw.git/tree/iw.c
// * http://git.kernel.org/cgit/linux/kernel/git/jberg/iw.git/tree/event.c
// * http://git.kernel.org/cgit/linux/kernel/git/jberg/iw.git/tree/genl.c
// * http://git.kernel.org/cgit/linux/kernel/git/jberg/iw.git/tree/util.c
// * https://raw.githubusercontent.com/Robpol86/libnl/master/example_c/scan_access_points.c

#include <errno.h>

#include "nwns.h"

int isprint(int);
int if_nametoindex(const char []);


struct trigger_results {
    int done;
    int aborted;
};

//genl.c line 30
struct handler_args {  // For family_handler() and nl_get_multicast_id().
    const char *group;
    int id;
};

//iw.c line 279
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg)
{       
        struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
        int len = nlh->nlmsg_len;
        struct nlattr *attrs;
        struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
        int *ret = arg;
        int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);
        
        *ret = err->error;
        
        if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
                return NL_STOP;
        
        if (!(nlh->nlmsg_flags & NLM_F_CAPPED)) 
                ack_len += err->msg.nlmsg_len - sizeof(*nlh);
        
        if (len <= ack_len)
                return NL_STOP;
        
        attrs = (void *)((unsigned char *)nlh + ack_len);
        len -= ack_len;
        
        nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL);
        if (tb[NLMSGERR_ATTR_MSG]) {
                len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
                              nla_len(tb[NLMSGERR_ATTR_MSG]));
                fprintf(stderr, "kernel reports: %*s\n", len,
                        (char *)nla_data(tb[NLMSGERR_ATTR_MSG]));
        }
        
        return NL_STOP;
}

//iw.c line 314
static int finish_handler(struct nl_msg *msg, void *arg) {
    // Callback for NL_CB_FINISH.
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

//iw.c line 321
static int ack_handler(struct nl_msg *msg, void *arg) {
    // Callback for NL_CB_ACK.
    int *ret = arg;
    *ret = 0;
    return NL_STOP;
}

//event.c line 8
static int no_seq_check(struct nl_msg *msg, void *arg) {
    // Callback for NL_CB_SEQ_CHECK.
    return NL_OK;
}

//genl.c line 35
static int family_handler(struct nl_msg *msg, void *arg)
{
        struct handler_args *grp = arg;
        struct nlattr *tb[CTRL_ATTR_MAX + 1];
        struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
        struct nlattr *mcgrp;
        int rem_mcgrp;

        nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
                  genlmsg_attrlen(gnlh, 0), NULL);

        if (!tb[CTRL_ATTR_MCAST_GROUPS])
                return NL_SKIP;

        nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
                struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

                nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
                          nla_data(mcgrp), nla_len(mcgrp), NULL);

                if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
                    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
                        continue;
                if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
                            grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
                        continue;
                grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
                break;
        }

        return NL_SKIP;
}

//genl.c line 68
int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
        struct nl_msg *msg;
        struct nl_cb *cb;
        int ret, ctrlid;
        struct handler_args grp = {
                .group = group,
                .id = -ENOENT,
        };

        msg = nlmsg_alloc();
        if (!msg)
                return -ENOMEM;

        cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb) {
                ret = -ENOMEM;
                goto out_fail_cb;
        }

        ctrlid = genl_ctrl_resolve(sock, "nlctrl");

        genlmsg_put(msg, 0, 0, ctrlid, 0,
                    0, CTRL_CMD_GETFAMILY, 0);

        ret = -ENOBUFS;
        NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

        ret = nl_send_auto_complete(sock, msg);
        if (ret < 0)
                goto out;

        ret = 1;

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

        while (ret > 0)
                nl_recvmsgs(sock, cb);

        if (ret == 0)
                ret = grp.id;
 nla_put_failure:
 out:
        nl_cb_put(cb);
 out_fail_cb:
        nlmsg_free(msg);
        return ret;
}

//util.c line 8
void mac_addr_n2a(char *mac_addr, const unsigned char *arg)
{
        int i, l;

        l = 0;
        for (i = 0; i < ETH_ALEN ; i++) {
                if (i == 0) {
                        sprintf(mac_addr+l, "%02x", arg[i]);
                        l += 2;
                } else {
                        sprintf(mac_addr+l, ":%02x", arg[i]);
                        l += 3;
                }
        }
}

//Adapted from print_ssid (scan_access_points.c line 107)
void get_ssid(unsigned char *ie, int ielen, char * arr) {
    uint8_t len;
    uint8_t *data;
    int i;

    while (ielen >= 2 && ielen >= ie[1]) {
        if (ie[0] == 0 && ie[1] >= 0 && ie[1] <= 32) {
            len = ie[1];
            data = ie + 2;
            for (i = 0; i < len; i++) {
                if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\'){
                        arr[i] = data[i];
                }
                else if (data[i] == ' ' && (i != 0 && i != len -1))
                        arr[i] = ' ';
                else
                arr[i + 1] = '\0';
            }
            break;
        }
        ielen -= ie[1] + 2;
        ie += ie[1] + 2;
    }
}

//Adapted from scan_access_points.c line 129
static int callback_trigger(struct nl_msg *msg, void *arg) {
    // Called by the kernel when the scan is done or has been aborted.
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct trigger_results *results = arg;

    if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED) {
        printf("Got NL80211_CMD_SCAN_ABORTED.\n");
        results->done = 1;
        results->aborted = 1;
    } else if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS) {
        results->done = 1;
        results->aborted = 0;
    }  // else probably an uninteresting multicast message.

    return NL_SKIP;
}

//Adapted from scan_access_points.c line 152
int callback_dump(struct nl_msg *msg, void *arg) {
    // Called by the kernel with a dump of the successful scan's data. Called for each SSID.
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    char mac_addr[20];
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_TSF] = { .type = NLA_U64 },
        [NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
        [NL80211_BSS_BSSID] = { },
        [NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
        [NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
        [NL80211_BSS_INFORMATION_ELEMENTS] = { },
        [NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
        [NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
        [NL80211_BSS_STATUS] = { .type = NLA_U32 },
        [NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
        [NL80211_BSS_BEACON_IES] = { },
    };

    // Parse and error check.
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[NL80211_ATTR_BSS]) {
        printf("bss info missing!\n");
        return NL_SKIP;
    }
    if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
        printf("failed to parse nested attributes!\n");
        return NL_SKIP;
    }
    if (!bss[NL80211_BSS_BSSID]) return NL_SKIP;
    if (!bss[NL80211_BSS_INFORMATION_ELEMENTS]) return NL_SKIP;

    insertNode((struct netNode**)arg);

    mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_BSSID]));
    strcpy((*(struct netNode**)arg)->mac,mac_addr);
    (*(struct netNode**)arg)->freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);

    get_ssid(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
	     nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
             (*(struct netNode**)arg)->ssid);

    return NL_SKIP;
}

//Adapted from scan_access_points.c line 196
int do_scan_trigger(struct nl_sock *socket, int if_index, int driver_id, void *arg){
    // Starts the scan and waits for it to finish. Does not return until the scan is done or has been aborted.
    struct trigger_results results = { .done = 0, .aborted = 0 };
    struct nl_msg *msg;
    struct nl_cb *cb;
    struct nl_msg *ssids_to_scan;
    int err;
    int ret;
    int mcid = nl_get_multicast_id(socket, "nl80211", "scan");
    nl_socket_add_membership(socket, mcid);  // Without this, callback_trigger() won't be called.

    // Allocate the messages and callback handler.
    msg = nlmsg_alloc();
    if (!msg) {
        printf("ERROR: Failed to allocate netlink message for msg.\n");
        return -ENOMEM;
    }
    ssids_to_scan = nlmsg_alloc();
    if (!ssids_to_scan) {
        printf("ERROR: Failed to allocate netlink message for ssids_to_scan.\n");
        nlmsg_free(msg);
        return -ENOMEM;
    }
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        printf("ERROR: Failed to allocate netlink callbacks.\n");
        nlmsg_free(msg);
        nlmsg_free(ssids_to_scan);
        return -ENOMEM;
    }

    // Setup the messages and callback handler.
    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);  // Setup which command to run.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);  // Add message attribute, which interface to use.
    nla_put(ssids_to_scan, 1, 0, "");  // Scan all SSIDs.
    nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids_to_scan);  // Add message attribute, which SSIDs to scan for.

    nlmsg_free(ssids_to_scan);  // Copied to `msg` above, no longer need this.
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger, &results);  // Add the callback.
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);  // No sequence checking for multicast messages.

    // Send NL80211_CMD_TRIGGER_SCAN to start the scan. The kernel may reply with NL80211_CMD_NEW_SCAN_RESULTS on
    // success or NL80211_CMD_SCAN_ABORTED if another scan was started by another process.
    err = 1;
    ret = nl_send_auto(socket, msg);  // Send the message.

    while (err > 0) ret = nl_recvmsgs(socket, cb);  // First wait for ack_handler(). This helps with basic errors.
    if (err < 0) {
        printf("WARNING: err has a value of %d.\n", err);
    }
    if (ret < 0) {
        printf("ERROR: nl_recvmsgs() returned %d (%s).\n", ret, nl_geterror(-ret));
        return ret;
    }
    while (!results.done) nl_recvmsgs(socket, cb);  // Now wait until the scan is done or aborted.
    if (results.aborted) {
        printf("ERROR: Kernel aborted scan.\n");
        return 1;
    }

    // Cleanup.
    nlmsg_free(msg);
    nl_cb_put(cb);
    nl_socket_drop_membership(socket, mcid);  // No longer need this.
    return 0;
}

//Inserts a new node into the netNode linked list
void insertNode(struct netNode **head){
    struct netNode* newHead = (struct netNode*)malloc(sizeof(struct netNode));

    if(*head)
      (*head)->prev = newHead;

    //Set ssid in new node to null terminating characters
    for(int i = 0; i < 32; i++)
      newHead->ssid[i] = '\0';

    newHead->prev = NULL;
    newHead->next = *head;
    
    *head = newHead;
}

