#include "nat64_prefix_util.h"
#include <iostream>
#include <string.h>
using namespace std;
int main ()
{  
   string ipv4,ipv6;
 
  cout<<"input ipv4"<<endl;
  cin>>ipv4;
  ConvertV4toNat64V6(ipv4,ipv6);
  cout<<ipv6<<endl;
  return 0;
}
