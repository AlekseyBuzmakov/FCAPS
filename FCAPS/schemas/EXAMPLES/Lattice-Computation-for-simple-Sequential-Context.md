# Computation of the Pattern Lattice for a simple Sequential Formal Context

In this example we consider how one can use SOFIA application for building pattern lattice for a simple sequential formal context.
Simple here mean that the alphabet of involved sequences is not ordered, i.e., two different elements are incomparable and, thus, substring is defined in the classical way.

## Sequential data

Let us imagine that we have a dataset describing visits of patients to hospitals. There is 3 patients visiting 3 hospitals all together. This dataset is shown below. For example, the first patient **P1** has three hospitalizations, the first two of them were in hospital 1, while the last one was in hospital 2.

|Patient|Hospitalization Trajectory|
|---|---|
|P1|\< H1, H1, H2 \>|
|P2|\< H1, H2, H3 \>|
|P3|\< H3, H1 \>|

## Pattern lattice

Given the aforementioned dataset, we are going to construct the following pattern lattice.

{FIG:TODO}

In this lattice, the intents are taken from the following semilattice. Every element is a set of sequences, while the semilattice operation is the set of maximal substrings (without gaps) that are included by at least one sequence from every set.

## Command line

One can construct this lattice by the following command line.

> $sofia-ps -CP:[settings.json]() -data:[context.json]() -out

Where context.json encodes the dataset, and setting.json describes the processing params.

The resulting lattice can be found [here]().

## Data encoding

Data is encoded in the following way

```json
[{
	"ObjNames":["P1","P2","P3"]
},{
	"Count": 3,
	"Data": [
		[[1,1,2]],
		[[1,2,3]],
		[[3,1]]
	]
}]

```

Basically every json dataset is an array, where the first element of the array is an object with metadata, and the second object in the array is the data.
As metedata we give here the names of patients. The data is encoded as an array under the name "Data". Every element of this array is the description of the corresponding patient. Every hospital is encoded by its number. 

We should notice here that, "Data" is an array of arrays. Each such an array contains only one subarray, i.e., the sequence itself. This two embedded arrays for every patient is needed, since a description of a patient and the intents can contain several sequences. However, in our particular example every patient is described by only one sequence.

## Settings

Here we have used the following file with settings.

```json
{
	"Type": "ContextProcessorModules",
	"Name": "AddIntentContextProcessorModule",
	"Params": {
		"PatternManager": {
			"Type" : "PatternManagerModules",
			"Name" : "PartialOrderPatternManagerModule",
			"Params" : {
				"PartialOrder" : {
					"Type": "PartialOrderElementsComparatorModules",
					"Name": "DwordStringPartialOrderComparatorModule",
					"Params": {
						"MinStrLength":1,
						"CutOnEmptySymbs":true
					}
				}
			}
		},
		"OutputParams": {
			"MinExtentSize" : 1,
			"MinLift" : 1,
			"MinStab" : 1,
			"OutExtent" : true,
			"OutSupport" : true,
			"OutOrder" : true,
			"OutStability" : false,
			"OutStabEstimation" : false,
			"IsStabilityInLog" : true
		}
	}
}
```

This setting file describes which module of the program should process the data.
Here we state that the module "AddIntentContextProcessorModule" of type "ContextProcessorModules" is used.
This module can process any pattern structure by algorithm AddIntent.

> [1] D. G. Kourie, S. A. Obiedkov, B. W. Watson, and D. van der Merwe, “An incremental algorithm to construct a lattice of set intersections,” Sci. Comput. Program., vol. 74, no. 3, pp. 128–142, Jan. 2009.

This module has two parameters:
 "PatternManager" describes the module that is able to compare patterns, that correspond to the type of the data.
 "Outputparams" are different parameters of the output.

Here we specify that patterns are dealled with module "PartialOrderPatternManagerModule" of type "PatternManagerModules". This module basically transforms a partial order to a lattice. It is needed for such data types as sequences and graphs, since their partial order does not form a semilattice.

The only parameter of this module is the description of the module describing the partial order. Here, we request module "DwordStringPartialOrderComparatorModule" of type "PartialOrderElementsComparatorModules" that describes the partial order of sequences of integer positive numbers smaller than 4*10^9. It has two parameters. One of them is "MinStrLength" the minimal length of sequences that should be present in intents. If it is smaller the sequence is just removed inducing a projection of the original pattern structure. The other, "CutOnEmptySymbs", forbids for artificial all covering node inside a string.

## The file with the lattice

The resulting lattice is encoded in the following way.

```json
[{"NodesCount":7,"ArcsCount":9,"Bottom":[0],"Top":[4]},{ "Nodes":[
{"Ext":{"Count":0,"Inds":[]},"Supp":0,"Int":"BOTTOM"},
{"Ext":{"Count":1,"Inds":[0],"Names":["P1"]},"Supp":1,"Int":[[1,1,2]]},
{"Ext":{"Count":2,"Inds":[0,1],"Names":["P1","P2"]},"Supp":2,"Int":[[1,2]]},
{"Ext":{"Count":1,"Inds":[1],"Names":["P2"]},"Supp":1,"Int":[[1,2,3]]},
{"Ext":{"Count":3,"Inds":[0,1,2],"Names":["P1","P2","P3"]},"Supp":3,"Int":[[1]]},
{"Ext":{"Count":2,"Inds":[1,2],"Names":["P2","P3"]},"Supp":2,"Int":[[3],[1]]},
{"Ext":{"Count":1,"Inds":[2],"Names":["P3"]},"Supp":1,"Int":[[3,1]]}
]},{ "Arcs":[
{"S":1,"D":0},
{"S":2,"D":1},
{"S":2,"D":3},
{"S":3,"D":0},
{"S":4,"D":2},
{"S":4,"D":5},
{"S":5,"D":3},
{"S":5,"D":6},
{"S":6,"D":0}]}]
```

Basically, it is an array, where the first element is a metadata, the second element enumerates all nodes of the lattice, and third element enumerates all edges or arcs of the lattice. For every node the extent "Ext" and the intent "Int" are given. For every edges the source and destination zero-based indices of the nodes are given.
