#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <mpi.h>

#include "mpi_error_check.hpp"

void exit_on_fail(const int return_code) {
  if (return_code != MPI_SUCCESS) {
    print_mpi_error(return_code, std::cerr);
    throw std::runtime_error("MPI call failed");
  }
}

void print_mpi_error(const int return_code, std::ostream& output) {
  assert(return_code != MPI_SUCCESS);

  // get the error class (which should be standard across the MPI implementations)
  int error_class = MPI_ERR_UNKNOWN;
  const int rc_class = ::MPI_Error_class(return_code, &error_class);
  if (rc_class != MPI_SUCCESS) {
    error_class = MPI_ERR_UNKNOWN;
  }

  // get a string that describes the error
  auto error_explanation = std::string{"Explanation not available"};
  char explanation_c_str[MPI_MAX_ERROR_STRING];
  int explanation_len = 0;
  const int rc_string = ::MPI_Error_string(return_code, explanation_c_str, &explanation_len);
  if (rc_string == MPI_SUCCESS) {
    assert(explanation_len < MPI_MAX_ERROR_STRING);
    explanation_c_str[explanation_len] = '\0';  // to be sure to have a valid c-string
    error_explanation = std::string{explanation_c_str};
  }

  // print the error (as a single string)
  std::ostringstream error_message;
  error_message << "MPI error " << error_class << ": \"" << error_explanation << "\"" << std::endl;
  output << error_message.str() << std::flush;
}