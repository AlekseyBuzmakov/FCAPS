# An example of a lattice from a binary context

Here we provide an example of json file for a lattice built from a binary context. The binary context corresponds to a well known example on ['Live in Water'](http://www.upriss.org.uk/fca/examples.html). This lattice is built with the settings from [a special file](https://github.com/AlekseyBuzmakov/FCAPS/blob/master/FCAPS/EXAMPLES/StdFCA/params.md). The dataset is given by the context file ([see further for more details](https://github.com/AlekseyBuzmakov/FCAPS/blob/master/FCAPS/EXAMPLES/StdFCA/context.md)). The computation starts with the following command:

$sofia-ps -CP:[params.json](https://raw.githubusercontent.com/AlekseyBuzmakov/FCAPS/master/FCAPS/EXAMPLES/StdFCA/params.json) -out

The result lattice image is taken from the following [site](http://www.upriss.org.uk/fca/examples.html).
![A lattice visualisation](resourses.md/liveinwater-lattice.png)

```json
[{
	"NodesCount":20,
	"ArcsCount":36,
	"Bottom":[0],
	"Top":[4]
},{ 
	"Nodes":[
		{
			"Ext":{"Count":0,"Inds":[]},
			"Stab":"inf",
			"Int":"BOTTOM"
		},
		{
			"Ext":{"Count":3,"Inds":[0,1,2],"Names":["fish leech","bream","frog"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":3,"Names":["needs water to live","lives in water","can move"]}
		},
		{
			"Ext":{"Count":2,"Inds":[1,2],"Names":["bream","frog"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":4,"Names":["needs water to live","lives in water","can move","has limbs"]}
		},
		{
			"Ext":{"Count":1,"Inds":[2],"Names":["frog"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":5,"Names":["needs water to live","lives in water","lives on land","can move","has limbs"]}
		},
		{
			"Ext":{"Count":8,"Inds":[0,1,2,3,4,5,6,7],"Names":["fish leech","bream","frog","dog","water weeds","reed","bean","corn"]},
			"LStab":1.2995602818589079,"UStab":3,"Stab":1.8300749985576877,
			"Int":{"Count":1,"Names":["needs water to live"]}
		},
		{
			"Ext":{"Count":3,"Inds":[1,2,3],"Names":["bream","frog","dog"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":2,"Names":["needs water to live","has limbs"]}
		},
		{
			"Ext":{"Count":2,"Inds":[2,3],"Names":["frog","dog"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":3,"Names":["needs water to live","lives on land","has limbs"]}
		},
		{
			"Ext":{"Count":1,"Inds":[3],"Names":["dog"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":5,"Names":["needs water to live","lives on land","monocotyledon","has limbs","breast feeds"]}
		},
		{
			"Ext":{"Count":5,"Inds":[0,1,2,4,5],"Names":["fish leech","bream","frog","water weeds","reed"]},
			"LStab":1.0,"UStab":2,"Stab":1.415037499278844,
			"Int":{"Count":2,"Names":["needs water to live","lives in water"]}
		},
		{
			"Ext":{"Count":4,"Inds":[3,4,5,7],"Names":["dog","water weeds","reed","corn"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":2,"Names":["needs water to live","monocotyledon"]}
		},
		{
			"Ext":{"Count":2,"Inds":[4,5],"Names":["water weeds","reed"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":4,"Names":["needs water to live","lives in water","needs chlorophyll","monocotyledon"]}
		},
		{
			"Ext":{"Count":5,"Inds":[2,3,5,6,7],"Names":["frog","dog","reed","bean","corn"]},
			"LStab":0.4150374992788437,"UStab":2,"Stab":1.0931094043914816,
			"Int":{"Count":2,"Names":["needs water to live","lives on land"]}
		},
		{
			"Ext":{"Count":2,"Inds":[2,5],"Names":["frog","reed"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":3,"Names":["needs water to live","lives in water","lives on land"]}
		},
		{
			"Ext":{"Count":3,"Inds":[3,5,7],"Names":["dog","reed","corn"]},
			"LStab":0.4150374992788438,"UStab":1,"Stab":0.6780719051126377,
			"Int":{"Count":3,"Names":["needs water to live","lives on land","monocotyledon"]}
		},
		{
			"Ext":{"Count":1,"Inds":[5],"Names":["reed"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":5,"Names":["needs water to live","lives in water","lives on land","needs chlorophyll","monocotyledon"]}
		},
		{
			"Ext":{"Count":4,"Inds":[4,5,6,7],"Names":["water weeds","reed","bean","corn"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":2,"Names":["needs water to live","needs chlorophyll"]}
		},
		{
			"Ext":{"Count":3,"Inds":[5,6,7],"Names":["reed","bean","corn"]},
			"LStab":0.4150374992788438,"UStab":1,"Stab":0.6780719051126377,
			"Int":{"Count":3,"Names":["needs water to live","lives on land","needs chlorophyll"]}
		},
		{
			"Ext":{"Count":1,"Inds":[6],"Names":["bean"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":4,"Names":["needs water to live","lives on land","needs chlorophyll","dicotyledon"]}
		},
		{
			"Ext":{"Count":3,"Inds":[4,5,7],"Names":["water weeds","reed","corn"]},
			"LStab":3.344763702117781e-17,"UStab":1,"Stab":0.4150374992788438,
			"Int":{"Count":3,"Names":["needs water to live","needs chlorophyll","monocotyledon"]}
		},
		{
			"Ext":{"Count":2,"Inds":[5,7],"Names":["reed","corn"]},
			"LStab":1.0,"UStab":1,"Stab":1.0,
			"Int":{"Count":4,"Names":["needs water to live","lives on land","needs chlorophyll","monocotyledon"]
		}}
	]
},{ 
	"Arcs":[
		{"S":1,"D":2},
		{"S":2,"D":3},
		{"S":3,"D":0},
		{"S":4,"D":5},
		{"S":4,"D":8},
		{"S":4,"D":9},
		{"S":4,"D":11},
		{"S":4,"D":15},
		{"S":5,"D":2},
		{"S":5,"D":6},
		{"S":6,"D":3},
		{"S":6,"D":7},
		{"S":7,"D":0},
		{"S":8,"D":1},
		{"S":8,"D":10},
		{"S":8,"D":12},
		{"S":9,"D":13},
		{"S":9,"D":18},
		{"S":10,"D":14},
		{"S":11,"D":6},
		{"S":11,"D":12},
		{"S":11,"D":13},
		{"S":11,"D":16},
		{"S":12,"D":3},
		{"S":12,"D":14},
		{"S":13,"D":7},
		{"S":13,"D":19},
		{"S":14,"D":0},
		{"S":15,"D":16},
		{"S":15,"D":18},
		{"S":16,"D":17},
		{"S":16,"D":19},
		{"S":17,"D":0},
		{"S":18,"D":10},
		{"S":18,"D":19},
		{"S":19,"D":14}
	]
}]
```
