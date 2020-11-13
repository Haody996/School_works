#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[]) {
  unsigned long int i;
  // checking there is a command line argument
  if (argc != 2) {
    printf("Usage: %s number\n", argv[0]);
    return 0;
  }
  // converting the command line argument to a unsigned long
  // and exit program if there is an error during the conversion.
  errno = 0;
  i = strtoul(argv[1], NULL, 0);
  if (errno != 0) {
    perror("Failed to convert number");
    return 0;
  }
  // write your solutions below this line
  //initializing variables
  int count=0;
  int c;
  //a array to store results of mod
  int arr[64];
  //make exception for the case of 0
  if (i==0)
  {
    printf("%lu", i);
  }
  else{
    while (i>0)
    {
      //algorithm to find reminder of 2 divided by the number and store the reminder into the array
      c = i % 2;
      i = i /2;
      arr[count]=c; 
      //store the count of digits
      count++;
    }
  }
  int a;
  //display the elemnts in the array backwards
  for (a=count-1;a>=0;a--)
    printf("%d", arr[a]);

  printf("\n");

  // write your solutions above this line
  return 0;
}
