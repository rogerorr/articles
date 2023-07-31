all: x64 x64\optimised x64\fully_optimised x64\unoptimised\
  x64\optimised\TestStackWalker.exe x64\optimised\TestStackWalker.dump \
  x64\fully_optimised\TestStackWalker.exe x64\fully_optimised\TestStackWalker.dump \
  x64\unoptimised\TestStackWalker.exe x64\unoptimised\TestStackWalker.dump \
  x64\unoptimised\TestUnhandledExceptionFilter.exe

clean:
	@-del /q/s x64

CC_FLAGS=$(CC_FLAGS) /W4 /EHsc /Zi
CC_FLAGS=$(CC_FLAGS) /I "$(VSINSTALLDIR)\DIA SDK\include"

x64 x64\optimised x64\fully_optimised x64\unoptimised:
  mkdir $@

x64\optimised\SimpleStackWalker.obj: SimpleStackWalker.cpp SimpleStackWalker.h
  cl /nologo /c /Fd$(*D)\ /Fo$@ /O1 $(CC_FLAGS) /DNDEBUG /MD $(*B).cpp
 
x64\fully_optimised\SimpleStackWalker.obj: SimpleStackWalker.cpp SimpleStackWalker.h
  cl /nologo /c /Fd$(*D)\ /Fo$@ /Ox $(CC_FLAGS) /DNDEBUG /MD $(*B).cpp
 
x64\unoptimised\SimpleStackWalker.obj: SimpleStackWalker.cpp SimpleStackWalker.h
  cl /nologo /c /Fd$(*D)\ /Fo$@ $(CC_FLAGS) /D_DEBUG /MDd $(*B).cpp
 
x64\optimised\TestStackWalker.exe: TestStackWalker.cpp $(*D)\SimpleStackWalker.obj
	cl /nologo /Fo$(*D)\ /Fd$(*D)\ /Fe$@ /O1 /FAsc /Fa$*.cod $(CC_FLAGS) /DNDEBUG /MD $**

x64\fully_optimised\TestStackWalker.exe: TestStackWalker.cpp $(*D)\SimpleStackWalker.obj
	cl /nologo /Fo$(*D)\ /Fd$(*D)\ /Fe$@ /Ox /FAsc /Fa$*.cod $(CC_FLAGS) /DNDEBUG /MD $**

x64\unoptimised\TestStackWalker.exe: TestStackWalker.cpp $(*D)\SimpleStackWalker.obj
	cl /nologo /Fo$(*D)\ /Fd$(*D)\ /Fe$@ /FAsc /Fa$*.cod $(CC_FLAGS)  /D_DEBUG /MDd $**

x64\unoptimised\TestUnhandledExceptionFilter.exe: TestUnhandledExceptionFilter.cpp $(*D)\SimpleStackWalker.obj
	cl /nologo /Fo$(*D)\ /Fd$(*D)\ /Fe$@ /FAsc /Fa$*.cod $(CC_FLAGS)  /D_DEBUG /MDd $**

x64\optimised\TestStackWalker.dump: $*.exe
	c:\projects\GitHub\microsoft-pdb\cvdump\cvdump.exe $*.exe > $@

x64\fully_optimised\TestStackWalker.dump: $*.exe
	c:\projects\GitHub\microsoft-pdb\cvdump\cvdump.exe $*.exe > $@

x64\unoptimised\TestStackWalker.dump: $*.exe
	c:\projects\GitHub\microsoft-pdb\cvdump\cvdump.exe $*.exe > $@
