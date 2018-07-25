# Computation of the Pattern Lattice for a general Sequential Formal Context

In this example we consider how one can use SOFIA application for building the pattern lattice for a general sequential formal context.
By general here we mean that the alphabet of sequences is an ordered set, more precisely a lattice. Thus a substring **ss** of a sequence **s** in this case is constituted from symbols different from the symbols found in **s**. However all symbols of **ss** should be 'more (or equal) general' than the symbols in **s**. 

Such pattern structures were used for mining patient hospitalisation trajectories in the following works.

> [1] A. Buzmakov, E. Egho, N. Jay, S. O. Kuznetsov, A. Napoli, and C. Raïssi, “On mining complex sequential data by means of FCA and pattern structures,” Int. J. Gen. Syst., vol. 45, no. 2, pp. 135–159, 2016.

> [2] A. Buzmakov, E. Egho, N. Jay, S. O. Kuznetsov, A. Napoli, and C. Raïssi, “On Projections of Sequential Pattern Structures (with an application on care trajectories),” in Proc. 10th International Conference on Concept Lattices and Their Applications, 2013, pp. 199–208.

Here we consider how to compute the lattice for the running example that was introduced in [1] and [2]. 
For understanding of this example it is essential to understand [the example on pattern structures with simple sequences](https://github.com/AlekseyBuzmakov/FCAPS/blob/master/FCAPS/EXAMPLES/Lattice-Computation-for-simple-Sequential-Context.md). In this section we do not explain the parts that are the same to the aforementioned example.

## Sequential data

Imagine that we have medical trajectories of patients, i.e. sequences of hospitalizations,
where every hospitalization is described by a hospital name and a set of procedures.
An example of sequential data on medical trajectories with three patients is given in
the table bellow. 

|Patient|Hospitalization Trajectory|
|---|---|
|P1|\< [H1,{a}], [H1,{c,d}], [H1,{a,b}], [H1,{d}] \>|
|P2|\< [H2,{c,d}], [H3,{b,d}], [H3,{a,d}] \>|
|P3|\< [H4,{c,d}], [H4,{b}], [H4,{a}], [H4,{a,d}] \>|

We have a set of procedures **P = {a, b, c, d}** and a set of hospital names **T={H1 , H2 , H3 , H4 , CL, CH, ∗}**, where hospital names are hierarchically organized (by the level of generality). 
**H1** and **H2** are central hospitals **CH**, **H3** and **H4** are clinics **CL** and **∗** denotes
the root of this hierarchy. 
Every hospitalization is described by one hospital
name and may contain several procedures. The procedure order in each hospitalization
is not given in our case. 

For example, the first hospitalization **[H2,{c, d}]** for the patient **P2** was a stay in hospital **H2** and during this hospitalization, the patient underwent procedures **c** and **d**.

## Pattern lattice

Given the aforementioned dataset, we are going to construct the following pattern lattice.

{FIG:TODO}

In this lattice, the intents are taken from the following semilattice. Every element is a set of sequences, while the semilattice operation is the set of maximal substrings (without gaps) that are included by at least one sequence from every set.

## Command line

One can construct this lattice by the following command line.

> $sofia-ps -CP:[settings.json](https://github.com/AlekseyBuzmakov/FCAPS/raw/master/FCAPS/EXAMPLES/AddIntentContextProcessor-for-general-Sequential-Data.json) -data:[context.json](https://github.com/AlekseyBuzmakov/FCAPS/raw/master/FCAPS/EXAMPLES/general-Sequential-Context.json) -out

Where context.json encodes the dataset, and setting.json describes the processing params.

The resulting lattice can be found [here](https://github.com/AlekseyBuzmakov/FCAPS/raw/master/FCAPS/EXAMPLES/Lattice-for-general-Sequential-Context.json).

## Data encoding

Data is encoded in the following way

```json
[{
	"ObjNames":["P1","P2","P3"]
},{
	"Count": 3,
	"Data": [
		[[ ["H1",{"Inds":[1]}],["H1",{"Inds":[3,4]}],["H1",{"Inds":[1,2]}],["H1",{"Inds":[4]}] ]],
		[[ ["H2",{"Inds":[3,4]}],["H3",{"Inds":[2,4]}],["H3",{"Inds":[1,4]}] ]],
		[[ ["H4",{"Inds":[3,4]}],["H4",{"Inds":[2]}],["H4",{"Inds":[1]}],["H4",{"Inds":[1,4]}] ]]
	]
}]

```

Basically every json dataset is an array, where the first element of the array is an object with metadata, and the second object in the array is the data.
As metedata we give here the names of patients. The data is encoded as an array under the name "Data". Every element of this array is the description of the corresponding patient. 
Hospitals are encoded here by their names (the appropriate structure for a taxonomy pattern manager as we will see). The medical procedures are encoded here as sets of procedure identifiers.

We should notice here that, "Data" is an array of arrays. Each such an array contains only one subarray, i.e., the sequence itself. This two embedded arrays for every patient is needed, since a description of a patient and the intents can contain several sequences. However, in our particular example every patient is described by only one sequence.

We have noticed that sequence is an array. Elements of this array are also arrays (the appropriate structure for the composite pattern manager). Every element encode a tuple of two components. The first component is the name of hospital (the reference to the taxonomy is given from the taxonomy pattern manager). The second component is the set of attributes (the appropriate data structure for the standard FCA pattern manager). We will discuss the datatypes in more details in the next section.

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
					"Name": "GeneralStringPartialOrderComparatorModule",
					"Params": {
```
_Here the discussing part starts. Check [simple sequential data](https://github.com/AlekseyBuzmakov/FCAPS/blob/master/FCAPS/EXAMPLES/Lattice-Computation-for-simple-Sequential-Context.md) for the rest of the settings._
```json
						"SymbolComparator":{
							"Type": "PatternManagerModules",
							"Name": "CompositPatternManagerModule",
							"Params": {
								"PMs":[{
									"Type": "PatternManagerModules",
									"Name": "TaxonomyPatternManagerModule",
									"Params" : {
										"TreePath" : "./hospital-taxonomy.json"
									}
								},{
									"Type" : "PatternManagerModules",
									"Name" : "BinarySetJoinPatternManagerModule",
									"Params" : {
										"UseInds" : true
									}
								}]
							}
```
_Here the discussing part ends._
```json
						},
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
This module can process any pattern structure by algorithm AddIntent. And as in the simple sequential data define the parameters of this module.

However, the partial order of sequences is given by module "GeneralStringPartialOrderComparatorModule" instead of module "DwordStringPartialOrderComparatorModule".
This module can deal with general strings with an additional partial order on alphabet symbols. However for this partial order module one should provide additional parameters.

Parameter "SymbolComparator" is the description of a module of type pattern manager, since  elements of general sequences in the case of our application form a semilattice. And pattern manager module describes exactly a semilattice.

Here we use module "CompositPatternManagerModule" as the pattern manager. This module describes a semilattice that is just a direct product of other semilattices.
The multiplied semilattices are given as an array under the key "PMs".

The first semilattice is a taxonomy. And the path to the associated taxonomy is given under the key "TreePath". The content of this taxonomy file is as follows.

```json
{
	"Root": 0,
	"Nodes": [
	{"ID":"*"},
	{"ID":"CH","Parent":0},
	{"ID":"CL","Parent":0},
	{"ID":"H1","Parent":1},
	{"ID":"H2","Parent":1},
	{"ID":"H3","Parent":2},
	{"ID":"H4","Parent":2}
	]
}
```

The second semilattice is a classical semilattice of the powerset of attributes. By providing just this pattern manager to the parameters of AddIntent one obtains the standard FCA scheme.

## The file with the lattice

The resulting lattice can be found [here](https://github.com/AlekseyBuzmakov/FCAPS/raw/master/FCAPS/EXAMPLES/Lattice-for-general-Sequential-Context.json).
Basically, it is an array, where the first element is a metadata, the second element enumerates all nodes of the lattice, and third element enumerates all edges or arcs of the lattice. For every node the extent "Ext" and the intent "Int" are given. For every edges the source and destination zero-based indices of the nodes are given.
