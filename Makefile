
B=build
CXX=g++
CXX_FLAGS=-Wall -Werror -O3 -g -std=c++17
CC=cc
CC_FLAGS=-Wall -Werror -O3 -g -std=c23


CXX_FILES=${wildcard *.cxx}
CXX_O_FILES=${addprefix $B/,${subst .cxx,.o,${CXX_FILES}}}

C_FILES=${wildcard *.c}
C_O_FILES=${addprefix $B/,${subst .c,.o,${C_FILES}}}

LINK=${firstword ${patsubst %.cxx,${CXX},${CXX_FILES} ${patsubst %.c,${CC},${C_FILES}}}}
LINK_FLAGS=

ARM_FILES=${sort ${wildcard *.arm}}
TESTS=${subst .arm,.test,${ARM_FILES}}
OK_FILES=${subst .arm,.ok,${ARM_FILES}}
OUT_FILES=${subst .arm,.out,${ARM_FILES}}
DIFF_FILES=${subst .arm,.diff,${ARM_FILES}}
RESULT_FILES=${subst .arm,.result,${ARM_FILES}}

all : $B/arm

show:
	@echo ${CXX_FILES}

test : Makefile ${TESTS}

$B/arm: ${CXX_O_FILES} ${C_O_FILES}
	@mkdir -p $B
	${LINK} -o $@ ${LINK_FLAGS} ${CXX_O_FILES} ${C_O_FILES}

${CXX_O_FILES} : $B/%.o: %.cxx Makefile
	@mkdir -p $B
	${CXX} -MMD -MF $B/$*.d -c -o $@ ${CXX_FLAGS} $*.cxx

${C_O_FILES} : $B/%.o: %.c Makefile
	@mkdir -p $B
	${CC} -MMD -MF $B/$*.d -c -o $@ ${CC_FLAGS} $*.c

${TESTS}: %.test : Makefile %.result
	echo "$* ... $$(cat $*.result) [$$(cat $*.time)]"

${RESULT_FILES}: %.result : Makefile %.diff
	echo "unknown" > $@
	((test -s $*.diff && echo "fail") || echo "pass") > $@


${DIFF_FILES}: %.diff : Makefile %.out %.ok
	@echo "no diff" > $@
	-diff $*.out $*.ok > $@ 2>&1

${OUT_FILES}: %.out : Makefile $B/arm %.arm
	@echo "failed to run" > $@
	-time --quiet -f '%e' -o $*.time timeout 10 $B/arm $*.arm > $@

-include $B/*.d

clean:
	rm -rf $B
	rm -f *.diff *.result *.out *.time
