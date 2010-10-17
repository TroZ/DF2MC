Dwarf Fortress 2 Minecraft (DF2MC) ReadMe
version 0.7

ABOUT:
DF2MC attempts to convert a Dwarf Fortress local area into a Minecraft map. It
does this by turning each 'square' of the Dwarf Fortress map in to a NxNxN 
group of cubes in the Minecraft map by looking up the Dwarf Fortress object and 
replacing with a confugration of cubes specified for that object in a settings
file. The default settings file converts each object in to a 3x3x3 group of 
cubes. This is the smallest group that is feasable (floor to walk on and 2 
empty spaces above), but larger sizes are possible, but require a new 
specification for each object. 

DF2MC uses the DFHack library to read the map information from a running 
DwarfFortress program.  This is the same library used by several external 
viewer programs to render views of running Dwarf Fortress games.

NEW for 0.7:
Directional walls and buildings. Diagnal hallways and other surrounding wall or
object specific object modifications are now possible. For example, chairs will
face neighboring tables or away for walls instead of always facing north as
they previously did. Chairs in corners will be angled.
Proper material support.  Item materials are now stored separatly in the 
settings file from the Item shape. This allows a Microcline table to look 
similar to a Microcline table. I have unique definations for every Stone, Ore, 
Soil, Sand, Metal Bar, and several other objects (potash, coal, etc.) Gems also
have the correct color and you can tell their worth (Ornamental, semi-presious,
precious, rare) just from looking at them (more info than DF will give you 
without using loo(k) ), although gems of the same color and worth are converted
identically currently (although you should be able to tell many apart from the 
stone they are in. It is possible to give everything unique conversions, I just
haven't. All materials attempt to replicate the color of the object in DF. For
Stone, the center block of each face indicates the rock 'class' (Sedementary = 
Rock, Igneous = Obsidian, Metamorphic = Cobble). Metals: 'Monitary' metals are
mostly gold blocks, where as usefull metals are mostly iron blocks. Pure metals
have top and bottom layers of only Iron or Gold blocks, their center layers 
being different, while Alloy metals have a different blocks in their top and
bottom layers as well.  Admantine is made out of all Diamond Blocks, its ore is 
pure Redstone. Ores have a similar system. The more usefull ores have more iron
in them where the more valuable ones have more gold. Constructed stone walls
should have a cobble center (natural is a smooth rock center), but this isn't 
apparent without breaking things. In general, diamond, redstone and gold ore, 
if seen in the corners of of a block are used to indicate blue, red and yellow
colaration respectively, and aren't necessarily indication the DF block is an
ore. Occasionally redstone and diamond are used together to indicate purple, 
kimberlite being an example.
With the new material system, sand and gravel are used for various sands 
(together and separtely) and gravel is also used in various ashes (ash, potash,
etc.). However, this causes issues if you have such material over an open space
(as sand and gravel are the only materials that fall if air is below them). 
There is now an option (default on) to replace sand and gravel in such 
situations with dirt. (I also originaly had the soil 'peat' made out of dirt 
and leaves, but that caused other problems with most of the leaves disappearing
or turning into saplings. This is also the reason green glass uses mossy cobble
instead of leaves).




USE:
Start Dwarf Fortress an load the world/region you want to convert.
Pause the Dwarf Fortress game.
Run DF2MC.	It will run for about 10 minutes, depending on home much of the 
	level is being converted and your computer's speed. Dwarf Fortress won't
	respond durring this time.
When it has finished converting the level, check if there were any 'unknown 
	objects' found during conversion. 'Unknown Objects' are objects that don't
	yet have a defination in the settings file, and so, aren't converted 
	properly. These will end up as a 'hole' in the Minecraft map.
Press enter to close the DF2MC program. Dwarf Fortress should then again 
	respond to input. If it does not, run DFunstuck which will fix the issue.
There should be 3 new files in the directory where you ran DF2MC:
	out.mcraw - an uncompressed version of the level, for debugging not needed
	updated.xml - an updated version of the settings file with new object types
	unimplementedobjects.xml - the full description of every object found
additionally, there will be a new directory if doing a conversion to Minecraft 
Alpha (the default) which contains the save or a file called'out.mclevel', the 
converted Minecraft level for Minecraft Indev.  The Alpha directory should be 
moved to your Minecraft save directory and renamed to WorldX, where X is 1-5.
Be careful to not delete or overwrite a Mincraft world you care about.


SETTINGS:
The settings.xml file stores all the settings for the conversion of the Dwarf 
Fortress map to the Minecraft level. The file is split into three main \
sections:
Settings
	This section holds basic information like the size of the group of cubes 
	that each DF 'square' get converted into, the size and position of the 
	section of the Dwarf Fortress map to convert, and how often to place
	torches in the dark sections of the Dwarf Fortress map.
MinecraftMaterials
	This sections lists the minecraft materials names and their associated
	values. This section can be updated as new block materials are added to 
	Minecraft so that they can be used in the conversion process.
DwarfFortressMaterials
	The Minecraft cubes for a solid wall of a particular Dwarf Fortress material
	are defined in this section. Each object is represented as list of block types
	that make up the group on NxNxN cubes for that location. The list of block 
	types procedes from west to east, then from north to south, then from top to 
	bottom, starting from the north west corner of the top layer of the group of 
	cubes. You can use any of the characters ',', ';', '|' to separate the 
	objects, but for convenience sake, the program uses ',' to separate columns 
	(west-east), ';' to separate rows (north-south), and '|' to separate layers 
	(up-down), but the program will accept a list of N^3 materials separeated by 
	any combination of those separtors. 
	Objects can have several different names. The most specifically matching
	object name will be used for an object.  The object names that are used are:
	 basic material, varient, description, specific material-really specific
	 basic material, varient, description, specific material
	 basic material, varient, description
	 basic material, description, specific material-really specific
	 basic material, description, specific material
	 basic material, description 
	 basic material, varient, specific material-really specific
	 basic material, varient, specific material
	 basic material, varient
	 basic material, specific material-really specific
	 basic material, specific material
	 basic material
	In general, 'really specific' is the material type used for a construction,
	while constructions have 'specific material' as the form (bars, blocks,
	stone, logs, etc.) Non-constructions don't use 'really specific'.
Terrian
	This section holds the defination for how different Dwarf Fortress objects
	should be SHAPED in Minecraft. Similar to the above material list, each object
	is represented as list of NxNxN cubes for that location. However, here you can
	use '-1' to indicate to use the cube type from the material of the object, and
	you cna use '0' to indicate to leave that location empty (air). In general, 
	you don't won't want to use specific material name here, although you can, and 
	those locations will always have that block type there no matter the material
	this location is made of.
	The object names that are used are:
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
		"boulder"
		"pebbles"
		"treetop"
		"building"
	Ramps and Stairs are handled a bit differently. 
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
	Additionally, 'data' can be specified for each block. Data is a number from
	0 to 15 which effects how some blocks are displayed, mostly indicating a 
	facing direction for blocks that can face a particular direction such as
	torches, furnaces or doors.
Flows
	Similar to terrain (all the following sections are), specifies how liquids
	should be displayed. Valid names are currently 'water' and 'magma' both with a
	period (.) and the depth (1-7) appended to the name. Material and data specified
	as in the terrain section
Plants
	Similar to rerrain, this section defines the plants that make up the map.
	Valid names consist of:
		"treedead"
		"tree"
		"treetop"
		"saplingdead"
		"sapling"
		"shrubdead"
		"shrub"
	with the specific plant name append the the name following a period (.).
Buildings
	Similar to terrain, specifies how the various buildings in the game are 
	converted. This section can get a bit large and complicated do to some 
	buildings covering multiple tiles, and some not having fixed sized.
	Vaild name vary, but generally consist of one of two forms:
	<name>.<facing direction>
	or
	<name>.x<Xpos>y<Ypos>
	I believe it possible to combine both facing direction and position (with 
	facing coming before position), this this has bee tested and isn't currently 
	used by the default settings.
	For building that cover multiple tiles (workshops for example), all positions
	use the smae name, but the position varies. The position varies from x0y0 (the
	upper left or North West corner and increases to the bottom right or South 
	East. The las row and column, however, does not use a number, replacing the
	highest row and column number with the word 'max'. This make workshops a bit
	harder to define, but this makes variable size buildings such as stockpiles or
	bridges easy to define with a border that differs from the center (otherwise,
	you would need to specify the size of that perticular version of the building
	as well as the location within that sized building. Any location that doesn't
	match a specific location within the building just uses the non-positioned
	version of that building's specification.  For example, bridges edges are 
	defined to have an alternating adge of 1/2 block, torch, 1/2 block on a base
	floor of the material that the bridge is made out of, while the center of the
	bridge just has the floor, as that is how the bridge entry without positioning
	info reads.  There is also special case postition information for very narrow
	in one direction buildings (width or height of 1). In those cases, the x or y
	(or both) is set to 'only'.
	Facing indicate the direction of immediatly surrounding building of a 
	particular type, or of immediatly surrounding walls. The building's default
	(undirectioal version) should have a property of which type of surrounding
	buildings to look for.  If any are found, their direction(s) are indicated in 
	a similar manner as ramps are, but appending string consisting of the numbers 
	1 to 9. If there are no surrounding buildings or no match for the specific 
	arrangement is found, surrounding walls are searched for. If walls are found,
	this is indicated in a similar manner, however this number is negative.
	If no match is found for either arrangement, the default (undirectional)
	specification of the object is used.



There is a separate execuitable for converting 40d maps. It has similar 
features but there are some differenes.  I'm not sure how to tell if a 
construction is made out of stones or blocks, but metal bars and logs should
be shown correctly. There is some issue preventing wells from appearing, but
there will be a hole where they should be locted. On the slightly possitive
side, I can determine the fortress name, and so the save file is named after 
the fortress converted, no more out.mclevel.




ISSUES:
A out.mcraw file (uncompressed version of the level) is left in the program 
directory. The isn't needed and can be deleted.
Running the program again in Indev mode overwrites the output file without a 
warning.




Compiling:
I'm not good with setting up separate projects - hopefully someone can help me out 
with that.
This code should be able to be compiled where ever DFHack can be compiled.
Requirements:
DFHack 0.4.0.7b (current version) - DFHack 0.2.1 (40d version)
TinyXml (also used by DFHack - I used the version that DFHack was using)
Zlib (I used 1.2.5)




Technical stuff:

Things to implement
√ a ReadMe file
√ Put the spawn in the center of the map, three cubes above the highest piece of ground in that column, or where inital wagon was (if still in chosen output area), if my assumption that it is always the certer of the map is wrong
√ add a layer of adminium at the bottom of the level
√ implement a way to specify what x and y area to output (somewhat implemented)
√ implement several ways to 'cut down' the level so that it is playable (z direction)
	√ none - output whole level (already implemented)
	√ top - top N levels will be output - gives room to build upwards - option to specify how much air (in DF levels (minimum 1)) to keep
	√ smart - scans the world, marking 'interesting' levels and outputs the top 42 interesting levels
		interesting levels are one that aren't all air, and aren't all wall (for the cut down x and y area)
		if room, possibly keep one or more 'uninteresting' levels between interesting levels
			how to deal with repetitive Stair layers / pump stack layers?
√ change ramps to use 1-9 instead of NSEW and implement diagnal only ramps, have fallback to 2468 if not found and again to just 'ramp'
√ implement material look-up for (wall, ramp, stair, floor) constructions (this was this initial version, precious all constructions were cobble - this has since been replaced with the current material lookup separate from shape lookup)
√ implement buildings (workshops, doors, floodgates, etc)
* implement objects (needs DFHack support for reporting on objects) - probably only implement barrels, bins and cages (maybe coins to gold cube)
√ make sure liquid conversion is working properly (noticed an air gap in water, but may have been because water - ice interaction
√ figure out tree and shrub types and replace floor material with shrub / tree type 
- replace dirt around shrub / tree with grass unless area is muddy (no longer going to do as grass grows in alpha)
√ properly calculate inital lighting
√ setup a project of some source code hosting site
* setup project like DFHack is setup so that different compilers can be used from cmake scripts or similar
√ rotate .mclevel output so that sun rises in east and set is west / clouds go north, so it matches assumed DF north is top of screen
√ implement infdev/alpha file output
- convert mud splatter to wood pressue plates, snow splatter to snow thin layer (78?), and other splatter to stone pressure plates ? (pressure plates get annoying, and setting snowy biome takes care of snow spatter ;) )
√ possibly add directions to buildings - eg. align beds with the headboard against the wall, align chairs to face tables, doors to surrounding walls (for alpha).
√ Support directional walls for diagonal passages (only wall implemented in settings, but fortification can also be added of wanted/needed)
√ Separate object shapes from material defination so that one material defination can be shaped into different constructions and buildings.
* create method to add fortress to an existing world (will need to possibly use an NBT Library to read existing chunks, or maybe only add to unexplored area)
* possibly convert to DF Block / chunk based output method for alpha (possibly means removing indev support) so larger areas can be converted without using much memory (won't need to keep entire map in memory), only current chunk / this plus surrounding for lighting.
