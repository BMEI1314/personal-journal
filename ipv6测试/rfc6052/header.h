/*
 *   head.h
 *
 *  Created on: 2017-03-09
 *      Author: zhangqi
 */
#ifndef  HEADER_H
#define  HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
  //选择ipv6类型[32,40,48,56,64,96]特定网络前缀
extern short int ipv6_prefix;
void  ipv4toipv6(char* ipv4,char* ipv6);
void load_config();
#endif
