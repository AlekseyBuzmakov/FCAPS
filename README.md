# FCAPS

## License and copyright

Initial software, Aleksey Buzmakov, Copyright (c) INRIA and University of Lorraine, 2011-2015, Copyright NRU Higher School of Economics, 2016, GPL v2 license, v0.9

## Compilation

Bellow there are some commands that should be copied in the console (__terminal__ in Linux/MacOs or __cmd__ in Windows). By "$" a prompt is denoted, which means that this sign should NOT be typed to the console.

You will need
* git (on Windows it should be accesible from __cmd__, i.e., you should be able to run "$git status" from __cmd__)
* [boost](http://www.boost.org/)
* [premake5](https://premake.github.io/download.html)

Boost libraries should be in the _root_ of the git folder under the name __boost__. 
You can put a symbolic (or a hard) link.

> $ ln -s "path/to/boost" boost \# Linux or MacOS

> $ mklink /D boost "path/to/boost" \# Windows

Now inside __root__ (FCAPS, the cloned folder of this project), you should see the following structure.

* boost
* FCAPS
* Sofia-PS
* Tools
* {files}

Boost library can be install in the system and then the corresponding folder should be linked to boost subfloder, or can be downloaded from the site. In this case some boost libraries (regex, thread, system, filesystem) should be compiled:
* Go to boost folder and run 

> $ ./bootstrap.

Or in Windows,

> $ bootstrap.bat

* Then build the necessary libraries.

For Linux/MacOs:
> $ ./b2 --with-system --with-filesystem --with-thread --with-math debug stage

> $ ./b2 --with-system --with-filesystem --with-thread --with-math release stage

For Windows:
> $ b2.exe --with-system --with-filesystem --with-thread --with-math debug stage

> $ b2.exe --with-system --with-filesystem --with-thread --with-math release stage

The next step is to convert the project to your most loved IDE. For that run

> $ premake5 {configuration}

The most widely used configurations are 

> $ premake5 vs2005|vs2008|...|vs2017 \# for diferent version of visual studio   *sln** file

> $ premake5 gmake \# for GNU make file

> $ premake5 codeblocks \# for cbp file of code::blocks IDE

> $ premake5 clean \# for removing the created files

Basically this script fetches [rapidjson](https://github.com/miloyip/rapidjson.git) and apply the file rapidjson.patch.
Then it converts the description from the file __premake5.lua__ to the format of your IDE in love. The file is placed in the __build__ subdirectory.
Then, I guess, you know what to do with the resulting files.

After running the scirpt you should see the following structure of folders in the __root__ (two new folders had to appear):

* boost
* __build__
* FCAPS
* __rapidjson__
* Sofia-PS
* Tools
* {files}

## Some notes for debugging

This subsection is likely to be skipped, however if some probelms appear, the subsection can be useful.

* *For the moment, on some configurations, e.g., Windows and MS Visual Studio, a manual naming of static linked libraries is needed. These libraries are found in boost/stage/libs, but their names are not fixed. They are named like "lib{BOOST LIB NAME}-bla-bla-bla".{a|lib}, for example 'libboost_regex-gcc34-mt-d-1_36.a'*
* *Be careful, the boost libraries should be compiled by the same toolchain as the main program. For instance it is not possible to compile the boost libraries by the MSVS2015 toolchain and the program by the MSVS2013 toolchain. Errors of linkage occur in this case.* 

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

This version of the software separates modules dynamic libraries from the executable file. If you are interested in the old version that has all modules inside the executable, the branch [Modules-inside-Executable](https://github.com/AlekseyBuzmakov/FCAPS/tree/Modules-inside-Executable) should be of interest for you. However, this branch is not developing any more. Only hotfixes (probably) are added there.

## How to use

The examples of usage can be found [here](https://github.com/AlekseyBuzmakov/FCAPS/tree/master/FCAPS/EXAMPLES#examples-of-the-application-usage). The main executable has two principal parameters: __-data__ is a json file with the context to process; and __-CP__ is a json file with the description of a context processor module that is going to process the context. In particular AddIntent module or Sofia module are examples of such context processors. Ideally, the schemas of json files for any type of json descriptions used by the program should be found [here](https://github.com/AlekseyBuzmakov/FCAPS/tree/master/FCAPS/schemas), but the world is not ideal... not yet.

