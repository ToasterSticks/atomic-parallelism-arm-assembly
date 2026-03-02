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
TEST_LOOPS = ${subst .s,.loop,${S_FILES}}
TEST_FAILS = ${subst .s,.fail,${S_FILES}}

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

OTHER_USERS = ${shell who | sed -e 's/ .*//' | sort | uniq}
HOW_MANY = ${shell who | sed -e 's/ .*//' | sort | uniq | wc -l}
LOOP_LIMIT ?= 10

loop_warning.%:
	@echo "*******************************************************************************"
	@echo "*** This is NOT the sort of thing you run ALL THE TIME on a SHARED MACHINE  ***"
	@echo "*** In particular long running tests and tests that timeout                 ***"
	@echo "*******************************************************************************"
	@echo ""
	@echo "You can use LOOP_LIMIT to control the number if iterations. For example:"
	@echo "   LOOP_LIMIT=7 make -s $*.loop"
	@echo ""
	@echo "::::::: You are 1 of ${HOW_MANY} users on this machine"
	@echo ":::::::         ${OTHER_USERS}"
	@echo ":::::::   all of them value their work and their time as much as you value yours"
	@echo ":::::::"
	@echo ""

${TEST_LOOPS} : %.loop : loop_warning.% %.result
	@pass=0; \
	i=1; \
	while [ $$i -le ${LOOP_LIMIT} ]; do \
		echo -n  "[$$i/${LOOP_LIMIT}] "; \
		rm $*.out $*.result; \
		$(MAKE) -s $*.test; \
		if [ "`cat $*.result`" = "pass" ]; then pass=$$((pass + 1)); fi; \
		i=$$((i + 1)); \
	done; \
	echo ""; \
	echo "$$(basename $$(pwd)) $* $$pass/${LOOP_LIMIT}"; \
	echo ""

${TEST_FAILS} : %.fail : loop_warning.% %.result
	@pass=0; \
	i=1; \
	while [ $$i -le ${LOOP_LIMIT} ]; do \
		echo -n  "[$$i/${LOOP_LIMIT}] "; \
		rm $*.out $*.result; \
		$(MAKE) -s $*.test; \
		if [ "`cat $*.time`" = "timeout" ]; then echo "timeout"; break; fi; \
		if [ "`cat $*.result`" = "pass" ]; then pass=$$((pass + 1)); else break; fi; \
		i=$$((i + 1)); \
	done; \
	echo ""; \
	echo "$$(basename $$(pwd)) $* $$pass/${LOOP_LIMIT}"; \
	echo ""	

test.loop: loop_warning.test ${TEST_LOOPS}

test.fail: loop_warning.test ${TEST_FAILS}

clean:
	rm -rf $B
	rm -f *.diff *.result *.out *.time *.arm

-include ${shell find $B -name '*.d' 2>/dev/null || true}
