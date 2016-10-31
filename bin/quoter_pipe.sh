#!/usr/bin/env cat
# This file is part of quoter v3.0 and distributed under the MIT license
#
# Usage: quoter_pipe [quoter-options] [--] command [args]
#
# is similar to
#
# quoter_pipe=`command [args] | quoter -ic [quoter-options]`
#
# but the exit status is nonzero also if command [args] failed.
# Moreover, the variables quoter_pipestatus and quoter_pipestatus1 are set
# to the exit status of "command ..." and "quoter ..." in the above pipe,
# respectively.
#
# Currently, the following quoter-options are not admissible for quoter_pipe:
#
# --short (-s) --output (-o) --append (-a) --version (-V) --help (-h)
#
# quoter_pipe does not check whether only admissible options are used:
# In case of bad options, the behaviour of quoter_pipe is undefined.

quoter_pipe() {
quoter_pipe=
empty_last=false
while [ $# -gt 0 ]
do	case $1 in
	--)
		shift
		break;;
# Admissible options have no arguments and do not require quoting:
	-*)
		quoter_pipe=$quoter_pipe' '$1;;
	*)
		break;;
	esac
	case $1 in
	--empty-last)
		empty_last=:;;
	--short|--output|--append|--version|--help)
		echo "quoter_pipe: bad option $1" >&2
		exit 1;;
	--*)
		:;;
	-*[soaVhH\?]*)
		echo "quoter_pipe: bad option $1" >&2
		exit 1;;
	-*e*)
		empty_last=:;;
	esac
	shift
done
if $empty_last
then	quoter_pipe=`{
${1+"$@"}
printf '\\0x%s' $?
} | quoter -ic$quoter_pipe`
	quoter_pipestatus1=$?
	quoter_pipestatus=${quoter_pipe##*x}
	quoter_pipe=${quoter_pipe%x*}
else	quoter_pipe=`{
${1+"$@"}
printf \'$?
} | quoter -ic$quoter_pipe`
	quoter_pipestatus1=$?
	quoter_pipestatus=${quoter_pipe##*\\\'}
	quoter_pipe=${quoter_pipe%\\\'*}
fi
case $quoter_pipe in
*' ')
	quoter_pipe=${quoter_pipe%?};;
*' \
')
	quoter_pipe=${quoter_pipe%%???};;
esac
! $empty_last || [ x"$quoter_pipe" != x"''" ] || quoter_pipe=
[ "$quoter_pipestatus1" -eq 0 ] && [ "$quoter_pipestatus" -eq 0 ] || return 1
}
