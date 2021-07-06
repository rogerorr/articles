# Makefile for ProcessTracer

PROGRAMS = ProcessTracer TrivialPtrace MultiPtrace BadProgram BreakPoint MultiThread

all : $(PROGRAMS)

clean :
	@-rm $(PROGRAMS)

ProcessTracer : ProcessTracer.cpp
	g++ -Wall ProcessTracer.cpp -o $@

BadProgram : BadProgram.cpp
	g++ -Wall BadProgram.cpp -o $@

BreakPoint : BreakPoint.cpp
	g++ -Wall BreakPoint.cpp -o $@

MultiThread : MultiThread.cpp
	g++ -Wall MultiThread.cpp -o $@ -lpthread

TrivialPtrace : TrivialPtrace.cpp
	g++ -Wall $@.cpp -o $@

MultiPtrace : MultiPtrace.cpp
	g++ -Wall $@.cpp -o $@
