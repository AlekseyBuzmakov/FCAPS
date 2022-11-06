# Running gSofia algorithm on top of the graph dataset

## Notice

In order to run this example you need:
1. to clone and build https://github.com/AlekseyBuzmakov/LibGastonForSofia close to FCAPS, in particular your folder structure should be something like
  * SOME FOLDER
    * FCAPS
    * LibGastonForSofia
2. to compile this project stating from FCAPS/premake5-graphs.lua

## The content of the example

* _params.json_ is the settings file to run the procedure
  * it specify to run the procedure on the graph dataset stored an LibGastonForSofia/sample-data/Chemical340
* _context.json_ it is just an empty file, needed for the procedure to be correctly run
* _lattice.json_ is the file with the found patterns, the extent of the file is a set of zero-based graph numbers, and intent is the zero based graph pattern numbers
* _PATTERNS.OUT_ is the file with found patterns

The procedure should be run as
> LD_LIBRARY_PATH="../../../boost/stage/lib/:../../../../LibGastonForSofia/bin/release/" ../../../build/final_x64/Sofia-PS -CP:params.json -out:lattice.json

Here, LD_LIBRARY_PATH is the path to libraries (bost and LibGastonForSofia). It is given as a relative path from the current directory (where the current README.md is located).
The second parameter is the path to Sofia-PS (also relative to the current folder). Finally, params is the setting file and lattice.json is the result file.

## The structure of the setting file$

```json
{
	"Type": "ComputationProcedureModules",
	"Name": "ContextBasedComputationProcedureModule",
	"Params": {
		"ContextFilePath": "context.json",
		"ContextProcessor": {
			"Type": "ContextProcessorModules",
			"Name": "SofiaContextProcessorModule",
			"Params": {
				"DefaultThld": 5,
				"MaxPatternNumber": 10000,
				"FindPartialOrder": true,
				"AdjustThreshold": true,
				"OutputParams": {
					"OutExtent": true,
					"OutIntent": true
				},
				"ProjectionChain": {
				
					"SKIPPED":"..."
                
				}
            }
        }
    }
}
```

The external structure of the file specify the task. The only options that can be changed here are:
* **DefaultThld** --  it is the default threshold for stability computation. The larger the value, the faster the program and less patterns are reported.
* **MaxPatternNumber** -- it is the memory limite for the number of stored patterns. It is used in combination with **AdjustThreshold**. The stability threshold is autoadjusted in order to limit the number of stored patterns. Just if you are out of memory make it smaller, if you want to have more patterns in the report -- make it larger.
* **ADjust Threshold** -- if true, the program tries to adjust the threshold in such a way, that the totol number of stored patterns is preserved.
* **OutParams** --  change it if you want to skip extent or intent from the output
* **ProjectionChain** is discussed later

### Projection chain block

```json
"ProjectionChain": {
	"Type": "ProjectionChainModules",
	"Name": "StabClsPatternProjectionChainModule",
	"Params": {
		"Enumerator":{
			"Type": "PatternEnumeratorModules",
			"Name": "ParallelPatternEnumeratorModule",
			"Params":{
				"PatternEnumeratorByCallback":{
					"Type": "PatternEnumeratorByCallbackModules",
					"Name": "GastonGraphPatternEnumeratorModule",
					"Params":{
						"SKIPPED":"..."
					}
				}
			}	
		},
		"ImageReserve": 1000,
		"ObjectCount": 339
	}

```

This part is just specifying the the graphs should be processed with Gaston. There are 2 parameters:
* **ImageReserve** -- it is the number of concepts to reserve in memory. Probably, 1000 is a good start.
* **ObjectCount** -- is the number of objects in the dataset. It can be larger than the actual number, but it should not be smaller.

Finally, let us check the skipped part:

```json
"Params":{
	"InputPath":"../../../../LibGastonForSofia/sample-data/Chemical_340",
	"RemoveInput": false,
	"PatternPath":"PATTERNS.OUT",
	"MinSupport":20,
	"MaxGraphSize": -1,
	"GastonMode":"Full|Trees|Pathes"
}
```

The meaning of the parameters are the following:
* **InputPath** -- it is the relative or absolute path to the graph dataset
* **RemoveInput** -- the input in **InputPath** can be removed after processing
* **PatternPath** -- it is the output file for the found patterns, they are put as numbers to the intent in the formal lattice
* **MinSupport** -- it is the minimal support of the graphs to be considered important
* **MaxGraphSize** -- the maximal size of the pattern in terms of vertices
* **GastonMode** -- by default it means to search any possible subgraph as a pattern. However if this option is set to be _Trees_ or _Pathes_ only trees or patterns would be considered
