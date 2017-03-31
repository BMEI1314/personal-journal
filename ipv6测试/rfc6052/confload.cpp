/*
 *   confload.cpp
 *
 *  Created on: 2017-03-09
 *      Author: zhangqi
 */
#include "header.h"

void load_config(void){
  int i,k,tmp=0;
  char string_buffer[16];  //16个字节长,用来储存ipv4地址
  char ipv6_address[40];    // IPv6地址
  int linecount=1;         //配置文件行数
  char line[255];          //包含配置文件一行的字符串
  char tmpline[255];       //用来储存评论
 
/************打开配置文件，并一行行取文件************/
FILE *fp=fopen ("setting.conf","r");
if(fp!=NULL)  {
    for(;fgets(line,sizeof line,fp)!=NULL;linecount++)  {
/*************判断是否是注释[#与//表示]*************/
      if (strlen(line) < 3 || line[0] == '#' || (line[0] == '/' && line[1] == '/')) {
          while ((strlen(line) == 254) && line[254] != 10) {
               if ( fgets( line, sizeof line, fp) == NULL) { 
		     return;
			                                     }
					                   }
               continue;
   }
 /***********不是注释*******************/                                                   
 	if ((strlen(line) == 254) && line[254] != 10) {
	    strcpy(tmpline,line);
		while((strlen(line) == 254 && line[254] != 10)) {
		   if ( fgets( line, sizeof line, fp) == NULL) {
		      return;
		                 			}
				                     }
		  strcpy(line,tmpline);
	}
/******************开始读取参数********************/
        i=0;
        tmp=0;
	while ( line[i] == ' ' || line[i] == 9 ) i++; //跳过空格与tab； 
        if ( !strncmp(line+i, "ipv6_prefix", strlen("ipv6_prefix")) ) {
	     i += strlen("ipv6_prefix");
           while ( line[i] == ' ' || line[i] == 9 ) i++;
	if(line[i]=='3'&&line[i+1]=='2')
		ipv6_prefix=32;
        else if(line[i]=='4'&&line[i+1]=='0')
               ipv6_prefix=40;
        else if(line[i]=='4'&&line[i+1]=='8')
               ipv6_prefix=48;
        else if(line[i]=='5'&&line[i+1]=='6')
               ipv6_prefix=56;
        else if(line[i]=='6'&&line[i+1]=='4')
               ipv6_prefix=64;
        else if(line[i]=='9'&&line[i+1]=='6')
               ipv6_prefix=96;
        else  
           printf("ipv6_prefix is  illegal!!!!\n");
        
	}

/*******************************************************************************/
}      
}
fclose(fp);
return ;
}

