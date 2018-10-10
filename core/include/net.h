/******************************************************************************
  文 件 名   : net.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : 网络接口
  函数列表   :
  修改历史   :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 创建文件
******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#pragma once

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#define SOCKET_INVALID          -1
#define SOCKET_CREATE_FAILED    -2
#define SOCKET_BIND_FAILED      -3
#define SOCKET_LISTEN_FAILED    -4

class CNet
{
public: //mothed
    static int init_unix_addr(struct sockaddr_un *addr, const char *path);
    static int unix_bind (const char* path, int backlog,int flag = 0);
    static int unix_connect (int* netfd, const char* path, int block = 1, int* out_errno = NULL);
    static int init_udp_unix(const char* szPath, int bFlag);
    static int wait_udp_unix(int sock_fd, int nTimeout);
    static int tcp_bind(const char *addr, uint16_t port, int backlog);
    static int udp_bind(const char *addr, uint16_t port, int rbufsz = 0, int wbufsz = 0);
    static int tcp_connect(int* netfd, const char* address, int port,int block);
	static int tcp_create_netfd(int* netfd, const char* address, int port,int block);
	static int tcp_connect_with_netfd(int netfd, const char* address, int port,int block);
};

