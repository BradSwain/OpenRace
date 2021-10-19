#include <stdio.h>
#include <stdlib.h>

int main() {
  int shared = 0;
#pragma omp parallel
  {
#pragma omp single
    {
#pragma omp task
      { shared = 1; }

#pragma omp taskwait
      printf("1 == %d\n", shared);
    }
  }

  return 0;
}
