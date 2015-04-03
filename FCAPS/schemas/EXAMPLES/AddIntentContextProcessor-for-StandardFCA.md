# Example of AddIntent Context Processor for Binary Data

File AddIntentContextProcessor-for-StandardFCA.json shows an example of settings for AddIntent context processor that uses BinarySetJoinPatternManagerModule for patterns, i.e., it processes formal contexts. 

When printing patterns, only names of attributes and not indices are printed. 
In the final lattice extent and intent of the concepts are printed but not the support, also stability and estimation of stability are printed out in the result file.

This lattice is filtered into another file by selecting concepts with stability >= 1, Lift >=1 and Support >= 2.

```json
{
	"Type": "ContextProcessorModules",
	"Name": "AddIntentContextProcessorModule",
	"Params": {
		"PatternManager": {
			"Type" : "PatternManagerModules",
			"Name" : "BinarySetJoinPatternManagerModule",
			"Params" : {
				"UseNames" : true,
				"UseInds" : false
			}
		},
		"OutputParams": {
			"MinExtentSize" : 2,
			"MinLift" : 1,
			"MinStab" : 1,
			"OutExtent" : true,
			"OutSupport" : false,
			"OutOrder" : true,
			"OutStability" : true,
			"OutStabEstimation" : true,
			"IsStabilityInLog" : true
		}
	}
}
```
