# quoter

quote arguments or standard input for usage in POSIX shell by eval

Author: Martin Väth (martin at mvath.de)

This project is under the MIT license.
SPDX-License-Identifier: MIT

This project serves two purposes:

1. Quote arguments for eval or remote usage in POSIX shells:
`quote --` is similar to `printf '%q '` supported by some shells or
external printf implementations. Note, however, that `%q` is not POSIX,
and so one cannot rely that this works on all systems.

2. Allow to deal in POSIX shell with standard input from tools which output
strings separated by \0 (like GNU find -print0, but one can also achieve a
similar output with POSIX find, see below)):
"quote -i" is somewhat similar to "xargs -0 printf '%q '" except that
neither the option -0 nor the %q are POSIX, and so one cannot rely that this
works on all systems.

The above points are best explained by some POSIX shell script snippets:

1.
 - (i)
   ```
	su -c "$(quoter -c -- cat -- "$@")"
   ```
   The effect of the above snippet is similar to
   ```
	su -c "cat -- $@"
   ```
   except that the latter cannot be used if the arguments might contain
   a special character. In fact, if `$1` has e.g. the value
   `/dev/null; rm -rf *` then the bad code `cat -- /dev/null; rm -rf *`
   would be executed, because the whole argument is interpreted by the
   remote shell.
   Note that the `--` after `quoter` in this example is mandatory to make sure
   that quoter will not interpret any furter options.
 - (ii)
   ```
	var=$var${var:+\ }`quoter -c -- ...`
   ```
   The above is the analogue of
   ```
	Push var ...
   ```
   where Push is the function from https://github.com/vaeth/push/
   Note that calling the external program quoter has quite some overhead, so
   this might be slower than Push. On the other hand, it becomes faster
   the longer and more complex the pushed data ```...``` is, because the
   compiledcode in __quoter__ will usually do the actual quoting considerably
   faster.

The above example 1(ii) still suffers from a problem:
The data which can be passed to an external program like __quoter__ is usually
limited by system restrictions. No such limitation exists for standard input
and output, that is, when `quoter -i` is used.
For this reason, it is usually advisable to prefer `quoter -i` over the "plain"
usage of `quoter`. This is simple if `printf` is a shell builtin which thus
does not suffer from the mentioned system restrictions.
In this case, one can simply replace `quoter -- ...` by
```
	printf '%s\0' ... | quoter -i
```

Summarizing: A variant of the above example without the mentioned limitations
(if printf is a built-in) reads as follows:
```
	var=$var${var:+\ }`printf '%s\0' ... | quoter -ic`
```

2.
  ```
	eval "set -- `find . -type f -print0 | quoter -i`"
	for file
	do ...
	done
  ```
  For the case that the find utility knows the GNU extension -print0,
  this will (recursively) iterate through all ordinary files from the current
  directory or some of its subdirectories. In contrast to the "naive" approach
  ```
	for file in `find . -type f`
	do ...
	done
  ```
  the above has no problem with special characters (like spaces) in filenames.

  Note that POSIX find instead of GNU extensions can be used: Just replace
  `-print0` within the command line by the more lengthy
  ```
  find -exec printf '%s\0' '{}' '+'
  ```
  (usually the `\` has to be quoted once more (`\\`), e.g. in the above
  content).

The example 2. above still has two problems:

- (a) If `find` returns nothing, then the above `eval` will expand to `set --`;
some (buggy) shells will not remove all arguments by this command.
Workaround: Add artificially an argument and remove it, that is, use instead
of the above `eval` for instance
```
eval "set -- a `find . -type f -print0 | quoter -i`"
shift
```
- (b) It is hard to check whether the call to "find" within the above "eval"
was succesful, because POSIX returns only the exit status of the last
command of a pipe, and neither -o pipefail nor the PIPESTATUS array are POSIX.


For this reason, this project also provides a shell function `quoter_pipe`
To define this shell function with __quoter-3.0__ or newer,
one can `eval` the output of the program `quoter_pipe.sh`.
For instance, to check whether quoter_pipe.sh is installed and to define it
if it is, one can use something like
```
if SOME_VARIABLE=`quoter_pipe.sh 2>/dev/null`
then	eval "$SOME_VARIABLE"
else	echo "quoter_pipe.sh is not installed" >&2
fi
```

__Remark__: An obsoleted method was to use instead
```
. quoter_pipe.sh
```
The latter works for older versions of __quoter__ or if one installs manually,
but unless an appropriate `PATH` before sourcing is set, it fails when
`quoter_pipe.sh` is replaced by a wrapper script which happens with the
provided Makefile. Moreover, if `quoter_pipe.sh` is not available,
it stops the script.

After the `quoter_pipe` function is defined, it can be used as follows.
The call
```
	quoter_pipe [quoter_options] [--] command [args]
```
is similar to
```
	quoter_pipe=`command [args] | quoter -ic [quoter_options]`
```
except that the exit status is 0 only if both commands of the pipe succeeded.
In addition, the variables `quoter_pipestatus` and `quoter_pipestatus1`
contain the exit status of the first and second command of the above pipe,
respectively.
(See the comments on top of the output of `quoter_pipe.sh` for more details,
e.g. which `quoter_options` are admissible.)
To do its task, `quoter_pipe` uses implementation details of `quoter` (e.g. in
which way expressions actually are quoted). These implementation details might
change in future versions of `quoter`. Therefore, `quoter_pipe` will in general
only work with the version of `quoter` it is distributed with.
For this reason, one should make sure to have always matching versions of
`quoter` and `quoter_pipe.sh` installed. For the same reason, it is not
recommended to write further scripts which rely on such implementation details.

Summarizing, a variant of the above example which solves the problems (a)
and (b) and works with POSIX `find` can look like this:
```
	# first source quoter_pipe.sh from $PATH if necessary:
	command -v quoter_pipe >/dev/null 2>&1 || . quoter_pipe.sh
	quoter_pipe find . -type f -exec printf '%s\0' '{}' '+' || {
		[ "$pipestatus" -eq 0 ] || echo "find failed" >&2
		[ "$pipestatus1" -eq 0 ] || echo "quote failed" >&2
		exit 1
	}
	eval "set -- a $quoter_pipe"
	shift
	for file
	do ...
	done
```

It is __not__ possible to specify redirection or several commands (e.g.
separated by `;` or `&&` or something similar) in the argument of
`quoter_pipe`. If this should be needed, a function can be used:
```
	my_pipe_task() {
		some_command && another_command >/dev/null 2>&1
	}
	quoter_pipe my_pipe_task
```

The call `quoter -h` describes further options of `quoter`.
Some options need a more verbose explanation:

Roughly speaking, quoter is intended to be used such that
```
	eval "set -- `quoter ...`"
```
can always be safely executed, i.e. all possibly "disturbing" characters
are quoted or escaped. Certain non-POSIX extensions of some shells might
require quoting further characters.
For instance, `{a,b}` is by some shells interpreted as `a` `b`, so -
although not necessary according to POSIX - the symbol `{` will need to be
escaped for such a shell.
Any quoting which the author is aware of the currently existing popular
shell versions (bash, dash, zsh, ksh, busybox, bosh, Bourne shell) is
taken care of, and if some quoting is missing, this will likely be fixed
in a future release of __quoter__.
This assertion holds with and without the options `-s` and `-l`, but with
the option `-s`, it is attempted to reduce the quoting while with `-l` a lot
of (usually redundant) quoting is used.
This means that with the option `-s`, the generated output is usually shorter
(though usually less readable by humans) while with `-l` the generated string
is usually longer (and less readable by humans) due to unnecessary quoting.

The default `-S` (`--unshort`) is a compromise which is readable and safe
(for all current or obsolete shells).
Usage of `-s` is only recommended if there is a serious reason to
get a string which is as short as possible, e.g. if memory is an issue.
Usage of `-l` is only recommended if paranoid future-safety is desired for
future nonstandard shell extension not existing yet, e.g. when in some
shell the symbol `%` should some day have a special meaning on the command
line.
Usage of `-s` and `-l` both are discouraged if a human readable output
is desired.

The meaning of the option `--empty-last` (`-e`) is perhaps not obvious.
If standard input looks like this
```
	string1\0string2\0string3
```
`quoter -i` will consider the three strings `string1` `string2` `string3`
However, what should happen if `string3` is empty in this case?
It is perhaps most logical that in this case `string3` is considered to be
an empty string. This was indeed the case for __quoter v1.0__ and
__quoter v1.1__.

However, this turned out to be not very practical:
Most commands like `find ... -print0` or `printf '%s\0' ...` generate actually
a redundant `\0` symbol at the end, so that the above interpretation of the
standard input would append a non-intended empty string as a last argument
which is very disturbing and not very practical.
For this reason, the above is no longer the default for __quoter v2.0__
or newer: A trailing `\0` is no longer interpreted as
"an empty string follows".
The purpose of the new option `--empty-last` (`-e`) is to restore the previous
(more logical but less practical) behaviour when needed.


## Installation

Just compile the single file `src/quoter.c` (run `make` to compile it into
`bin/quoter` with default options) and copy the result into `$PATH` under the
name `quoter`.
The code is c89 compatible with the exception of two compiler specials
which might optimize the code:
If compilation fails, try to use the compiler options
`-DAVOID_BUILTIN_EXPECT` and/or `-DAVOID_ATTRIBUTE_NORETURN`
to avoid these specials.

To obtain support for `quoter_pipe`, also `bin/quoter_pipe.sh` is needed in
`$PATH` with the executable bit being set.
To obtain zsh completions, the content of `zsh/` is needed in zsh's `$fpath`
(perhaps `/usr/share/zsh/site-functions/`).
For the standard paths, these copies all happen by `make install`.

For gentoo, there is an ebuild in the mv overlay (available by layman).
