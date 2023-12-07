#!/bin/bash

#########################################################################
## Input validation (do not change this part)
#########################################################################
function print_usage {
  >&2 echo ""
  >&2 echo "USAGE: /path/to/launch.sh /path/to/main /path/to/input.smi /path/to/output.csv N"
  >&2 echo ""
  >&2 echo "N stands for the parallelism level"
  >&2 echo ""
  >&2 echo "Example:"
  >&2 echo "./scripts/launch.sh ./build/main ./data/molecules.smi output.csv 1"
}
if [ "$#" -ne "4" ]; then
  >&2 echo "Error: expecting 4 parameters, got $#"
  print_usage
  exit -1
fi
application_filepath="$1"
input_filepath="$2"
output_filepath="$3"
parallelism_level="$4"
for required_file in "$application_filepath" "$input_filepath"; do
  if [ ! -s "$required_file" ]; then
    >&2 echo "Error: file \"$required_file\" is empty or non-existent"
    print_usage
    exit -2
  fi
done
if ! [[ $parallelism_level =~ ^[0-9]+$ ]] ; then
  >&2 echo "Error: the parallelism level \"$parallelism_level\" is not an integer"
  exit -3
fi
#########################################################################
## Change the file below this mark
#########################################################################
# You can use the following variable:
# - application_filepath -> with the path to the executable
# - input_filepath       -> with the path to the input file
# - output_filepath      -> with the path to the output file
# - parallelism_level    -> the expected level of parallelism
#                           - the number of threads in OpenMP
#                           - the number of processes in MPI
#########################################################################


# launch the application (using MPI)
mpirun -np "$parallelism_level" "$application_filepath" < "$input_filepath" > "$output_filepath"