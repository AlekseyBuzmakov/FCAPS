# FCAPS

## License and copyright

Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, 2011-2015, Copyright NRU Higher School of Economics, 2016, GPL v2 license, v0.9

## Compilation

The project is moved to **CMAKE**. The description for the old compilation can be found [here](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/COMPILE_WITH_PREMAKE.md).

Bellow there are some commands that should be copied in the console (__terminal__ in Linux/MacOs or __cmd__ in Windows). By "$" a prompt is denoted, which means that this sign should NOT be typed to the console. Some problems are discussed at the end of the section.

You will need
* git (on Windows it should be accesible from __cmd__, i.e., you should be able to run "$git status" from __cmd__)
* [boost](http://www.boost.org/)

Normally, the CMAKE should take the library from the system, so you can just install it in a regular way.
Then, building the project will be as simple as:

> cd $FCAPS\_PROJECT\_DIR

> mkdir -p build && cd build

> cmake ..

> make

However, it is possible, that in such a way the boost library will be build with different toolchain and then the linakeg would not be possible. So you can get some errors like:

> undefined reference to `boost::filesystem::detail::directory_iterator_construct(boost::filesystem::directory_iterator&, boost::filesystem::path const&, boost::system::error_code*)'

Then the only option known to me is to build boost by you self and install it to the system.

### Boost library

Goto the [boost webpage](http://www.boost.org/), download and extract the boost library.
The version 1.65.1 is garanteed to work. It is known that for windows some linkage problems can appear for versions 1.70+ but you can try. Let the library is extracted to $BOOST\_SRC folder.

Go to this folder, build and install the library. For reducing the compilation time only few libraries can be actually compiled:
* regex
* thread
* system
* filesystem

> cd $BOOST\_SRC

> $ ./bootstrap.

Or in Windows,

> $ bootstrap.bat

* Then build the necessary libraries.

For Linux/MacOs:
> $ ./b2 --with-system --with-filesystem --with-thread --with-math debug stage install

> $ ./b2 --with-system --with-filesystem --with-thread --with-math release stage install

For Windows:
> $ b2.exe --with-system --with-filesystem --with-thread --with-math debug stage install

> $ b2.exe --with-system --with-filesystem --with-thread --with-math release stage install

Then return to the FCAPS project folder and run the following:

> cd $FCAPS\_PROJECT\_DIR

> mkdir -p build && cd build

> cmake -DBOOST_ROOT=$BOOST_SRC -DBoost_NO_SYSTEM_PATHS=TRUE -DBoost_NO_BOOST_CMAKE=TRUE ..

> make

### Some notes for debugging

This subsection is likely to be skipped, however if some probelms appear, the subsection can be useful.

* *For the moment, on some configurations, e.g., Windows and MS Visual Studio, a manual naming of static linked libraries is needed. These libraries are found in boost/stage/libs, but their names are not fixed. They are named like "lib{BOOST LIB NAME}-bla-bla-bla".{a|lib}, for example 'libboost_regex-gcc34-mt-d-1_36.a'*
* *Be careful, the boost libraries should be compiled by the same toolchain as the main program. For instance it is not possible to compile the boost libraries by the MSVS2015 toolchain and the program by the MSVS2013 toolchain. Errors of linkage occur in this case.* 
* *Be careful, not all versions of boost can be compiled by old MS Visual Studios toolchains. For example, MSVC-11 (MS Visual Studio 2012) can compile boost 1.60.0 and 1.67.0 (though not all boost modules are supported) but cannot compile 1.72.0*

## Repository content

The repository contains
* the main executable project in __Sofia-PS__ subfolder
* library __Tools__ with some shared tools
* modules for different kind of pattern structures and different algorithms processing a context
  * __StdFCA__ are modules for the standard FCA. There is algorithm AddIntent and the pattern structure processing binary contexts.

  _[1] B. Ganter and R. Wille, Formal Concept Analysis: Mathematical Foundations, 1st ed. Springer, 1999._

  _[2]  D. G. Kourie, S. A. Obiedkov, B. W. Watson, and D. van der Merwe, “An incremental algorithm to construct a lattice of set intersections,” Sci. Comput. Program., vol. 74, no. 3, pp. 128–142, 2009._
  * __PS-Modules__ are modules with diferent kinds of pattern structures

  _[3] B. Ganter and S. O. Kuznetsov, “Pattern Structures and Their Projections,” in Conceptual Structures: Broadening the Base, vol. 2120, H. S. Delugach and G. Stumme, Eds. Springer Berlin Heidelberg, 2001, pp. 129–142._
  * __SofiaModules__ are modules for direct search for stable concepts.

  _[4] A. Buzmakov, S. O. Kuznetsov, and A. Napoli, “Fast Generation of Best Interval Patterns for Nonmonotonic Constraints,” in Machine Learning and Knowledge Discovery in Databases, vol. 9285, A. Appice, P. P. Rodrigues, V. Santos Costa, J. Gama, A. Jorge, and C. Soares, Eds. Springer International Publishing, 2015, pp. 157–172._
  * __GastonGraphPatternEnumeratorModule__ is a special module for enumerating extents of graphs found by Gaston. It can be used in _StabClsPatternProjectionChainModule_ modules from __SofiaModules__. This module has external dependency  on file __../LibGastonForSofia/inc__. That is the interface file of the [library](https://github.com/AlekseyBuzmakov/LibGastonForSofia/blob/master/inc/LibGastonForSofia.h) wrapping the Gaston code. If you are not going to use it, it can be excluded from the compilation.
  * __SharedModulesLib__ are premodules (not registered modules, that are registered in other dynamic libraries) that can be directly used from other modules.
* __ClassifierModules__ are very basic modules for classification with pattern structures.
* __StabilityEstimatorContextProcessor__ is a simple module for computing stability of intents.
* _FCAPS/src/fcaps/premodules_ are never compiled or used files that probably will be convert to other modules.

## How to use

When running the executable you probably have to explicitly add the folder with all libraries of boost, e.g.,
* $FCAPS\_PROJECT\_DIR/build/Sofia -I 

The examples of usage can be found [here](https://github.com/AlekseyBuzmakov/FCAPS/tree/master/FCAPS/EXAMPLES#examples-of-the-application-usage). The main executable has two principal parameters: __-data__ is a json file with the context to process; and __-CP__ is a json file with the description of a context processor module that is going to process the context. In particular AddIntent module or Sofia module are examples of such context processors. Ideally, the schemas of json files for any type of json descriptions used by the program should be found [here](https://github.com/AlekseyBuzmakov/FCAPS/tree/master/FCAPS/schemas), but the world is not ideal... not yet.

