#!/bin/sh
# autoconf -- create `configure' using m4 macros
# Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# If given no args, create `configure' from template file `configure.in'.
# With one arg, create a configure script on standard output from
# the given template file.

usage="\
Usage: autoconf [-h] [--help] [-m dir] [--macrodir=dir] 
       [-l dir] [--localdir=dir] [--version] [template-file]" 

# NLS nuisances.
# Only set `LANG' and `LC_ALL' to "C" if already set.
# These must not be set unconditionally because not all systems understand
# e.g. LANG=C (notably SCO).
if test "${LC_ALL+set}" = set; then LC_ALL=C; export LC_ALL; fi
if test "${LANG+set}"   = set; then LANG=C;   export LANG;   fi

test -z "${AC_MACRODIR}" && AC_MACRODIR=@datadir@
test -z "${M4}" && M4=@M4@
case "${M4}" in
/*) # Handle the case that m4 has moved since we were configured.
    # It may have been found originally in a build directory.
    test -f "${M4}" || M4=m4 ;;
esac

tmpout=/tmp/acout.$$
localdir=
show_version=no

while test $# -gt 0 ; do
   case "${1}" in 
      -h | --help | --h* )
         echo "${usage}" 1>&2; exit 0 ;;
      --localdir=* | --l*=* )
         localdir="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
         shift ;;
      -l | --localdir | --l*)
         shift
         test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
         localdir="${1}"
         shift ;;
      --macrodir=* | --m*=* )
         AC_MACRODIR="`echo \"${1}\" | sed -e 's/^[^=]*=//'`"
         shift ;;
      -m | --macrodir | --m* ) 
         shift
         test $# -eq 0 && { echo "${usage}" 1>&2; exit 1; }
         AC_MACRODIR="${1}"
         shift ;;
      --version | --v* )
         show_version=yes; shift ;;
      -- )     # Stop option processing
        shift; break ;;
      - )	# Use stdin as input.
        break ;;
      -* )
        echo "${usage}" 1>&2; exit 1 ;;
      * )
        break ;;
   esac
done

if test $show_version = yes; then
  version=`sed -n 's/define.AC_ACVERSION.[ 	]*\([0-9.]*\).*/\1/p' \
    $AC_MACRODIR/acgeneral.m4`
  echo "Autoconf version $version"
  exit 0
fi

case $# in
  0) infile=configure.in ;;
  1) infile="$1" ;;
  *) echo "$usage" >&2; exit 1 ;;
esac

trap 'rm -f $tmpin $tmpout; exit 1' 1 2 15

tmpin=/tmp/acin.$$ # Always set this, to avoid bogus errors from some rm's.
if test z$infile = z-; then
  infile=$tmpin
  cat > $infile
elif test ! -r "$infile"; then
  echo "autoconf: ${infile}: No such file or directory" >&2
  exit 1
fi

if test -n "$localdir"; then
  use_localdir="-I$localdir -DAC_LOCALDIR=$localdir"
else
  use_localdir=
fi

# Use the frozen version of Autoconf if available.
r= f=
# Some non-GNU m4's don't reject the --help option, so give them /dev/null.
case `$M4 --help < /dev/null 2>&1` in
*reload-state*) test -r $AC_MACRODIR/autoconf.m4f && { r=--reload f=f; } ;;
*traditional*) ;;
*) echo Autoconf requires GNU m4 1.1 or later >&2; rm -f $tmpin; exit 1 ;;
esac

$M4 -I$AC_MACRODIR $use_localdir $r autoconf.m4$f $infile > $tmpout ||
  { rm -f $tmpin $tmpout; exit 2; }

# You could add your own prefixes to pattern if you wanted to check for
# them too, e.g. pattern="AC_\|ILT_", except that UNIX sed doesn't do
# alternation.
pattern="AC_"

status=0
if grep "${pattern}" $tmpout > /dev/null 2>&1; then
  echo "autoconf: Undefined macros:" >&2
  grep "${pattern}" $tmpout | sed "s/.*\(${pattern}[_A-Z0-9]*\).*/\1/" |
    while read name; do
      grep -n $name $infile /dev/null
    done | sort -u >&2
  status=1
fi

if test $# -eq 0; then
  exec 4> configure; chmod +x configure
else
  exec 4>&1
fi

# Put the real line numbers into configure to make config.log more helpful.
awk '
/__oline__/ { printf "%d:", NR + 1 }
           { print }
' $tmpout | sed '
/__oline__/s/^\([0-9][0-9]*\):\(.*\)__oline__\(.*\)$/\2\1\3/
' >&4

rm -f $tmpout

exit $status
