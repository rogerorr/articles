#include <pthread.h>
#include <stdio.h>
#include <iostream>

void *threadStart(void *)
{
  FILE *fp = fopen("/dev/null", "r");
  if (fp)
  {
    fclose(fp);
  }
  std::cout << "Thread done" << std::endl;
  return 0;
}

int main()
{
  pthread_t thread;
  pthread_create(&thread, 0, threadStart, 0);
  pthread_join(thread, 0);
  std::cout << "Exiting" << std::endl;
}
