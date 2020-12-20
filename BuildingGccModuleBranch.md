# Building g++ from the GCC modules branch
Roger Orr

## Introduction

The last issue of Overload contained Nathan Sidwell's article "C++ Modules: A Brief Tour" where he provided some short examples of C++20 modules in action. The 'Implementation' box in the article showed the status of four compilers, including Nathan's own branch of gcc. In the conclusion of the article he wrote: "Unfortunately, for GCC one must use Godbolt, which is awkward for the more advanced use, or build one\rquote s own compiler, which is a steep cliff to climb for most users."

I thought a worked example of building g++ from the modules branch from scratch might be helpful for people who are keen to experiment further with gcc's implementation of C++ modules but are intimidated by the thought of building the compiler for the first time.

## Getting started

The Gnu Compiler Collection (gcc) can be built on a very wide range of systems. The overall process is much the same, but there will be various differences depending on the exact target. One main difference between systems is the mechanism you need to use to obtain the various pre-requisites that building gcc requires.

I have no statistics on which operating systems the readership of Overload use and so I have chosen to build the compiler on *Windows 10*  using Ubuntu running in the "Windows Subsystem for Linux". This should be useful both to those with Windows machines _and_ to those with Ubuntu running natively.

Other alternatives on Windows are possible; you can for example build the compiler using Cygwin.

On other Linux distributions the process will be similar, but the actual commands used to download the other tools will depend on the package management system they use. On Ubuntu the downloads can be performed using APT (which in this context is the acronym for the "Advanced Package Tool" rather than for an "Advanced Persistent Threat" ... !)

One of the things that makes building gcc quite painful, in my experience, is that analysing any build errors is complicated by the number of lines of output the build produces. In particular, I spent quite a bit of time when I first built gcc tracking down missing dependencies since, especially as a newcomer, the symptoms do not necessarily directly indicate the root cause. For example, it is worth checking specifically for the string 'missing' early in the logs if you get a build failure to see if it may be caused by a dependency you are lacking.

## Installing WSL 
\b0\fs22
If you have not previously installed this feature, it is quite straightforward to get started with it, at least on moderately recent versions of Windows 10. Open the control panel, click on "Programs and Features" and in the resulting dialog box, click on "Turn Windows features on or off". Enable "Windows Subsystem for Linux" and click Ok.

The computer needs to reboot to install the additional feature, and when this has completed you should visit the Microsoft Store and select an appropriate Linux installation: I simply selected "Ubuntu" which, at the time of writing, installed version 20.04 LTS (Long Term Support). The length of time this takes will depend on your download speed -- it's a touch under 500 Mb. After this has completed you should now have an "Ubuntu" icon in your start menu.

The first time you run this you will need to enter the username and password for the primary account (which will be granted sudo permissions, which you will need to install the pre-requisites). I suggest the first two commands you run are "sudo apt update" and "sudo apt upgrade" which ensures your base operating system is up-to-date.

Note: the installation of WSL on earlier versions of Windows 10 required enabling developer mode, which is not the case on the current release. Additionally, there is now support for both "version 1" and "version 2" of WSL. Interesting as this might be, it is orthogonal to the primary purpose of _this_ article, which is focussed on building the g++ compiler.

## Getting dependencies
\b0\fs22
As mentioned above, the build of gcc makes use of a number of other tools, some of which will be installed in the base Linux installation but some of which may need to be installed manually. 

Since in this case I am looking to build and _use_ a branch of gcc, rather than being a developers _of_ gcc, I can save some time by avoiding the "bootstrap" part of building gcc and use a mainstream version of gcc to compile the modules branch .

For building gcc I needed to ensure the following components were present:

*bison*, *flex*, *git*, *g++*, and *make*

On Ubuntu this can be achived with one command:

`sudo apt install bison flex git g++ make`

On some installations you also need to install *m4 *and *perl*, but they're part of the base install of Ubuntu. The build also uses makeinfo to create *info* files for the compiler. I was not particularly interested in the info files, so didn't bother to install makeinfo, but if you do want those files then you also need to install the *texinfo* package which provides makeinfo.


## Checking a base build of g++

Once these dependencies are installed, you can test your setup by building the main trunk of g++. Having had various issues building g++ on Windows machines caused by the Windows default line ending (carriage return and line feed), I err on the side of caution by specifying explicit options to git to ensure that a single line feed is used.

The base build splits into two parts, firstly *downloading* the source files and some other dependencies:

```bash
mkdir ~/projects
cd ~/projects
git clone -c core.eoln=lf -c core.autocrlf=false\
  git://gcc.gnu.org/git/gcc.git
cd gcc
./contrib/download_prerequisites
```

and then *building* the compiler:

```bsh
mkdir ../build
cd ../build
../gcc-trunk/configure --disable-bootstrap --disable-multilib\
  --enable-languages=c++ --enable-threads=posix
make -j 4
```

If all goes well this will (eventually!) build a version of the latest trunk code for the g++ compiler.

A few notes on the build commands.
- It's best to build in a directory _outside_ the source tree: here I use a sibling directory
- `disable-bootstrap`  as this avoids 'bootstrapping' the build process (by using a reasonably up-to-date g++ compiler to kickstart the build) which makes the build significantly quicker
- `disable-multilib`  as I only want the 64-bit compiler, without this option I'll get the 32-bit compiler too (and will also need some additional prerequisites)
- `enable-languages=c++`  as I just want to try out the c++ modules support
- `enable-threads=posix`  so I can use threads in my C++ programs

## Building the modules branch

Once you have got this far, building the modules branch itself should be relatively straightforward. When I originally built the modules branch after the article was published, there was an additional pre-requisite (zsh) and you also needed to download and build the libcody library separately; but this was recently included as a subproject in the modules branch and so the build process is now simpler.

Simply check out the right branch:

```bash
cd ../gcc
git checkout devel/c++-modules
```

and then build and install this version

```bash
mkdir ../build-modules
cd ../build-modules
../gcc-modules/configure --disable-bootstrap --disable-multilib\
  --enable-languages=c++ --enable-threads=posix\
  --prefix=/usr/share/gcc-modules
make -j 4
sudo make install
```

Note that I am providing a specific target directory for the installation with --prefix as I don't want the modules branch build to other builds of gcc that I have installed. This does mean I will need to select this version explicitly; for example by giving the full path to g++ or by prepending the directory containing g++ to the PATH environment variable.

## Kicking the tyres

Now we should be able to build the first example from Nathan's article:

```bash
cd ~/projects/example1
export PATH=/usr/share/gcc-modules;%PATH%

g++ -fmodules-ts -std=c++20 -c hello.cc
g++ -fmodules-ts -std=c++20 -c main.cc
g++ -o main main.o hello.o
./main
Hello World
```
Success!

## Updating the build

If you want to rebuild the compiler to pick up later changes to the modules branch there are a couple of things to bear in mind.

Firstly, the `./contrib/download_prerequisites`  command adds some directories and symlinks to the source tree. You don't usually need to run this again; but if the versions of the pre-requisites *change* (as they sometimes do) it is important to remove the old versions before running the command. (My own scripts simply delete any existing artifacts and unconditionally download each time I do a build of gcc.)

Secondly, I recommend deleting the contents of the build directory before re-compiling. While in theory the timestamp-based dependency algorithm used by `make`  should handle changes smoothly, this has not always been my actual experience and the resultant build issues took me longer to resolve than any time saved by performing an incremental build.

So my instructions for a full refresh are:

```bash
cd gcc-modules
git pull --ff-only
rm -f gmp* mpfr* mpc* isl*
./contrib/download_prerequisites
rm rf ../build-modules
```
and then build and install as before.

## Conclusion

Building gcc can seem difficult, but I hope this worked example encourages some of you to try it for yourself and thereby be able to further explore the gcc modules implementation that Nathan's article made reference to.

<hr>
Copyright (c) Roger Orr 2020-11-28 20:35:22Z
First published in Overload, Dec 2020

}
 