#!/bin/bash
if [ "$#" -ne "3" ]; then
	echo "Usage: from_csv.sh INPUT OUTPUT FIELD" >&2
	exit 1
fi
tail -n+2 $1 | awk "BEGIN { FS=\",\" } { print \$${3}; }" >$2
