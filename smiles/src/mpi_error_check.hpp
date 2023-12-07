#ifndef CHALLENGE_MPI_ERROR_CHECK_HDR
#define CHALLENGE_MPI_ERROR_CHECK_HDR

#include <ostream>

// the input parameter is the return code of an MPI function. This function
// will print on the target file the error message that it's associated with
// the return code
void print_mpi_error(const int return_code, std::ostream& output);

// the input parameter is the return code of an MPI function. If this value
// is not equal to MPI_SUCCESS, it will print on the standard error more
// information and halt the execution of the process raising SIGABRT
void exit_on_fail(const int return_code);

#endif  // CHALLENGE_MPI_ERROR_CHECK_HDR