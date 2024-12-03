#include <mpi.h>
int secret(int x) { return x * 42; }

int foo(int x) { return x * x; }

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
  } else if (rank == 1) {
    if (argc == 1) {
      void* vp = reinterpret_cast<void*>(&secret);
      int (*fptr)(int) = reinterpret_cast<int (*)(int)>(vp);

      int result = fptr(7);
    } else {
    }
  }

  MPI_Finalize();
  return 0;
}
