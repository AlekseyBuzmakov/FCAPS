# Example of settings for ClassifierContextProcessorModule

This file exemplifies the settings of Classifier Context Processor. It takes the classes file for the context in question (test data) and the ClassifierModule for making the classification.
You can also specify what should be written in the result file, i.e., accurasy, confusion matrix, and/or the prediction of classes.

The classifier by itself is defined by the PatternManager for working with patterns, e.g., IntervalPatternManagerModule, by the path to training data, e.g., lattice, by the path to classes of training data and by the emergency threshold.

```json
{
	"Type": "ContextProcessorModules",
	"Name": "ClassifierContextProcessorModule",
	"Params": {
		"Classifier": {
			"Type" : "ClassifierModules",
			"Name" : "CAEPByDongClassifierModule",
			"Params" : {
				"TrainPath": "{PATH}",
				"ClassesPath": "{PATH}",
				"PatternManager": {
					"Type" : "PatternManagerModules",
					"Name" : "IntervalPatternManagerModule",
					"Params" : {}
				},
				"EmergencyThld":1
			}
		},
		"ClassesPath": "{PATH}",
		"OutputParams": {
			"OutAccuracy": true,
			"OutConfusion": true,
			"OutObjectClassification": true
		}
	}
}
```
