#include <memory.h>
#include <stdio.h>

#include <string>

bool is_space(char c)
{
	static const char* ws = " \t\n\r\f\v";
	return strchr(ws, c) != nullptr;
}

int main()
{
    std::string s;
    if(is_space('x'))
    {
     s += 'x';
    }
}

/*

void test()
{
 char buf[100];
 memset(buf,0, sizeof(buf));
 *(volatile char*)buf= *(volatile char*)buf;
}

int main()
{
 int i;
 int len=0;
//  test();
//  
  for (i=0;i<100;++i)
  {
   len += strlen("CERTIFICATE-----");
  }
  printf("%d",len);
  return 0;
}
*/
