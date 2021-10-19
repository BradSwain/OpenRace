#include <stdio.h>
#include <stdlib.h>

int main() {  // Thread 0
  int A = 0;
  int B = 0;
#pragma omp parallel  // Threads 1, 4
  {
#pragma omp single
    {
#pragma omp task  // Task 1 (Thread 2)
      {
#pragma omp task  // Task 2 (Thread 3)
        { B = 1; }
        A = 1;
      }

#pragma omp taskwait  // Task 1 is joined here, but not Task 2
      printf("%d == %d\n", A, B);
    }
  }

  return 0;
}
