# Using 32-bit COM objects from 64-bit programs

## Introduction

Microsoft introduced COM - the "Component Object Model" - in the early 1990s and it has proved to be
remarkably resilient even though the computing world has changed quite a bit since those days - for
example most people who run Windows are now running it on 64-bit hardware. However, a large number of
the programs and components that run in such environments are still 32-bit for a variety of reasons,
some better than others.

While Microsoft have done a pretty good job of integrating a mix of 64- and 32-bit applications there
can be a problem when it comes to using older 32-bit COM components in a 64-bit application as Windows
does not allow a mix of 32-bit and 64-bit user code in a single application. This means a 32-bit COM
DLL cannot be used by a 64-bit application (although a 32-bit COM EXE can.)

There are a variety of solutions to this problem. The best solution is to use a 64-bit version of the
COM DLL as this will provide the best integration with the 64-bit application. However this requires
that a 64-bit version exists, or can be produced; this is not always the case.

An alternative solution is to host the 32-bit COM DLL inside a 32-bit application and use this hosting
application from the 64-bit application. Microsoft provide an easy-to-use standard example of such a
solution in the form of a DllHost process.

This article describes how to use this DllHost mechanism and highlights a couple of potential issues
with this as a solution that you need to be aware of.

## Demonstration of the problem

We can quickly show the problem by writing an example 32-bit COM DLL in C# and trying to use it with
the 32-bit and the 64-bit scripting engines. (In general the 32-bit COM DLLs likely to need this
solution are probably written in some other language, such as C++, but the C# example has the benefit of being short!)

```
---- CSharpCOM.cs ----
namespace CSharp_COMObject
{
    using System.Runtime.InteropServices;

    [Guid("E7C52644-7AF1-4B8B-832C-23816F4188D9")]
    public interface CSharp_Interface
    {
        [DispId(1)]
        string GetData();
    }

    [Guid("1C5B73C2-4652-432D-AEED-3034BDB285F7"),
    ClassInterface(ClassInterfaceType.None)]
    public class CSharp_Class : CSharp_Interface
    {
        public string GetData()
        {
            if (System.Environment.Is64BitProcess)
                return "Hello from 64-bit C#";
            else
                return "Hello from 32-bit C#";
        }
    }
}
```
This can be turned into a COM object from the Visual Studio "Developer Command Prompt" using:

`csc /t:library CSharpCOM.cs`

and then registered (from an administrator command prompt) using:

`RegAsm.exe CSharpCOM.dll /codebase`

(You may need to provide the full path to `RegAsm.exe`, such as:
`C:\Windows\Microsoft.NET\Framework\v4.0.30319\RegAsm.exe)`

Note: you are likely to get a warning about registering an unsigned assembly - we are not concerned
about that for the purposes of this demonstration.

Next create a VB script that loads and uses this COM object:

```
----- exercise.vbs -----
Set obj = WScript.CreateObject("CSharp_COMObject.CSharp_Class")
WScript.Echo obj.GetData
```
and invoke it with the **32-bit** scripting engine:

<code>c:\windows\<b>syswow64</b>\cscript.exe /nologo exercise.vbs</code>

If all has gone according to plan this will produce the output:

`Hello from 32-bit C#`

Now try to invoke it using the **64-bit** scripting engine:

<code>c:\windows\<b>system32</b>\cscript.exe /nologo exercise.vbs</code>

This will fail with the error message:
 `WScript.CreateObject: Could not locate automation class named "CSharp_COMObject.CSharp_Class".`

This demonstrates the typical issue when trying to use a 32-bit COM object from a 64-bit application
(or vice versa). While the precise error message may vary a little depending on the hosting application
the basic problem is that the 64-bit application cannot locate the 32-bit COM object.

## Configuring the COM object to use DllHost

In order to make use of the standard DllHost mechanism we simply need to add a couple of registry keys
to the system. Microsoft have used the term "DllSurrogate" for this mechanism and this name is used in
one of the values written into the registry.

We need to add a new value to the CLSID for the class(es) affected and this change must be made in
the **32-bit **area of the registry. We must then also define a new application with a blank DllSurrogate.

In our example the target CLSID is {`1C5B73C2-4652-432D-AEED-3034BDB285F7`} (the GUID we provided for
our COM class in the C# source code) and we need to create a new GUID for our DLL surrogate. I've split the lines up
with the '`^`' continuation character for ease of reading:

*(1) Get a new GUID for the DllSurrogate*<br>
`C:> uuidgen -c 308B9966-063A-48B8-9659-4EBA6626DE5C`

*(2) Add an AppID value to the (32-bit) class ID*<br>
`c:\windows\**syswow64\b0\reg.exe add ^`
`HKCR\CLSID\{1C5B73C2-4652-432D-AEED-3034BDB285F7} ^`
`/v AppID /d {308B9966-063A-48B8-9659-4EBA6626DE5C}`

*(3) Create a new AppID*<br>
`reg.exe add HKCR\AppID\{308B9966-063A-48B8-9659-4EBA6626DE5C} ^`
`/v DllSurrogate /d ""`

Now we can successfully use the COM object from the 64-bit scripting engine:

`c:\windows\system32\cscript.exe /nologo exercise.vbs`<br>
`Hello from 32-bit C#`

(*Note*: strictly speaking you do not need a new GUID for the AppID; some people recommend
that you re-use the CLSID. I prefer using a new GUID for clarity.)

## Removing the COM object and the surrogate

If you like keeping your computer tidy and would like to unregister the COM object, you can do this by
reversing the installation above with the following commands:

`reg.exe delete HKCR\AppID\{308B9966-063A-48B8-9659-4EBA6626DE5C}`

<code>c:\windows\<b>syswow64</b>\reg.exe delete ^</code>
<code>HKCR\CLSID\{1C5B73C2-4652-432D-AEED-3034BDB285F7} /v AppID</code>

`regasm.exe CSharpCOM.dll /u`

## How does it work?

If you use the task manager or some similar tool to examine the processes running in the computer you
will see an instance of the `Dllhost.exe` process start and then disappear after a few seconds.

The command line for DllHost contains the CLSID of the target class and this (32-bit) process actually
creates the 32-bit COM object using this CLSID and makes it available to the calling application, using
cross-process marshalling for the calls between the 64-bit application and the 32-bit target object.

When the COM object is destroyed, DllHost hangs around for a few seconds, ready for another process
to create a COM object. If none does it times out and exits.

## Some differences from in-process COM objects

There are a number of differences between a 'normal' in-process COM object and this externally hosted object.

The first obvious difference is that each call into the COM object now has to marshall data between
the calling process and the DllHost process. This is obviously going to introduce a performance overhead.
It is hard to tell how significant this performance overhead will be; it depends on the 'chattiness' of
the COM interface and the sizes of the data items being passed around.

Some COM interfaces pass non-serialisable values around (for example, pointers to internal data structures)
and these will require additional support of some kind to work successfully in the Dll surrogate environment.

Another difference is that the same DllHost process may host COM objects for a number of different applications at the
**same **time. While this is perfectly valid according to the rules of COM, there are a number of cases where problems
can arise - for example if the implementation makes assumptions about being able to share internal data between separate COM objects.

Additionally, some in-process COM objects are written with some assumptions that the calling application code is in
the same process as the COM object. For example, the name of a log file might be based on the name of the application.
This may be an insurmountable problem for using this solution, although if the COM interface is in your control you
might be able to enhance it to provide additional methods to support out-of-process operation.

## Multiple classes

If your COM DLL provides a number of different COM classes, you might wish to host them all inside the
same DllHost instance. You can do this by using the \i same \i0 AppID for each class ID; the first COM
object created will launch the DllHost and each additional object created by the application will simply
bind to an object in the same DllHost.

## Calling 64-bit COM objects from 32-bit applications

The commonest use case for Dll surrogates is allowing a legacy 32-bit COM object to used in a new
64-bit application, but the same mechanism does work in the other direction, although it is less common for this to be necessary.

You need to add the same registry keys as above, but this time the CLSID will be the one in the
**64-bit **registry hive so you will use the 64-bit reg.exe from the normal directory `C:\Windows\System32`.

## A note on 64-bit .Net COM objects

In this article I have used a simple C# COM object to demonstrate the problem of 64-bit programs using
32-bit COM objects and how to solve this by using a Dll surrogate.

A native COM DLL, for example written in C++, is built for either 32-bit or 64-bit use and can only be
loaded by a program of the same bitness. In this case the Dll surrogate or an equivalent is the only
way that the native COM DLL can be used by a program of different bitness.

C# programs though are 'bit-size agnostic' by default -- the same C# program can run in 32-bit or 64-bit
mode and the same C# assembly can be used from both 32-bit and 64-bit programs. (This works because
there are two .Net virtual machine implementations, one for 32-bit programs and one for 64-bit ones.)

So the DLL surrogate approach used in this article is actually required only in the case of legacy COM
DLLs, as .Net COM objects can operate in this dual-mode fashion.

The fundamental reason why our C# COM object could be used by 32-bit programs but not by the
64-bit scripting engine was because it had been \i registered \i0 with the 32-bit regasm program,
which only writes entries to the areas of the system registry read by 32-bit programs. If you
register the C# COM DLL with the 64-bit regasm, for example:

<code>C:\Windows\Microsoft.NET\Framework<b>64</b>\v4.0.30319\RegAsm.exe CSharpCOM.dll /codebase</code>

then the C# COM object will be usable directly by a 64-bit program without needing to use a Dll
surrogate. However this is only true for .Net assemblies, whereas the Dll surrogate approach works with native DLLs as well.

## Summary

While many users of Windows will be able to make use of 64-bit applications with 64-bit COM
objects it is good to know that, subject to a few restrictions, you can make use of a mix of
64-bit and 32-bit components by setting only a few registry keys.

## Further information
There is some Microsoft documentation about all this at:
https://msdn.microsoft.com/en-us/library/windows/desktop/ms695225%28v=vs.85%29.aspx

While the technique has been available for some time, there was initially a lack of documentation
about the process and, in my experience anyway, few people are aware of the existence of this technique.

<hr>

Copyright (c) Roger Orr 2021-07-06 (First published in C Vu,  May 2015)
