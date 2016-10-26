#!/bin/sh  This line is only for editors: This file must be sourced
# This file is part of the quoter project and distributed under the MIT license
#
# Usage: quoter_pipe [-l|--long] [--] command [args]
#
# is similar to
#
# quoter_pipe=`command [args] | quoter --stdin [-l|--long]`
#
# but the exit status is nonzero also if command [args] failed.
# Moreover, the variables quoter_pipestatus and quoter_pipestatus1 are set
# to the exit status of "command ..." and "quoter ..." in the above pipe,
# respectively.

quoter_pipe() {
	if [ x"$1" = x'-l' ] || [ x"$1" = x'--long' ]
	then	quoter_pipe=l
		shift
	else	quoter_pipe=
	fi
	[ x"$1" != x'--' ] || shift
# \0 is required to distinguish a trailing \0 in normal output
	quoter_pipe=`{
"$@"
printf '\\0%s' $?
} | quoter -ic$quoter_pipe`
	quoter_pipestatus1=$?
	quoter_pipestatus=${quoter_pipe##*\ }
	quoter_pipe=${quoter_pipe%\ *}
	[ "$quoter_pipestatus1" -eq 0 ] && [ "$quoter_pipestatus" -eq 0 ] \
		|| return 1
}
