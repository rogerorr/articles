# Makefile for ProcessTracer

!if "$(PLATFORM)" == ""
PLATFORM=x86
!endif

all : $(PLATFORM) $(PLATFORM)\ProcessTracer.exe $(PLATFORM)\TrivialProgram.exe $(PLATFORM)\BadProgram.exe $(PLATFORM)\TestSimpleSymbolEngine.exe

clean :
	@-del /s/q ProcessTracer.zip $(PLATFORM)\*

zip :
	if not exist ProcessTracer\* mkdir ProcessTracer
	del ProcessTracer\* /q
	del ProcessTracer.zip
	copy BadProgram.cpp ProcessTracer\*
	copy Makefile ProcessTracer\*
	copy ProcessTracer.cpp ProcessTracer\*
	copy SimpleSymbolEngine.cpp ProcessTracer\*
	copy SimpleSymbolEngine.h ProcessTracer\*
	copy TrivialProgram.cpp ProcessTracer\*
	zip ProcessTracer.zip ProcessTracer/*

# Rules

$(PLATFORM) :
	md $(PLATFORM)

.cpp{$(PLATFORM)}.obj::
	cl /nologo /Fo$(PLATFORM)\ /c /Zi /EHsc /GR /MD /W4 $<

$(PLATFORM)\ProcessTracer.exe : $(PLATFORM)\$(*B).obj
	cl /nologo /Fe$@ /Zi $**

$(PLATFORM)\TrivialProgram.exe : $(PLATFORM)\$(*B).obj 
	cl /nologo /Fe$@ $** 

$(PLATFORM)\BadProgram.exe : $(PLATFORM)\$(*B).obj 
	cl /nologo /Fe$@ /Zi $**

$(PLATFORM)\TestSimpleSymbolEngine.exe : $(PLATFORM)\$(*B).obj
	cl /nologo /Fe$@ /Zi $**

# Dependencies

$(PLATFORM)\ProcessTracer.obj : SimpleSymbolEngine.h

$(PLATFORM)\SimpleSymbolEngine.obj : SimpleSymbolEngine.h

$(PLATFORM)\TestSimpleSymbolEngine.obj : SimpleSymbolEngine.h

$(PLATFORM)\ProcessTracer.exe : $(PLATFORM)\SimpleSymbolEngine.obj

$(PLATFORM)\TestSimpleSymbolEngine.exe : $(PLATFORM)\SimpleSymbolEngine.obj