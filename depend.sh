#!/bin/sh

# Automatically generates dependencies

# 1. Dependencies of library sources
sources=`ls src | grep '\.c'`
for file in $sources; do
        src="src/$file"
    	name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
        includes1=`grep '^[ 	]*#[ 	]*include[ 	]*[a-zA-Z0-9_][a-zA-Z0-9_]*' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*\([a-zA-Z0-9_]*\),\1,'`
        includes2=`grep '^[ 	]*#[ 	]*include[ 	]*".*"' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*"\(.*\)",\1,'`
	headers=
	if test -n "$includes1"; then
		for header in $includes1; do
			if test -f "include/$header"; then
				headers="$headers include/$header"
			fi
			if test -f "src/$header"; then
				headers="$headers src/$header"
			fi
		done
	fi
	if test -n "$includes2"; then
		for header in $includes2; do
			if test -f "include/$header"; then
				headers="$headers include/$header"
			fi
			if test -f "src/$header"; then
				headers="$headers src/$header"
			fi
		done
	fi
	echo "obj/$name.o: src/$file $headers"
done

# 2. Dependencies of examples
sources=`ls tests | grep '\.c'`
for file in $sources; do
        src="tests/$file"
    	name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
        includes1=`grep '^[ 	]*#[ 	]*include[ 	]*[a-zA-Z0-9_][a-zA-Z0-9_]*' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*\([a-zA-Z0-9_]*\),\1,'`
        includes2=`grep '^[ 	]*#[ 	]*include[ 	]*".*"' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*"\(.*\)",\1,'`
	headers=
	if test -n "$includes1"; then
		for header in $includes1; do
			if test -f "include/$header"; then
				headers="$headers include/$header"
			fi
		done
	fi
	if test -n "$includes2"; then
		for header in $includes2; do
			if test -f "src/$header"; then
				headers="$headers src/$header"
			fi
		done
	fi
	echo "tests/$name: tests/$file $headers"
done
