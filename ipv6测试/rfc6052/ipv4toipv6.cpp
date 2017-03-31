/*
 *   ipv4toipv6.cpp
 *
 *  Created on: 2017-03-09
 *      Author: zhangqi
 */

#include "header.h"
 short int ipv6_prefix; 
 char ipv4_addr[16]={0}; 
 char ipv6tran[40] = {0};

void   ipv4toipv6(char* ipv4,char* ipv6)
{      struct in6_addr checkedipv6,pointer;
       struct in_addr ipv4ptr;
       const char *ipv6_addr="2001:67c:27e4:642::"; // /96的前缀
	int i;
        inet_pton(AF_INET, ipv4, &ipv4ptr);
	if (inet_pton(AF_INET6, ipv6_addr,&checkedipv6))
   {        

		if ( ipv6_prefix == 96 ) 
             {    
                 for (i = 12;i<=15; i++) checkedipv6.s6_addr[i]=0x0;
		     memcpy(pointer.s6_addr, checkedipv6.s6_addr, 12);
	             pointer.s6_addr[12] = ipv4ptr.s_addr&0xff;
	             pointer.s6_addr[13] = ipv4ptr.s_addr>>8&0xff;	
		     pointer.s6_addr[14] = ipv4ptr.s_addr>>16&0xff;
		     pointer.s6_addr[15] = ipv4ptr.s_addr>>24&0xff;
		       // printf("%d\n",ipv4ptr.s_addr>>8&0xff);
             }  
		else if ( ipv6_prefix == 64 )
            {
                 for (i = 8; i<=15; i++) checkedipv6.s6_addr[i]=0x0;
		 memcpy(pointer.s6_addr, checkedipv6.s6_addr, 16);
		pointer.s6_addr[8] = 0x0;
		pointer.s6_addr[9] = ipv4ptr.s_addr&0xff;
		pointer.s6_addr[10] = ipv4ptr.s_addr>>8&0xff;
		pointer.s6_addr[11] = ipv4ptr.s_addr>>16&0xff;
		pointer.s6_addr[12] = ipv4ptr.s_addr>>24&0xff;
            }
                else if ( ipv6_prefix == 56 ) 
            {
                  for (i = 7; i<=15; i++) checkedipv6.s6_addr[i]=0x0;
		memcpy(pointer.s6_addr, checkedipv6.s6_addr, 16);
		pointer.s6_addr[7] = ipv4ptr.s_addr&0xff;
		pointer.s6_addr[8] = 0x0;
		pointer.s6_addr[9] = ipv4ptr.s_addr>>8&0xff;
		pointer.s6_addr[10] = ipv4ptr.s_addr>>16&0xff;
		pointer.s6_addr[11] = ipv4ptr.s_addr>>24&0xff;
            }

                 else if ( ipv6_prefix == 48 ) 
             {
                for (i = 6; i<=15; i++) checkedipv6.s6_addr[i]=0x0;
		 memcpy(pointer.s6_addr, checkedipv6.s6_addr, 16);
		pointer.s6_addr[6] = ipv4ptr.s_addr&0xff;
		pointer.s6_addr[7] = ipv4ptr.s_addr>>8&0xff;
		pointer.s6_addr[8] = 0x0;
		pointer.s6_addr[9] = ipv4ptr.s_addr>>16&0xff;
		pointer.s6_addr[10] = ipv4ptr.s_addr>>24&0xff;
             }
                 else if ( ipv6_prefix == 40 ) 
             {  
                for (i = 5; i<=15; i++) checkedipv6.s6_addr[i]=0x0;
               memcpy(pointer.s6_addr, checkedipv6.s6_addr, 16);
		pointer.s6_addr[5] = ipv4ptr.s_addr&0xff;
		pointer.s6_addr[6] = ipv4ptr.s_addr>>8&0xff;
		pointer.s6_addr[7] = ipv4ptr.s_addr>>16&0xff;
		pointer.s6_addr[8] = 0x0;
		pointer.s6_addr[9] = ipv4ptr.s_addr>>24&0xff;
              }
		else if ( ipv6_prefix == 32 ) 
            {
             for (i = 4; i<=15; i++) checkedipv6.s6_addr[i]=0x0;
              memcpy(pointer.s6_addr, checkedipv6.s6_addr, 16);
		pointer.s6_addr[4] = ipv4ptr.s_addr&0xff;
		pointer.s6_addr[5] = ipv4ptr.s_addr>>8&0xff;
		pointer.s6_addr[6] = ipv4ptr.s_addr>>16&0xff;
		pointer.s6_addr[7] = ipv4ptr.s_addr>>24&0xff;
		pointer.s6_addr[8] = 0x0;
             }
                else { printf("Config ERROR: Invalid IPv6 prefix found in configuration file\n");exit(-1);}
                  
               inet_ntop(AF_INET6,&pointer, ipv6,40);
               printf("inet_ntop6: %s\n", ipv6);
	}

}

int main ()
{   
     load_config();
     printf("input the ipv4 address\n");
     scanf("%s",&ipv4_addr);
     ipv4toipv6(ipv4_addr,ipv6tran);          
     return 0;
}

