#include <stdio.h>
#include <stdlib.h>

int main() {
  int shared = 0;

#pragma omp parallel
  {
#pragma omp single
    {
#pragma omp task  // This task spawns two more nested tasks accessign the same data
      {
#pragma omp task
        { shared = 1; }

#pragma omp task
        { shared = 2; }
      }

#pragma omp taskwait  // Both the nested tasks have not joined yet
      printf("shared == %d\n", shared);
    }
  }

  return 0;
}
