include $(top_srcdir)/config/Makefile.for.ROSE.includes.and.libs

###########################################################################################
# These are C99 testcodes targeting aspects of C99 that might not be a subset of C or C++ #
###########################################################################################

if ROSE_USE_EDG_VERSION_4_4
TESTCODES_REQUIRED_TO_PASS = \
     test2005_186.c \
     test2006_127.c \
     test2006_143.c \
     test2012_20.c \
     test2012_23.c \
     test2012_25.c \
     test2012_58.c \
     test2012_100.c \
     test2012_104.c \
     test2012_105.c \
     complex_01.c \
     complex_03.c \
     bool.c \
     constants.c 

# DQ (10/6/2012): This is a failing test for the new edg 4.x work.
# test2008_01.c 
# test2012_106.c

else
TESTCODES_REQUIRED_TO_PASS = \
     test2005_186.c \
     test2006_127.c \
     test2006_143.c \
     test2008_01.c \
     complex_01.c \
     complex_03.c \
     bool.c \
     constants.c 
endif

TESTCODE_CURRENTLY_FAILING = \
     complex_02.c \
     complex_04.c 

# Automake's testing mechanism (which defines the "make check" rule) requires passing tests.
TESTCODES = \
$(TESTCODES_REQUIRED_TO_PASS)

# QMTest allows both passing and failing tests.
ALL_TESTCODES = \
$(TESTCODES_REQUIRED_TO_PASS) \
$(TESTCODE_CURRENTLY_FAILING)

#ROSE_FLAGS = -rose:C99 --edg:no_warnings -w --edg:restrict 
ROSE_FLAGS = -rose:verbose 0 -rose:C99 --edg:no_warnings -w --edg:restrict 

PASSING_TEST_Objects = ${TESTCODES:.c=.o}
TEST_Objects = ${ALL_TESTCODES:.c=.o}

# DQ (10/27/2012): Removed the dependency on the object files to avoid recompiling all files.
# $(TEST_Objects): ../../testTranslator $(srcdir)/$(@:.o=.c)
$(TEST_Objects): ../../testTranslator
	../../testTranslator $(ROSE_FLAGS) -c $(srcdir)/$(@:.o=.c)

../../testTranslator:
	cd ../..; $(MAKE) testTranslator

QMTEST_Objects = ${ALL_TESTCODES:.c=.qmt}

# Make rule to build the QMTest database files
CURRENT_DIRECTORY = `pwd`
$(QMTEST_Objects): ../../testTranslator $(srcdir)/$(@:.qmt=.c)
	qm.sh f rose.RoseTest $(CURRENT_DIRECTORY)/../../testTranslator NULL $(ROSE_FLAGS) -c $(srcdir)/$(@:.qmt=.c)

# Include makefile rules specific to QMTest
include $(top_srcdir)/config/QMTest_makefile.inc

EXTRA_DIST = $(ALL_TESTCODES)

check-local:
	@echo "Tests for C99 examples."
	@$(MAKE) $(PASSING_TEST_Objects)
	@echo "***********************************************************************************************"
	@echo "****** ROSE/tests/CompileTests/C99_tests: make check rule complete (terminated normally) ******"
	@echo "***********************************************************************************************"

clean-local:
	rm -f *.o rose_*.[cC]
	rm -rf QMTest

