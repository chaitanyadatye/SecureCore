#!/bin/sh

#
# Telelab-LLVM compilation script
# 4-22-2004 Kihwal Lee
#

# $1 is the compiler selector
# $2 is the tmp file name base
# $3 is target name

NAME=$2
export PATH=/home/telelab/local/bin:/home/telelab/safecodellvm/tools/Debug:$PATH
export PATH=/home/telelab/safecodellvm/projects/safecode/tools/Debug:$PATH
export LD_LIBRARY_PATH=/home/telelab/local/lib:/usr/lib:/lib
COMMON_OBJS="/home/telelab/eSimplex/complex/wrapper_cx.o \
/home/telelab/eSimplex/complex/es_comm_cli.o"
LLVM_OBJS="/home/telelab/safecodellvm/projects/poolalloc.withoutcua/lib/Debug/poolalloc_safe_rt.o"
LD_FLAGS=""
LLVM_LD_FLAGS="-lstdc++"

cd /home/telelab/eSimplex/tmp
echo "Compilation started at `date`" > $NAME.stdio

# remove the previous binary
rm -f $3 $NAME.stderr

case "$1" in
   secure)

	cat $NAME.c /home/telelab/eSimplex/scripts/main.c > $NAME-1.c
	mv $NAME-1.c $NAME.c
	
	/home/telelab/llvm-gcc/gcc/xgcc -Wa,-disable-inlining \
		-Wl,-disable-inlining -o $NAME $NAME.c 2> $NAME.stderr
	RETVAL=$?
	# exit on err. mostly parse error
	[ $RETVAL -ne 0 ] && exit $RETVAL

	# if the code passed the frontend, we can ignore its stderr
	# messages. we will overwrite.
	embec -f $NAME.bc > $NAME.ll 2> $NAME.stderr
	RETVAL=$?
	# exit on security violations.
	if [ $RETVAL -ne 0 ]; then
		echo "---- POTENTIAL SECURITY PROBLEMS DETECTED ---" \
		>> $NAME.stderr
		exit $RETVAL
	fi

	# Now we generate C code then compile it with our normal
	# gcc.
	llvm-as -f $NAME.ll
	llc -f -march=c $NAME.bc

	# Ugly tricks to match the altered symbols.
	sed -e 's/void __main(void);/void __main(void){};/' $NAME.cbe.c > \
		$NAME.cbe.tmp
	sed -e 's/calc_command_cx.*(/calc_command_cx(/g' $NAME.cbe.tmp > \
		$NAME.cbe.tmp1
	sed -e 's/static float .*calc_command_cx(/float calc_command_cx(/g' \
		$NAME.cbe.tmp1 > $NAME.cbe.tmp
	sed -e 's/= .*calc_command_cx(/= calc_command_cx(/g' $NAME.cbe.tmp > \
		$NAME.cbe.tmp1
	sed -e 's/main/imain/g' $NAME.cbe.tmp1 > $NAME.cbe.c

	echo "Security check has been performed sucessfully." >> $NAME.stderr
	echo "Now compiling with gcc..." >> $NAME.stderr
	/home/telelab/local/bin/gcc -o $3 $NAME.cbe.c $COMMON_OBJS \
		$LLVM_OBJS $LLVM_LD_FLAGS >> $NAME.stdio 2>> $NAME.stderr
	RETVAL=$?
	if [ $RETVAL -eq 0 ]; then

		echo "---- Compilation Successful ---" \
		>> $NAME.stdio
		strip $3
	fi
	echo "Compilation finished at `date`" >> $NAME.stdio
	exit $RETVAL
   ;;
   normal)
	echo using normal compiler
	/home/telelab/local/bin/gcc -o $3 $NAME.c $COMMON_OBJS $LD_FLAGS >> \
		$NAME.stdio 2> $NAME.stderr
	echo "Compilation finished at `date`" >> $NAME.stdio
	RETVAL=$?

	exit $RETVAL
   ;;
   *)
	echo "Internal Error\nCompilation aborted" > $NAME.stdio
	exit 1
esac


