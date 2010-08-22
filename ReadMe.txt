Dwarf Fortress 2 Minecraft (DF2MC) ReadMe
version 0.11

ABOUT:
DF2MC attempts to convert a Dwarf Fortress local area into a Minecraft map. It
does this by turning each 'square' of the Dwarf Fortress map in to a NxNxN 
group of cubes in the Minecraft map by looking up the Dwarf Fortress object and 
replacing with a configuration of cubes specified for that object in a settings
file. The default settings file converts each object in to a 3x3x3 group of 
cubes. There is the smallest group that is feasable (floor to walk on and 2 
empty spaces above), but larger sizes are possible, but require a new 
specification for each object. 

DF2MC uses the DFHack library to read the map information from a running 
DwarfFortress program.  This is the same library used by several external 
viewer programs to render views of running Dwarf Fortress games.


USE:
Start Dwarf Fortress an load the world/region you want to convert.
Pause the Dwarf Fortress game.
Run DF2MC.
	It will run for about 10 minutes, depending on home much of the 
	level is being converted and your computer's speed. Dwarf Fortress won't
	respond durring this time.
When it has finished converting the level, check if there were any 'unknown 
	objects' found during conversion. 'Unknown Objects' are objects that don't
	yet have a defination in the settings file, and so, aren't converted 
	properly. These will end up as a 'hole' in the Minecraft map.
Press enter to close the DF2MC program. Dwarf Fortress should then again 
	respond to input. If it does not, run DFunstuck which will fix the issue.
There should be 4 new files in the directory where you ran DF2MC:
	out.mclevel - the converted Minecraft level
	out.mcraw - an uncompressed version of out.mclevel, for debugging not needed
	updated.xml - an updated version of the settings file with new object types
	unimplementedobjects.xml - the full description of every object found


SETTINGS:
The settings.xml file stores all the settings for the conversion of the Dwarf 
Fortress map to the Minecraft level. The file is split into three main \
sections:
Settings
	This section holds basic information like the size of the group of cubes 
	that each DF 'square' get converted into, the size and position of the 
	section of the Dwarf Fortress map to convert, and how ofter to place
	torches in the dark sections of the Dwarf Fortress map.
MinecraftMaterials
	This sections lists the minecraft materials names and their associated
	values. This section can be updated as new block materials are added to 
	Minecraft so that they can be used in the conversion process.
Objects
	This section holds the defination for how different Dwarf Fortress objects
	should be represented in Minecraft. Each object is represented as list of
	block types that make up the group on NxNxN cubes for that location. The 
	list of block types procedes from west to east, then from north to south,
	then from top to bottom, starting from the north west corner of the top 
	layer of the group of cubes. You can use any of the characters ',', ';', 
	'|' to separate the objects, but for convenience sake, the program uses 
	',' to separate columns (west-east), ';' to separate rows (north-south), 
	and '|' to separate layers (up-down), but the program will accept a list 
	of N^3 materials separeated by any combination of those separtors.
	Objects can have several different names. The most specifically matching
	object name will be used for an object. The object names that are used are:
		class, basic material, varient, description, specific material
		class, basic material, varient, description
		class, basic material, description, specific material
		class, basic material, description 
		class, basic material, varient, specific material
		class, basic material, varient
		class, basic material, specific material
		class, basic material
	with class being one of:
		"empty"
		"wall"
		"pillar"
		"forticication"
		"stairup"
		"stairdown"
		"stairupdown"
		"ramp"
		"ramptop"
		"floor"
		"treedead"
		"tree"
		"saplingdead"
		"sapling"
		"shrubdead"
		"shrub"
		"boulder"
		"pebbles"
		"treetop"
		"building"
	and basic material being one of:
		"air"
		"soil"
		"stone"
		"featstone"
		"obsidian"
		"vein"
		"ice"
		"grass"
		"grass2"
		"grassdead"
		"grassdry"
		"driftwood"
		"hfs"
		"magma"
		"campfire"
		"fire"
		"ashes"
		"constructed"
		"cyanglow"
	Ramps, Stairs and Buildings are handled a bit differently. 
	Stairs have a number append to their class name to indicate the level mod 4
	(the remainder after the level they are on is divided by 4). This allows 
	you to define stairs in such a way that tall up-down stair stacks result in
	a spiral staircase in Minecraft, completing a full twist every 4 levels or 
	every 2 levels (by having stair 0 be the same as stair 2 and stair 1 be the 
	same as stair 3, which is the default implementation).
	Ramps will have a series of numbers possibly appended to their class name
	to indicate the sides of the ramp that have walls (the high sides). The 
	numbers are as follow:
		NW N  NE	1 2 3
		W ramp E	4 5 6
		SW S  SE	7 8 9
	The number 5 will never actually be used, as that is the location the ramp
	will be in. An unsupported ramp will just have a class name of 'ramp'. 
	Ramps will first attempt to find a match using the exact number of the 
	surrounding walls. If no match is found, it then attempts to use the 
	numbers for only the walls in the 4 cardinal directions (2,4,6,8). If 
	still no match is found, it will use an unsupported ramp for that location.
	Buildings typically span several squares. The description field is used to
	describe which square of the building this location is. It will describe
	the X and Y offset from the top left corner which is 'x0y0'. The final row
	and colum are described as 'max' with the bottom right corner being 
	'xmaxymax'. In cases of single location buildings, like doors, tables, etc.
	the description is 'only'. For narrow building, like some bridges or piles,
	the x or y may be described as 'only' while the other increments if it is 
	both the min and max of that direction. For example, and 3x1 stockpile
	would have locations described as 'x0yonly', 'x1yonly' and 'xmaxyonly'. The
	default settings file uses this to have stockpiles outlined by torches.



ISSUES:
Lighting isn't calculated at all. The game calculates the lighting over the first
few minutes of play, but it starts completely dark.
Not all buildings are described in the settings file yet, and so they aren't all
converted with the default settings file.
Tree and plant types aren't currently determined, ans do they all look the same.
Files outputted only work in Minecraft Indev. An Infdev save routine is yet to be
written.
A out.mcraw file (uncompressed version of the level) is left in the program 
directory. The isn't needed and can be deleted.
Running the program again overwrites the output file without a warning.




Compiling:
I'm not good with setting up separate projects - hopefully someone can help me out 
with that.
This code should be able to be compiled where ever DFHack can be compiled.
Requirements:
DFHack 0.4.0.5 (not tested with 7b yet)
TinyXml (also used by DFHack - I used the version that DFHack was using)
Zlib (I used 1.2.5)




Technical stuff:

Things to implement
√ a ReadMe file
√ Put the spawn in the center of the map, three cubes above the highest piece of 
	ground in that column, or where inital wagon was (if still in chosen output
	area), if my assumption that it is always the certer of the map is wrong
√ add a layer of adminium at the bottom of the level
√ implement a way to specify what x and y area to output 
√ implement several ways to 'cut down' the level so that it is playable (z direction)
	√ none - output whole level (already implemented)
	√ top - top N levels will be output - gives room to build upwards - option to
		specify how much air (in DF levels (minimum 1)) to keep
	√ smart - scans the world, marking 'interesting' levels and outputs the top 
		42 interesting levels interesting levels are one that aren't all air,
		and aren't all wall (for the cut down x and y area) if room, possibly
		keep one or more 'uninteresting' levels between interesting levels
			how to deal with repetitive Stair layers / pump stack layers?
√ change ramps to use 1-9 instead of NSEW and implement diagnal only ramps, have 
	fallback to 2468 if not found and again to just 'ramp'
√ implement material look-up for (wall, ramp, stair, floor) constructions
√ implement buildings (workshops, doors, floodgates, etc)
* implement objects (needs DFHack support for reporting on objects) - probably only 
	implement barrels, bins and cages (maybe coins to gold cube)
√ make sure liquid conversion is working properly (noticed an air gap in water, but 
	may have been because water - ice interaction
* figure out tree and shrub types and replace stone material with shrub / tree type 
* replace dirt around shrub / tree with grass unless area is muddy
* properly calculate inital lighting
* setup a project of some source code hosting site
* setup project like DFHack is setup so that different compilers can be used from cmake
	scripts or similar
√ rotate .mclevel output so that sun rises in east and set is west / clouds go north, so
	it matches assumed DF north is top of screen
* implement infdev file output
* convert mud splatter to wood pressue plates, snow splatter to snow thin layer (78?), 
	and other splatter to stone pressure plates ?