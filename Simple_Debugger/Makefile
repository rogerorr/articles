# Makefile for ProcessTracer

all : ProcessTracer.exe BadProgram.exe CloseHandle.exe UnhandledExceptionFilter.exe

clean :
	@-del *.obj *.exe *.exe.manifest *.ilk *.pdb

# Rules

.cpp.obj::
	cl /nologo /c /Zi /EHsc /GR /MD /W4 $<

ProcessTracer.exe : $(*B).obj
	cl /nologo /Zi $**

BadProgram.exe : $(*B).obj 
	cl /nologo /Zi $**

CloseHandle.exe : $(*B).obj 
	cl /nologo /Zi $**

UnhandledExceptionFilter.exe : $(*B).obj 
	cl /nologo /Zi $**

# Dependencies

ProcessTracer.obj : SimpleSymbolEngine.h

SimpleSymbolEngine.obj : SimpleSymbolEngine.h

ProcessTracer.exe : SimpleSymbolEngine.obj
