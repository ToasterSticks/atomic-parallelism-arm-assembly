
B=build
CXX=g++
CXX_FLAGS=-Wall -Werror -O3 -g -std=c++23 -Iinclude
CC=cc
CC_FLAGS=-Wall -Werror -O3 -g -std=c23 -Iinclude


CXX_FILES=${shell find . -name '*.cxx'}
CXX_O_FILES=${addprefix $B/,${subst .cxx,.o,${CXX_FILES}}}

C_FILES=${shell find . -name '*.c'}
C_O_FILES=${addprefix $B/,${subst .c,.o,${C_FILES}}}

LINK=${firstword ${patsubst %.cxx,${CXX},${CXX_FILES} ${patsubst %.c,${CC},${C_FILES}}}}
LINK_FLAGS=

S_FILES=${sort ${wildcard *.s}}
ARM_FILES=${subst .s,.arm,${S_FILES}}
TESTS=${subst .s,.test,${S_FILES}}
OK_FILES=${subst .s,.ok,${S_FILES}}
OUT_FILES=${subst .s,.out,${S_FILES}}
DIFF_FILES=${subst .s,.diff,${S_FILES}}
RESULT_FILES=${subst .s,.result,${S_FILES}}

all : $B/arm

show:
	@echo ${CXX_FILES} ${C_FILES}

test : Makefile ${TESTS}

$B/arm: ${CXX_O_FILES} ${C_O_FILES}
	@mkdir -p $(dir $@)
	${LINK} -o $@ ${LINK_FLAGS} ${CXX_O_FILES} ${C_O_FILES}

${CXX_O_FILES} : $B/%.o: %.cxx Makefile
	@mkdir -p $(dir $@)
	${CXX} -MMD -MF $B/$*.d -c -o $@ ${CXX_FLAGS} $*.cxx

${C_O_FILES} : $B/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	${CC} -MMD -MF $B/$*.d -c -o $@ ${CC_FLAGS} $*.c

${ARM_FILES}: %.arm : Makefile %.s
	@mkdir -p $(dir $@)
	aarch64-linux-cc -nostdlib -march=armv8.1-a $*.s -o $*.arm

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

-include ${shell find $B -name '*.d' 2>/dev/null || true}

clean:
	rm -rf $B
	rm -f *.diff *.result *.out *.time *.arm
