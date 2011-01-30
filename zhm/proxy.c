/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendList function.
 *
 *   Created by:    Karl Ramm
 *
 *   Copyright 2010 by Karl Ramm andthe Massachusetts Institute of Technology.
 *   For copying and distribution information, see the file
 *   "mit-copyright.h".
 *
 */

#include "zhm.h"

#include <netdb.h>

extern int proxyfd;

void
zproxy(int cfd, struct sockaddr *addr, socklen_t addr_len) {
    Code_t ret;
    char addrstr[128];
    char portstr[128];
    ZNotice_t z;
    fd_set fds;
    int zfd;
    char packet[Z_MAXPKTLEN + 512 /*XXX I have my suspicions */];
    unsigned int len;
    struct sockaddr_in my_sockaddr; /*XXX6*/

    ret = getnameinfo(addr, addr_len,
                      addrstr, sizeof(addrstr),
                      portstr, sizeof(portstr),
                      NI_NUMERICHOST|NI_NUMERICSERV);

    if (ret < 0)
        syslog(LOG_ERR, "zproxy: connection from unrepresentable address");
    else
        syslog(LOG_INFO, "zproxy: connection from %s:%s", addrstr, portstr);

    ret = fork();

    if (ret < 0)
        syslog(LOG_ERR, "zproxy: fork: %m");
    if (ret > 0)
        return;

    close(proxyfd); /* close our copy of the proxy server port */
    ret = ZOpenPort(NULL); /* open a new client port */
    if (ret) {
        syslog(LOG_ERR, "zproxy: ZOpenPort: %s", error_message(ret));
        exit(1);
    }

    Z_SetTCPFD(cfd);

    memset(&z, 0, sizeof(z));

    z.z_kind = UNSAFE;
    /* this should end up on the other end with our address data */
    ret = ZSrvSendNotice(&z, NULL, Z_TCPXmitFragment);
    if (ret) {
        syslog(LOG_ERR, "zproxy: initial message: ZSrvSendList: %s",
               error_message(ret));
        exit(1);
    }

    zfd = ZGetFD();

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(zfd, &fds);
        FD_SET(cfd, &fds);

        ret = select(MAX(zfd,cfd) + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            syslog(LOG_ERR, "zproxy: select: %m");
            break;
        }

        /* this should not do reassembly, but Z_GetUDP will ack the packet */

        if (FD_ISSET(zfd, &fds)) {
            /* Factor something out of Z_ReadWait: */
            ret = Z_GetUDP((ZPacket_t *)&packet, &len);
            if (ret)
                syslog(LOG_ERR, "Z_GetUDP: %s", error_message(ret));
            else if (len != 0) {
                ret = Z_SendTCP(packet, len);
                if (ret)
                    break;  /* not worth logging? */
            }
        }

        if (FD_ISSET(cfd, &fds)) {
            /* read packet(s) from the pipe */
            ret = Z_GetTCP(&packet, &len);
            if (ret)
                break; /* not worth logging? */
            ret = Z_SendUDP(packet, len);
            if (ret)
                syslog(LOG_ERR, "Z_SendUDP: %s", error_message(ret));
        }
    }
    /* XXX Rethink error handling once Z_{Send,Read}{UDP,TCP} have been written */
    ZCancelSubscriptions(__Zephyr_port);

    exit(0);
}
