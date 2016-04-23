# Computation of the Pattern Lattice for a composite Pattern Structure

In this example we consider how we can construct a lattice for a pattern structure being a direct product of other pattern structure.
In particular here we consider an example of frequently found dataset with numerical and binary attributes together.

## Sample dataset

Let us imagine a dataset having 3 binary attributes a,b, c, and 2 numerical attributes X and Y. It has 3 objects and is shown below.

|Obj| a | b | c | X | Y |
|---|---|---|---|---|---|
| g1| x |   | x | 1 | [1,2] |
| g2| x |   |   | 0 | 2 |
| g3|   | X | x | 2 | 1 |

## Pattern lattice

Given the aforementioned dataset, we are going to construct the following pattern lattice.

{FIG:TODO}

In this lattice, the intents are taken from the following semilattice. Every element is a set of sequences, while the semilattice operation is the set of maximal substrings (without gaps) that are included by at least one sequence from every set. Thus, this pattern structure is a direct product of an interval pattern structure containing attributes X and Y, and a binary pattern structure (standard FCA) based on the set. Here after we will encode the dataset and the pattern structure params based on this notice.

## Command line

One can construct this lattice by the following command line.

> $sofia-ps -CP:[settings.json]() -data:[context.json]() -out

Where context.json encodes the dataset, and setting.json describes the processing params.

The resulting lattice can be found [here]().

## Data encoding

Data is encoded in the following way

```json
[{
	"ObjNames":["g1","g2","g3"]
},{
	"Count": 3,
	"Data": [
		[{"Inds":[0,2]},[1,1]],
		[{"Inds":[0]},[0,2]],
		[{"Inds":[1,2]},[2,1]]
	]
}]

```

Basically every json dataset is an array, where the first element of the array is an object with metadata, and the second object in the array is the data.
As metedata we give here the names of patients. The data is encoded as an array under the name "Data". Every element of this array is the description of the corresponding object.

Here we encode the direct product and different components of the product should be placed within an array. Then objects should be described by described by arrays of equal length, since they should contain the same amount of components.

Then we have two components. The first one is a description of a binary set, similar to the description of an object in the standard case. The second one is a description of an interval tuple which is an array of either numbers or intervals.

For example the first object is described by `[{"Inds":[0,2]},[1,[1,2]]]`. Here, `{"Inds":[0,2]}` is the description of the binary set `{a,c}` where attributes are encoded by their indices, and `[1,[1,2]]` is a tuple (encoded as an array) of two components corresponding to numerical number and interval.

## Settings

Here we have used the following file with settings.

```json
{
	"Type": "ContextProcessorModules",
	"Name": "AddIntentContextProcessorModule",
	"Params": {
		"PatternManager": {
			"Type" : "PatternManagerModules",
			"Name" : "CompositPatternManagerModule",
			"Params" : {
				"PMs":[
				{
					"Type" : "PatternManagerModules",
					"Name" : "BinarySetJoinPatternManagerModule"
				},{
					"Type" : "PatternManagerModules",
					"Name" : "IntervalPatternManagerModule"
				}
				]
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

Here we specify that patterns are dealled with module "CompositPatternManagerModule" of type "PatternManagerModules". 
This module construct a direct product of other pattern structures given as params to the array PMs. Every element of this array is a description of a pattern manager module providing a certain semilattice of descriptions.

We specify that the first one is a binary pattern mananger with the join as the semilattice operation and the second one is an interval tuple data.

## The file with the lattice

The resulting lattice is encoded in the following way.

```json
[
	{"NodesCount":7,"ArcsCount":9,"Bottom":[0],"Top":[4]},
	{ 
		"Nodes":[
			{"Ext":{"Count":0,"Inds":[]},"Supp":0,"Int":"BOTTOM"},
			{"Ext":{"Count":1,"Inds":[0],"Names":["g1"]},"Supp":1,"Int":[{"Count":2,"Inds":[0,2]},[[1,1],[1,2]]]},
			{"Ext":{"Count":2,"Inds":[0,1],"Names":["g1","g2"]},"Supp":2,"Int":[{"Count":1,"Inds":[0]},[[0,1],[1,2]]]},
			{"Ext":{"Count":1,"Inds":[1],"Names":["g2"]},"Supp":1,"Int":[{"Count":1,"Inds":[0]},[[0,0],[2,2]]]},
			{"Ext":{"Count":3,"Inds":[0,1,2],"Names":["g1","g2","g3"]},"Supp":3,"Int":[{"Count":0,"Inds":[]},[[0,2],[1,2]]]},
			{"Ext":{"Count":2,"Inds":[0,2],"Names":["g1","g3"]},"Supp":2,"Int":[{"Count":1,"Inds":[2]},[[1,2],[1,2]]]},
			{"Ext":{"Count":1,"Inds":[2],"Names":["g3"]},"Supp":1,"Int":[{"Count":2,"Inds":[1,2]},[[2,2],[1,1]]]}
		]
	},{ 
		"Arcs":[
			{"S":1,"D":0},
			{"S":2,"D":1},
			{"S":2,"D":3},
			{"S":3,"D":0},
			{"S":4,"D":2},
			{"S":4,"D":5},
			{"S":5,"D":1},
			{"S":5,"D":6},
			{"S":6,"D":0}
		]
	}
]
```

Basically, it is an array, where the first element is a metadata, the second element enumerates all nodes of the lattice, and third element enumerates all edges or arcs of the lattice. For every node the extent "Ext" and the intent "Int" are given. For every edges the source and destination zero-based indices of the nodes are given.
