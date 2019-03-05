## Settings for SOFIA and Interval Pattern Structure

Here a small example on running SOFIA algorithm on interval pattern structure is shown. SOFIA finds stable concepts in polynomial time.
The cost for polinomial time is the risk that no pattern is found.

The command line is provided below. Every used file is downloadable by clicking.

$sofia-ps -CP:[settings.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/Sofia-IPS/settings.json)  -out

It generates file [result.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/Sofia-IPS/result.json) with the found stable concepts.

The data is stored in [context.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/Sofia-IPS/context.json) file and is referenced by the [settings.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/Sofia-IPS/settings.json) file.

Bellow we shortly describe how this files are constructed.

### The Settings File

In this section we discuss [settings.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/Sofia-IPS/settings.json). In what follws in this section we will present parts of the file with some comments.

```json
{
    "Type": "ComputationProcedureModules",
    "Name": "ContextBasedComputationProcedureModule",
    "Params": {
        "ContextFilePath": "context.json",
```

ContextFilePath is the path to the file with the data.

```json
        "ContextProcessor": {
		"Type" : "ContextProcessorModules",
		"Name" : "SofiaContextProcessorModule",
```
ContextProcessor is the algorithm that procces context. In this example we works with algorithm SOFIA. The parameters of the SOFIA is given below.

```json
		"Params" : {
			"ProjectionChain" : {	
				"Type": "ProjectionChainModules",
				"Name": "StabIntervalClsPatternsProjectionChainModule",
```

Here we just say that we are dealing with Interval Pattern Structures, i.e., we are interesting in analysing numerical data and our patterns (intents) are tupplse of numerical intervals.

```json
				"Params": {}
			},
			"DefualtThld": 2,
			"MaxPatternNumber": 10,
			"AdjustThreshold": true,
			"FindPartialOrder": true,
			"OutputParams" : {
				"OutExtent": true,
				"OutIntent": true
			},
			"MaxKnownConceptSize":15000
		}
        }
    }
}
```
The final lines are parameters of SOFIA. 


**DefaultThld** is the minimal Delta-measure for found concepts. In particular we say that Delta-measure is at least 2, the larger the parameter the more ``closed'' the pattern is, i.e., in more bootstrap datasets it is closed.

**MaxPatternNumber** and **AdjustThreshold** define should the threshold for Delta-measure be adjusted. If yes it specify the maximal number of candidate patterns to be stored in the memory and if at any moment the number of patterns is increased, the Delta-measure threshold is increased accordingly.

**MaxKnownConceptSize** works similarly to **MaxPatternNumber** and **AdjustThreshold** however it control physical memory size in bytes of the stored candidate concepts rather than just their number. It is better to set this value at most half of the whole physical memory of the computer. Otherwise, the program could hang on your computer for a large dataset.

**FindPartialOrder** is specifying that the concepts should be reported as a partial order rather than just a set of concepts.

**OutParams** specifying how the found concepts should be reported, in particular it is said that the extent and the intent of the concepts are reported.

