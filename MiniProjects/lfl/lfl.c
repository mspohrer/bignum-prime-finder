//Tacy Bechtel and Matthew Spohrer

//Lock free logging

#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("%s", "Usage: ./lfl [child count]\n");
    return -1;
  }
  int childCount = atoi(argv[1]);
  printf("%d\n", childCount);

  return 0;
}

