/*
DF2MC40d version 0.2
Dwarf Fortress To Minecraft
Converts Dwarf Frotress Game Maps into Minecraft Game Level for use as a
Dwarf Fortress 3D visulaizer or for creating Minecraft levels to play in.

Copyright (c) 2010 Brian Risinger (TroZ)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


A copy of the license is also available at:
http://www.gnu.org/licenses/old-licenses/gpl-2.0.html


This source code has a project at:
http://github.com/TroZ/DF2MC

*/
#include <iostream>
#include <string.h> 
#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <algorithm>
#include <time.h>

using namespace std;


#define DFHACK_WANT_MISCUTILS
#define DFHACK_WANT_TILETYPES
#include <DFHackAPI.h>
#include <DFTypes.h>
#include <DFTileTypes.h>
#include <DFMemInfo.h>
#include <tinyxml/tinyxml.h>

#include "zlib-1.2.5\zlib.h"


#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  include <sys\stat.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK (1024*256)


static uint32_t SQUARESPERBLOCK = 16;//number of squares per DF block

std::map<std::string,uint8_t> mcMats;		//Minecraft material name to id
std::map<int,int> dfMat2mcMat;			//DF Material name to minecraft id
std::map<std::string,uint8_t*> objects;	//object description string to minecraft material array of size 2 * squaresize * squaresize * squaresize; 0 is material, 1 is data

TiXmlElement *settings;
TiXmlElement *materialmapping;
TiXmlElement *xmlobjects;

const char TileClassNames[18][16] ={
	"empty",
	"wall",
	"pillar",
	"forticication",
	"stairup",
	"stairdown",
	"stairupdown",
	"ramp",
	"ramptop",
	"floor",
	"treedead",
	"tree",
	"saplingdead",
	"sapling",
	"shrubdead",
	"shrub",
	"boulder",
	"pebbles"
};
const char TileMaterialNames[19][16]={
	"air",
	"soil",
	"stone",
	"featstone",
	"obsidian",
	"vein",
	"ice",
	"grass",
	"grass2",
	"grassdead",
	"grassdry",
	"driftwood",
	"hfs",
	"magma",
	"campfire",
	"fire",
	"ashes",
	"constructed",
	"cyanglow"
};

const char* Months[13][16]={
	"Granite",
	"Slate"
	"Felsite",
	"Hematite",
	"Malachite",
	"Galena",
	"Limestone",
	"Sandstone",
	"Timber",
	"Moonstone"
	"Opal",
	"Obsidian",
	"INVALID"
};


struct myBuilding {
	char type[256];
	char desc[16];
    DFHack::t_matglossPair material;
};


char *airarray=NULL;
char *intarray=NULL;

bool createUnknown = true;

#define LIMITNONE	0
#define LIMITTOP	1
#define LIMITRANGE	2
#define LIMITSMART	3

//settings
int squaresize=3;//number of minecraft blocks per DF square
int torchPerInside = 10;
int torchPerDark = 20;
int torchPerSubter = 30;

uint32_t limitxmin = 0;
uint32_t limitxmax = 1000;
uint32_t limitymin = 0;
uint32_t limitymax = 1000;
uint32_t limittype = LIMITNONE;
uint32_t limitlevels = 1000;
uint32_t limitairtokeep = 1000;
uint32_t limittoplevel = 0;
uint8_t limitz[1000];

//stats counters
int unknowncount = 0;
int imperfectknown = 0;
int perfectknown = 0;
int prevunseen = 0;
 

using namespace std;
using namespace DFHack;



void loadMcMats(TiXmlDocument* doc){

	printf("Loading Minecraft Materials...\n");

	TiXmlElement *elm = doc->FirstChildElement();
	while(elm!=NULL){

		if(strcmp(elm->Value(),"minecraftmaterials")==0){
			TiXmlElement *mat = elm->FirstChildElement();
			while(mat!=NULL){

				if(strcmp(mat->Value(),"material")==0){
					int val;
					mat->Attribute("val",&val);
					string name(mat->GetText());

					//printf("%s -> %d\n",name.c_str(),val);

					mcMats[name]=(uint8_t)val;

					mat=mat->NextSiblingElement();
				}
			}
		}

		elm=elm->NextSiblingElement();
	}

	printf("loaded %d materials\n\n",mcMats.size());

}

char* makeAirArray(){ 
	//makes an array representing an empty block for the current squaresize setting
	if(airarray==NULL){
		airarray=new char[1024*10];
		char *pos=airarray;
		for(int z=0;z<squaresize;z++){
			if(z>0){
				*pos='|'; pos++;
			}
			for(int y=0;y<squaresize;y++){
				if(y>0){
					*pos=';'; pos++;
				}
				for(int x=0;x<squaresize;x++){
					if(x>0){
						*pos=','; pos++;
					}
					*pos='a';pos++;
					*pos='i';pos++;
					*pos='r';pos++;
				}
			}
		}
		*pos='\0';
	}
	return airarray;
}

char* makeZeroArray(){ 
	//makes an array representing an empty block for the current squaresize setting
	if(intarray==NULL){
		intarray=new char[1024*10];
		char *pos=intarray;
		for(int z=0;z<squaresize;z++){
			if(z>0){
				*pos='|'; pos++;
			}
			for(int y=0;y<squaresize;y++){
				if(y>0){
					*pos=';'; pos++;
				}
				for(int x=0;x<squaresize;x++){
					if(x>0){
						*pos=','; pos++;
					}
					*pos='0';pos++;
				}
			}
		}
		*pos='\0';
	}
	return intarray;
}


uint8_t* makeAirArrayInt(){

	uint8_t *data = new uint8_t[2*squaresize*squaresize*squaresize];
	memset(data,0,sizeof(int8_t)*2*squaresize*squaresize*squaresize);

	return data;
}

std::vector<std::string> split(const std::string &s, const char *delim, std::vector<std::string> &elems) {
    //std::stringstream ss(s);
    //std::string item;
    //while(std::getline(ss, item, *delim)) {
    //    elems.push_back(item);
    //}

	unsigned int start = 0;
	unsigned int end = 0;
	while(end<s.length()){
		while(strchr(delim,s.c_str()[end])==NULL && end<s.length()){
			end++;
		}
		elems.push_back(s.substr(start,end-start));
		start=end+1;
		end=start;
	}

    return elems;
}


void loadObjects(){

	printf("Loading DF to MC Object Definations...\n");

	TiXmlElement *elm = xmlobjects->FirstChildElement();

	
	//iterate through elements adding them into the objects map
	while(elm!=NULL){
		bool test=false;

		const char *val = elm->Attribute("mat");
		if(val!=NULL){
			const char* name = elm->Value();
			string val = elm->Attribute("mat");
			string data = elm->Attribute("data");

			int size = squaresize*squaresize*squaresize;
			uint8_t *obj = new uint8_t[2*size];
			memset(obj,0,2*size*sizeof(uint8_t));

			//parse val
			std::vector<std::string> vals;
			split(val,",;|",vals);
			if(vals.size() != (squaresize*squaresize*squaresize) ){
				printf("Object %s doesn't have correct number of materials\n (%d expeted %d) - resetting to Air\n",name,vals.size(),(squaresize*squaresize*squaresize));
				vals.clear();
				val = makeAirArray();
				elm->SetAttribute("mat",val.c_str());
				split(val,",;|",vals);
			}

			for(unsigned int i=0;i<vals.size();i++){
				std::map<std::string,uint8_t>::iterator it;
				it = mcMats.find(vals[i]);
				if(it!=mcMats.end()){
					obj[i]=it->second;
				}else{
					printf("Unknown Minecraft Material %s in object %s - using Air\n",vals[i].c_str(),name);
					obj[i]=0;
				}
			}

			//parse data
			if(data.length()>0){
				std::vector<std::string> datas;
				split(data,",;|",datas);
				if(datas.size() != (squaresize*squaresize*squaresize) ){
					printf("Object %s doesn't have correct number of materials\n (%d expeted %d) - resetting to 0",name,datas.size(),(squaresize*squaresize*squaresize));
					datas.clear();
					data = makeZeroArray();
					elm->SetAttribute("data",data.c_str());
					split(data,",;|",vals);
				}

				for(unsigned int i=0;i<datas.size();i++){
					int8_t d=0;
					d = atoi(datas[i].c_str());
					if(d!=0){
						printf("non-zero d");
						test=true;
					}
					obj[i + (squaresize*squaresize*squaresize)] = (d&0xf)<<4;//data it the upper nibble, lighting appears to be the lower nibble.
				}
			}

			//ok, now add to objects map
			objects[name] = obj;

			//if(test){
			//	uint8_t *thing = objects.find(name)->second;
			//	printf("%s\n",name);
			//	for(int i=0;i<(size);i++){
			//		printf("%d-\t%d\t%d\n",i,thing[i],thing[i+size]);
			//	}
			//}

		}

		elm = elm->NextSiblingElement();
	}

	printf("loaded %d objects\n\n",objects.size());

}


int saveMCLevel(uint8_t* mclayers,uint8_t* mcdata,int mcxsquares,int mcysquares,int mczsquares,int cloudheightdf, int xs, int ys, int zs,char* name){

	//This saves the level in .mclevel format which is used by the indev version of Minecraft.
	//see http://www.minecraft.net/docs/levelformat.jsp
	//and http://www.minecraft.net/docs/NBT.txt
	//
	//should support 65536 x 65536 x 65536 blocks, but practically limited to ~ 512 x 512 x 128 due to memory limits of the indev client

	//ok save minecraft file as a gzipped stream
	printf("\nWritting file...");

	int of = open("out.mcraw",O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY,_S_IREAD | _S_IWRITE);

	//first write the raw file to disk - can be large - up to 1GB or more depending on DF area size.
	write(of,"\012\000\016MinecraftLevel\012\000\013Environment\001\000\025SurroundingGroundType\002\002\000\027SurroundingGroundHeight\000\004\002\000\026SurroundingWaterHeight\000\003\002\000\013CloudHeight",125);
	int cloudheight = mczsquares-(cloudheightdf*squaresize);
	write(of,((char*)&(cloudheight))+1,1);
	write(of,&(cloudheight),1);
	write(of,"\001\000\024SurroundingWaterType\010\003\000\012CloudColor\000\xff\xff\xff\003\000\010SkyColor\000\x99\xcc\xff\003\000\010FogColor\000\xff\xff\xff\001\000\015SkyBrightness\x64\000",89);//last 000 is end of environment coumpound

	//entities
	//write(of,"\011\000\010Entities\012\000\000\000\000",16);//no entities
	write(of,"\011\000\010Entities\012\000\000\000\001",16);//1 entity
	write(of,"\010\000\002id\000\013LocalPlayer\002\000\006Health\000\024\002\000\003Air\001\000\002\000\004Fire\xff\xec\003\000\005Score\000\000\000\000\005\000\014FallDistance\000\000\000\000\011\000\011Inventory\012\000\000\000\016",94);//14 entities
	short entid[] = {276,277,278,279,293,261,280,263,282,262,310,311,312,313};
	char entcnt[] = {1,	1,	1,	1,	1,	1,	64,	64,	64,	64,	1,	1,	1,	1};
	char entpos[] = {1,	2,	3,	4,	5,	6,	9,	10,	11,	12,	18,	19,	20,	21};
	for(int i=0;i<14;i++){
		write(of,"\002\000\002id",5);
		write(of,((char*)&(entid[i]))+1,1);
		write(of,&(entid[i]),1);
		write(of,"\001\000\005Count",8);
		write(of,&entcnt[i],1);
		write(of,"\001\000\004Slot",7);
		write(of,&entpos[i],1);
		write(of,"\000",1);
	}
	write(of,"\011\000\003Pos\005\000\000\000\003",11);
	float xf = (float)xs + 0.5f;
	float yf = (float)ys + 0.5f;
	float zf = (float)zs + 0.05f;
	char flt[5];
	flt[0] = *(((char*)&(xf))+3);
	flt[1] = *(((char*)&(xf))+2);
	flt[2] = *(((char*)&(xf))+1);
	flt[3] = *(((char*)&(xf))+0);
	flt[4] = '\0';
	write(of,flt,4);
	flt[0] = *(((char*)&(zf))+3);
	flt[1] = *(((char*)&(zf))+2);
	flt[2] = *(((char*)&(zf))+1);
	flt[3] = *(((char*)&(zf))+0);
	write(of,flt,4);
	flt[0] = *(((char*)&(yf))+3);
	flt[1] = *(((char*)&(yf))+2);
	flt[2] = *(((char*)&(yf))+1);
	flt[3] = *(((char*)&(yf))+0);
	write(of,flt,4);
	write(of,"\011\000\010Rotation\005\000\000\000\002\000\000\000\000\000\000\000\000",24);
	write(of,"\011\000\006Motion\005\000\000\000\003\000\000\000\000\000\000\000\000\000\000\000\000",26);
	write(of,"\000",1);

	//now for the map section
	write(of,"\012\000\003Map\002\000\005Width",14);
	write(of,((char*)&(mcxsquares))+1,1);
	write(of,&(mcxsquares),1);
	write(of,"\002\000\006Length",9);
	write(of,((char*)&(mcysquares))+1,1);
	write(of,&(mcysquares),1);
	write(of,"\002\000\006Height",9);
	write(of,((char*)&(mczsquares))+1,1);
	write(of,&(mczsquares),1);

	write(of,"\007\000\006Blocks",9);
	char size[5];
	int isize = (mcxsquares*mcysquares*mczsquares);
	size[0] = *(((char*)(&isize))+3);
	size[1] = *(((char*)(&isize))+2);
	size[2] = *(((char*)(&isize))+1);
	size[3] = *(((char*)(&isize))+0);
	size[4] = '\0';
	write(of,size,4);
	write(of,(char*)mclayers,(mcxsquares*mcysquares*mczsquares));
	delete mclayers;
	write(of,"\007\000\004Data",7);
	write(of,size,4);
	write(of,(char*)mcdata,(mcxsquares*mcysquares*mczsquares));
	delete mcdata;

	write(of,"\011\000\005Spawn\002\000\000\000\003",13);
	write(of,((char*)&(xs))+1,1);
	write(of,&(xs),1);
	write(of,((char*)&(zs))+1,1);
	write(of,&(zs),1);
	write(of,((char*)&(ys))+1,1);
	write(of,&(ys),1);
	write(of,"\000",1);//end of map coumpound

	//finally for about section
	write(of,"\012\000\005About\010\000\004Name",15);
	int len = strlen(name);
	write(of,((char*)&(len))+1,1);
	write(of,&(len),1);
	write(of,name,len);
	write(of,"\010\000\006Author",9);
	char str[256];
	strcpy(str,"DF2MC by TroZ");
	len = strlen(str);
	write(of,((char*)&(len))+1,1);
	write(of,&(len),1);
	write(of,str,len);
	__time64_t timer;
	_time64(&timer);
	write(of,"\004\000\011CreatedOn",12);
	write(of,((char*)(&timer))+7,1);
	write(of,((char*)(&timer))+6,1);
	write(of,((char*)(&timer))+5,1);
	write(of,((char*)(&timer))+4,1);
	write(of,((char*)(&timer))+3,1);
	write(of,((char*)(&timer))+2,1);
	write(of,((char*)(&timer))+1,1);
	write(of,((char*)(&timer))+0,1);
	
	write(of,"\010\000\013GeneratedBy",14);
	strcpy(str,"Dwarf Fortress To Minecraft by TroZ");
	len = strlen(str);
	write(of,((char*)&(len))+1,1);
	write(of,&(len),1);
	write(of,str,len);
	write(of,"\000\000\000",3);//this should be one too many character 0s, but seems to fix a loading issue



	close(of);


	//compress file gzip
	int ret, flush;
    unsigned have;
    z_stream strm;
	unsigned char in[CHUNK];
    unsigned char out[CHUNK];

	FILE *s = fopen("out.mcraw","r");
	if(s==NULL){
		printf("Could not open file for reading, exiting.");
		return -50;
	}
	SET_BINARY_MODE(s);


	FILE *f = fopen(name,"w");
	if(f==NULL){
		printf("Could not open file for writing, exiting.");
		return -51;
	}
	SET_BINARY_MODE(f);

	//most of the rest of this is from a zlib example

	/* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, Z_BEST_COMPRESSION,Z_DEFLATED,31,9,Z_DEFAULT_STRATEGY);//the 31 indicates gzip, set to 15 for normal zlib file header
    if (ret != Z_OK){
		printf("Unable to initalize compression routine, exiting.");
        return ret;
	}

	/* compress until end of file */
    do {
		strm.avail_in = fread(in, 1, CHUNK, s);
        if (ferror(s)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(s) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
		
		/* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, f) != have || ferror(f)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */


	/* clean up and return */
    (void)deflateEnd(&strm);

	fclose(f);
	fclose(s);


	printf("Done!\n");
	

	return 0;
}



void replacespaces(char* str){
	while(*str!='\0'){
		if(*str==' ' || (!isalnum(*str) && *str!='_' && *str!='.' && *str!='-'))
			*str='_';
		*str = tolower(*str);
		str++;
	}
}

uint32_t getMapIndex(uint32_t x,uint32_t y,uint32_t z){
	return ((z&0x1ff)<<22) | ((x&0x7ff)<<11) | (y&0x7ff); 
}


uint8_t* getObject(TiXmlElement *uio,int x, int y, int z,const char* classname,const  char* basicmaterial, int variant=0,const char* fullname = NULL,
	const char* specificmaterial = NULL,const char* reallyspecificmaterial = NULL,bool addstats=true){

	//check order
		//class, basic material, varient, description, specific material-really specific
		//class, basic material, varient, description, specific material
		//class, basic material, varient, description
		//class, basic material, description, specific material-really specific
		//class, basic material, description, specific material
		//class, basic material, description 
		//class, basic material, varient, specific material-really specific
		//class, basic material, varient, specific material
		//class, basic material, varient
		//class, basic material, specific material-really specific
		//class, basic material, specific material
		//class, basic material
#define NUM_OBJECT_CHECKS 12

	char loc[NUM_OBJECT_CHECKS][256]; //description of current block for lookup in objects
	const char* smat = NULL;
	if( NULL!=specificmaterial && specificmaterial[0]!='\0'){
		smat = specificmaterial;
	}

	
	if(smat!=NULL){
		if(reallyspecificmaterial!=NULL ){
			if(fullname!=NULL){
				_snprintf(loc[11],255,"%s.%s.%d.%s.%s-%s",	classname,	basicmaterial,	variant,	fullname,	smat,	reallyspecificmaterial);
				_snprintf(loc[8],255,"%s.%s.%s.%s-%s",	classname,	basicmaterial,	fullname,	smat,	reallyspecificmaterial);
			}else{
				loc[11][0]='\0';
				loc[8][0]='\0';
			}
			_snprintf(loc[5],255,"%s.%s.%d.%s-%s",	classname,	basicmaterial,	variant,	smat,	reallyspecificmaterial);
			_snprintf(loc[2],255,"%s.%s.%s-%s",	classname,	basicmaterial,	smat,	reallyspecificmaterial);
		}else{
			loc[11][0]='\0';
			loc[8][0]='\0';
			loc[5][0]='\0';
			loc[2][0]='\0';
		}
		if(fullname!=NULL){
			_snprintf(loc[10],255,"%s.%s.%d.%s.%s",	classname,	basicmaterial,	variant,	fullname,	smat);
			_snprintf(loc[7],255,"%s.%s.%s.%s",	classname,	basicmaterial,	fullname,	smat);
		}else{
			loc[10][0]='\0';
			loc[7][0]='\0';
		}
		_snprintf(loc[4],255,"%s.%s.%d.%s",	classname,	basicmaterial,	variant,	smat);
		_snprintf(loc[1],255,"%s.%s.%s",	classname,	basicmaterial,	smat);
	}else{
		loc[11][0]='\0';
		loc[10][0]='\0';
		loc[8][0]='\0';
		loc[7][0]='\0';
		loc[5][0]='\0';
		loc[4][0]='\0';
		loc[2][0]='\0';
		loc[1][0]='\0';
	}
	if(fullname!=NULL){
		_snprintf(loc[9],255,"%s.%s.%d.%s",	classname,	basicmaterial,	variant,	fullname);
		_snprintf(loc[6],255,"%s.%s.%s",	classname,	basicmaterial,	fullname);
	}else{
		loc[9][0]='\0';
		loc[6][0]='\0';
	}
	_snprintf(loc[3],255,"%s.%s.%d",	classname,	basicmaterial,	variant);
	_snprintf(loc[0],255,"%s.%s",	classname,	basicmaterial);


	for(int r=0;r<NUM_OBJECT_CHECKS;r++)
		replacespaces(loc[r]);

	uint8_t *object = NULL;

	//find the best description we have for the location
	int bpos=NUM_OBJECT_CHECKS-1;
	char *best = loc[bpos];
	while(best[0]=='\0' && bpos>-1){
		bpos--;
		best = loc[bpos];
	}

	//find the most descriptive object that matches the current location
	if(objects.find(best)==objects.end()){
		//no perfect match - find a good match and add perfect to list of unimplemented objects
		char* use=NULL;
		for(int l=(NUM_OBJECT_CHECKS-1);l>-1&&use==NULL;l--){
			if(loc[l][0]!='\0' && objects.find(loc[l])!=objects.end()){
				use=loc[l];
				object=objects[use];
			}
		}
		if(use!=NULL){
			//printf("location %d,%d,%d is %s\tusing %s\n",x,y,z,best,use);
			if(addstats)
				imperfectknown++;
			if(uio!=NULL){
				TiXmlElement *elm = uio->FirstChildElement(best);
				if(elm==NULL){//add perfect match to unimplemented objects
					elm = new TiXmlElement( best );  
					elm->SetAttribute("mat",makeAirArray());
					elm->SetAttribute("data","");
					uio->LinkEndChild( elm ); 
					if(addstats)
						prevunseen++;
				}
			}
		}else {
			//not even a basic object found - create a basic object for settings.xml
			if(addstats){
				printf("location %d,%d,%d is %s\tNOT FOUND!\tcreating %s as air\n",x,y,z,best,loc[0]);
				unknowncount++;

				TiXmlElement * ss = new TiXmlElement( loc[0] );  
				ss->SetAttribute("mat",makeAirArray());
				ss->SetAttribute("data","");
				xmlobjects->LinkEndChild( ss ); 

				object = makeAirArrayInt();
				objects[loc[0]] = object;
			}

			if(uio!=NULL){
				TiXmlElement *elm = uio->FirstChildElement(best);
				if(elm==NULL){//add perfect match to unimplemented objects
					elm = new TiXmlElement( best );  
					elm->SetAttribute("mat",makeAirArray());
					elm->SetAttribute("data","");
					uio->LinkEndChild( elm ); 
					if(addstats)
						prevunseen++;
				}
			}
		}

	}else{
		// perfect match found (this is probably rare) use this object
		//printf("location %d,%d,%d is %s\t\n",x,y,z,best);
		object=objects[best];
		if(addstats)
			perfectknown++;
	}

	//if(stricmp("farmplotst",basicmaterial)==0){
	//	int size=(squaresize*squaresize*squaresize);
	//	for(int i=0;i<size;i++){
	//		printf("%d\t%d\n",object[i],object[i+size]);
	//	}
	//}

	return object;
}

void addObject(uint8_t* mclayers, uint8_t* mcdata, uint8_t *object, int dfx, int dfy, int dfz, int xoffset, int yoffset, int zoffset, int mcxsquares, int mcysquares,bool overwrite=false){

	//now copy object in to mclayer array

	int mcx = dfx * squaresize - (xoffset*SQUARESPERBLOCK*squaresize);
	int mcy = dfy * squaresize - (yoffset*SQUARESPERBLOCK*squaresize);
	//int mcz = dfz * squaresize - (zoffset*squaresize);
	int mcz = (zoffset*squaresize) + 1;
	int pos = 0;
	int pos2 = squaresize*squaresize*squaresize;

	for(int oz=0;oz<squaresize;oz++){
		for(int oy=0;oy<squaresize;oy++){
			for(int ox=0;ox<squaresize;ox++){

				//Index = x + (y * Depth + z) * Width //where y is up down
				//int x = (mcx+ox);
				//int y = (mcy+oy);
				//int z = (mcz+2-oz);
				//also MC's X and Z seem rotated to the assumed DF X and Y - correcting so that sun in MC rises in DF East
				int x = (mcy+oy);
				int y = mcxsquares-(mcx+ox);
				int z = (mcz+2-oz);
				int idx = x + (z * mcysquares +y) * mcxsquares;
				if(overwrite || mclayers[idx]==0){
					mclayers[idx]=object[pos];
					mcdata[idx]=object[pos2];
					//if(object[pos2]!=0 || object[pos]==59){
					//	printf("non-zero data");
					//	int size=(squaresize*squaresize*squaresize);
					//	for(int i=0;i<size;i++){
					//		printf("%d\t%d\n",object[i],object[i+size]);
					//	}
					//}
				}
				pos++;pos2++;

			}
		}
	}

}


void getRampDir(DFHack::API *Maps,DFHack::mapblock40d *Bl,TiXmlElement *uio,char *dir,int x,int y,int z,int bx, int by,const char* mat, int varient,const char* full,const char* specmat,const char* consmat){
	//this will find and place in the string dir the letters of the high side of a ramp

	DFHack::mapblock40d *Block;
	DFHack::mapblock40d *BlockLast = Bl;
	DFHack::mapblock40d B;
	TileClass tc;

	int blockx,blocky,offsetx,offsety;
	int lastblockx = bx;
	int lastblocky = by;

	// Ramps are named with numbers to indicate which side have 'walls' - the high sides
	// other sides are assumed to be low, but may also have ramps (does this need to be handled?)
	// assuming ramp is at position 5 (which will never be shown as high) walls are numbered like this:
	// 1 2 3
	// 4 5 6
	// 7 8 9
	//
	// numbers will be listed in number order, a wall to the south, southwest and west is 478 not 874 or other possibilities
	//
	// we first try to find a defination of a ramp using all directions (numbers)
	// if we don't find a match, we then then limit to directly north, south , east, and west (2,4,6,8)
	// so if a wall to the south, southwest and west is not found as 478, we will check if 48 exists and if not, use no numbers

	//ok first we will try to find an exact match for the ramp
	uint8_t* obj = NULL;
	int step = 1;
	int start = 1;
	//this DO tries all numverf first, and only the even ones in the second pass
	do{
		char *pos=dir;

		for(int i=start;i<10;i+=step){

			if(i==5) continue;//don't check self

			int ox=((i-1)%3)-1;
			int oy=((i-1)/3)-1;
		

			blockx	=	(x+ox) / SQUARESPERBLOCK;
			offsetx =	(x+ox) % SQUARESPERBLOCK;
			blocky	=	(y+oy) / SQUARESPERBLOCK;
			offsety =	(y+oy) % SQUARESPERBLOCK;
			//see if we already have that block (we should most of the time)
			if(blockx == lastblockx && blocky == lastblocky){
				Block=BlockLast;
			}else{
				if(Maps->isValidBlock(blockx,blocky,z)){
					Maps->ReadBlock40d(blockx,blocky,z, &B);
					Block=&B;
					lastblockx = blockx;
					lastblocky = blocky;
					BlockLast = Block;
				}else{
					Block=NULL;
				}
			}
			//check location for wall or fortification
			if( Block!=NULL){ 
				tc = tileTypeTable[Block->tiletypes[offsetx][offsety]].c;
				if( tc == WALL || tc == PILLAR || tc == FORTIFICATION){
				   *pos=('0'+i);
				   pos++;
				}
			}

		}

		*pos='\0';
		//try to find a match
		char test[32];
		_snprintf(test,31,"ramp%s",dir);
		obj = getObject(uio,x,y,z,test,mat,varient,full,specmat,consmat,false);

		//setup of next DO loop, if needed
		step++;
		start++;
	}while(obj==NULL && start < 3);

	if(obj==NULL){ //no match found - use 'undirected' ramp
		dir[0]='\0';
	}
	
}

int findLevels(DFHack::API *Maps,int xmin,int xmax,int ymin,int ymax,int zmax,int typeToFind){

	DFHack::mapblock40d Block;
	int count = 0;

	for(int z=0;z<zmax;z++){

		bool found = false;

		for(int x=xmin;(x<xmax)&&(!found);x++){
			for(int y=ymin;(y<ymax)&&(!found);y++){

				if(!Maps->isValidBlock(x,y,z))
					continue;
				Maps->ReadBlock40d(x,y,z, &Block);

				for(uint32_t xx=0;xx<SQUARESPERBLOCK&&!found;xx++){
					for(uint32_t yy=0;yy<SQUARESPERBLOCK&&!found;yy++){

						int type = tileTypeTable[Block.tiletypes[xx][yy]].c;
						int val = (1<<type)&typeToFind;
						if(val){
							found=true;
						}
					}
				}
			}
		}

		if(found){
			limitz[z]=255;
			count++;
		}
	}

	return count;
}


void convertDFBlock(DFHack::API *Maps, /*DFHack::API * Mats,*/ vector< vector <uint16_t> > layerassign, 
		//vector<DFHack::t_feature> global_features, std::map <DFHack::planecoord, std::vector<DFHack::t_feature *> > local_features,
		std::vector<t_matgloss> &stonematgloss, std::vector<t_matgloss> &metalmatgloss, std::vector<t_matgloss> &woodmatgloss,
		DFHack::API *Cons, uint32_t numCons, map<uint32_t,myBuilding> Buildings, map<uint32_t,char*> vegs,
		TiXmlElement *uio, uint8_t* mclayers, uint8_t* mcdata,
		uint32_t dfblockx, uint32_t dfblocky, uint32_t zzz, uint32_t zcount,
		uint32_t xoffset, uint32_t yoffset, int mcxsquares, int mcysquares){

	DFHack::mapblock40d Block;

	// read data
	Maps->ReadBlock40d(dfblockx,dfblocky,zzz, &Block);
	//Maps->ReadTileTypes(x,y,z, &tiletypes);
	//Maps->ReadDesignations(x,y,z, &designations);

	vector<DFHack::t_vein> veins;
	//vector<DFHack::t_spattervein> splatter;
	vector<DFHack::t_frozenliquidvein> ices;
	Maps->ReadVeins(dfblockx,dfblocky,zzz,veins,ices);//,&splatter);

			
	for(uint32_t dfoffsetx=0;dfoffsetx<SQUARESPERBLOCK;dfoffsetx++){
		for(uint32_t dfoffsety=0;dfoffsety<SQUARESPERBLOCK;dfoffsety++){

			uint32_t dfx = dfblockx*SQUARESPERBLOCK + dfoffsetx;
			uint32_t dfy = dfblocky*SQUARESPERBLOCK + dfoffsety;
			int16_t tiletype = Block.tiletypes[dfoffsetx][dfoffsety]; //this is the type of terrian at the location (or at least the tiletype.c(lass) is)

			naked_designation &des = Block.designation[dfoffsetx][dfoffsety].bits; //designations at this location

			const char* mat = NULL; //item material
			const char *consmat=NULL;//material of construction
							
			if(tileTypeTable[tiletype].m != CONSTRUCTED){
				int16_t tempvein = -1;

				//try to find material in base layer type
							
				uint8_t test = Block.designation[dfoffsetx][dfoffsety].bits.biome;
				if( test < sizeof(Block.biome_indices)){
					//otherwise memory error - not sure how to handle, but shouldn't happen
					tempvein = layerassign[Block.biome_indices[test]][Block.designation[dfoffsetx][dfoffsety].bits.geolayer_index];
				}

				//find material in veins
				//iterate through the bits
				for(int v = 0; v < (int)veins.size();v++){
					// and the bit array with a one-bit mask, check if the bit is set
					bool set = !!(((1 << dfoffsetx) & veins[v].assignment[dfoffsety]) >> dfoffsetx);
					if(set){
						// store matgloss
						tempvein = veins[v].type;
					}
				}
				// global feature overrides
				/*
				int16_t idx = Block.global_feature;
				if( idx != -1 && uint16_t(idx) < global_features.size() && global_features[idx].type == DFHack::feature_Underworld){
					if(Block.designation[dfoffsetx][dfoffsety].bits.feature_global){
						if(global_features[idx].main_material == 0){ // stone
							tempvein = global_features[idx].sub_material;
						}else{
							tempvein = -1;
						}
					}
				}
				//check local features
				idx = Block.local_feature;
				if( idx != -1 ){
					DFHack::planecoord pc;
					pc.dim.x = dfblockx;
					pc.dim.y = dfblocky;
					std::map <DFHack::planecoord, std::vector<DFHack::t_feature *> >::iterator it;
					it = local_features.find(pc);
					if(it != local_features.end()){
						std::vector<DFHack::t_feature *>& vectr = (*it).second;
						if(uint16_t(idx) < vectr.size() && vectr[idx]->type == DFHack::feature_Adamantine_Tube)
						if(Block.designation[dfoffsetx][dfoffsety].bits.feature_local && DFHack::isWallTerrain(Block.tiletypes[dfoffsetx][dfoffsety])){
							if(vectr[idx]->main_material == 0){ // stone
								tempvein = vectr[idx]->sub_material;
							}else{
								tempvein = -1;
							}
						}
					}
				}
				*/

				if(tempvein!=-1){
					mat = stonematgloss[tempvein].id;
				}else{
					mat=NULL;
				}

			}else{
				//find construction and locate it's material
				mat = NULL;
				consmat=NULL;
				t_construction con;
				for(uint32_t i = 0; i < numCons; i++){
					Cons->ReadConstruction(i,con);
					if(dfx == con.x && dfy == con.y && zzz == con.z){
						consmat="unknown";
						//TODO
						switch (con.material.type){
							case 0:
								if(con.material.index>=0 && con.material.index<woodmatgloss.size()){
									consmat = woodmatgloss[con.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "logs";
								break;
							case 1:
								if(con.material.index>=0 && con.material.index<woodmatgloss.size()){
									consmat = stonematgloss[con.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "stone";
								break;
							case 2:
								if(con.material.index>=0 && con.material.index<woodmatgloss.size()){
									consmat = metalmatgloss[con.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "bars";
								break;
							//case 12: // don't ask me why this has such a large jump, maybe this is not actually the matType for plants, but they all have this set to 12
							//	consmat = mat.plantMat[con.material.index].id;
							//	break;
							//case 32:
							//	consmat = mat.plantMat[con.material.index].id;
							//	break;
							//case 121:
							//	consmat = mat.creatureMat[con.material.index].id;
							//	break;
							
						}


						/*
						if(con.material.type == 0){
							if(con.material.index != 0xffffffff)
								consmat = Mats->inorganic[con.material.index].id;
							else consmat = "inorganic";
						}
						if(con.material.type == 420){
							if(con.material.index != 0xffffffff)
								consmat = Mats->organic[con.material.index].id;
							else consmat = "organic";
							mat="logs";
						}
						
						switch(con.form){
							case constr_bar:
								mat="bars";
								break;
							case constr_block:
								mat="blocks";
								break;
							case constr_boulder:
								mat="stones";
								break;
							case constr_logs:
								mat="logs";
								break;
							default:
								mat="unknown";
						}
						break;
						*/
					}
				}
			}

						

			char classname[128];
			int variant = tileTypeTable[tiletype].v;
			switch(tileTypeTable[tiletype].c){
				case RAMP:{
					char dir[16];
					getRampDir(Maps,&Block,uio,dir,dfx,dfy,zzz,dfblockx,dfblocky,TileMaterialNames[tileTypeTable[tiletype].m], variant, tileTypeTable[tiletype].name, mat,consmat);
					_snprintf(classname,127,"%s%s",TileClassNames[tileTypeTable[tiletype].c],dir);
				}break;
				case STAIR_UP:
				case STAIR_DOWN:
				case STAIR_UPDOWN:{
					_snprintf(classname,127,"%s%d",TileClassNames[tileTypeTable[tiletype].c],zcount%4);//z$4 is the level height mod 4 so you can do spiral stairs or alterante sides if object defination of 0 = 2 and 1 = 3
				}break;
				case TREE_DEAD:
				case TREE_OK:
				case SAPLING_DEAD:
				case SAPLING_OK:
				case SHRUB_DEAD:
				case SHRUB_OK:{
					consmat = mat;
					mat = "unknown";
					map<uint32_t,char*>::iterator vit;
					vit = vegs.find(getMapIndex(dfx,dfy,zzz));
					if(vit!=vegs.end()){
						mat = vit->second;
					}else{
						printf("Cant find plant that should already be defined!\n");
					}
				}//no break, we want to fall through
				default:
					strncpy(classname,TileClassNames[tileTypeTable[tiletype].c],127);
					if(strnicmp(tileTypeTable[tiletype].name,"smooth",5)==0){//doing this as des.smooth never seems to be set.
						variant+=10;//and if I could figure out engraved it would be 20 over base variant
					}
					break;
			}

						
			uint8_t* object = getObject(uio,dfx, dfy, zzz,classname, TileMaterialNames[tileTypeTable[tiletype].m], variant, tileTypeTable[tiletype].name, mat,consmat);


			//now copy object in to mclayer array
			addObject(mclayers, mcdata, object,  dfx,  dfy, zzz, xoffset, yoffset, zcount, mcxsquares, mcysquares);

						
			//add tree top if tree
			if((tileTypeTable[tiletype].c == TREE_OK) && ((zcount+1) < limitlevels) ){
				object = getObject(uio,dfx, dfy, zzz,"treetop", "air", variant, tileTypeTable[tiletype].name, mat);
				if(object!=NULL)
					addObject(mclayers, mcdata, object,  dfx,  dfy, zzz+1, xoffset, yoffset, zcount+1, mcxsquares, mcysquares);
			}


			//Add building if any (furnaces/forge to furnace, others to workbench, make 'tables'/chests to block impassible squares
			uint32_t index = getMapIndex(dfx,dfy,zzz);
			map<uint32_t,myBuilding>::iterator it;
			it = Buildings.find(index);
			if(it!=Buildings.end()){
				myBuilding mb = it->second;
				mat="unknown";
				//TODO
				switch (mb.material.type){
							case 0:
								if(mb.material.index>=0 && mb.material.index<woodmatgloss.size()){
									consmat = woodmatgloss[mb.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "logs";
								break;
							case 1:
								if(mb.material.index>=0 && mb.material.index<woodmatgloss.size()){
									consmat = stonematgloss[mb.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "stone";
								break;
							case 2:
								if(mb.material.index>=0 && mb.material.index<woodmatgloss.size()){
									consmat = metalmatgloss[mb.material.index].id;
								}else{
									consmat = "unknown";
								}
								mat = "bars";
								break;
				}
				/*if(mb.material.type == 0){
					if(mb.material.index!= 0xffffffff)
						mat = Mats->inorganic[mb.material.index].id;
					else mat = "inorganic";
				}
				if(mb.material.type == 420){
					if(mb.material.index != 0xffffffff)
						mat = Mats->organic[mb.material.index].id;
					else mat = "organic";
				}
				*/
				object = getObject(uio,dfx, dfy, zzz,"building", mb.type, 0, mb.desc, mat);
				if(object!=NULL){
					addObject(mclayers, mcdata, object,  dfx,  dfy, zzz, xoffset, yoffset, zcount, mcxsquares, mcysquares);
				}else{
					printf("Cant find building that should already be defined!\n");
				}
			}

			//add object if any (mostly convert bins and barrels to chests)
			//TODO


			//add water/magma (liquid) if any
			if(des.flow_size>0){

				char *type = "magma";
				if(des.liquid_type == DFHack::liquid_water){
					type = "water";
				}
							
				object = getObject(uio,dfx, dfy, zzz,"flow", type ,des.flow_size); //more standardized to other objects
				if(object==NULL){
					char amount[16];
					itoa(des.flow_size,amount,10);
					object = getObject(uio,dfx, dfy, zzz, type, amount); //easier to remember
				}
				if(object!=NULL){
					addObject(mclayers, mcdata, object,  dfx,  dfy, zzz, xoffset, yoffset, zcount, mcxsquares, mcysquares);
				}
			}


			//add torch if any 'dark' and is floor, and no mud (cave)
			if(tileTypeTable[tiletype].c == FLOOR && (des.light==0 || des.skyview==0 || des.subterranean>0)){
				//we will add torches to floors that aren't muddy as muddy floors are caves (or farms) usually, which we don't want lit,- we will claim the mud puts out the torch
							
							
				//check for mud
				bool mud = false;
				/*TODO
				for(int v = 0; v < (int)splatter.size()&&!mud;v++){
					if(splatter[v].mat1=0xC){//magic number for mud - see cleanmap.cpp
						if(splatter[v].intensity[dfoffsetx][dfoffsety]>0){
							mud=true; //yes, this tile is muddy
						}
					}
				}
				*/
				if(!mud){
					//ok, good spot for torch - add if the random number is less than the chance depending on the 'dark' type, we don't want every floor to have a torch, just randomly placed
					int percent = 0;
					if(des.skyview==0)
						percent = max(percent,torchPerInside);
					if(des.light==0)
						percent = max(percent,torchPerDark);
					if(des.subterranean>0)//what are possible values
						percent = max(percent,torchPerSubter);

					if( (rand()%100)<percent ){
						//ok, try to add a torch here
						object = getObject(uio,dfx, dfy, zzz,"building", "torch");
						if(object!=NULL)
							addObject(mclayers, mcdata, object,  dfx,  dfy, zzz, xoffset, yoffset, zcount, mcxsquares, mcysquares);

					}
				}
			}
		}
	}
}

int convertMaps(DFHack::API *DF/*,DFHack::Materials * Mats*/){

	printf("\nCalculating size limit...\n");

	//setup

	uint32_t x_max,y_max,z_max;
	//vector<DFHack::t_feature> global_features;
    //std::map <DFHack::planecoord, std::vector<DFHack::t_feature *> > local_features;
	vector< vector <uint16_t> > layerassign;
	
	uint16_t cloudheight=0;

	//create xml file for a list of full unimplemented objects
	TiXmlElement *uio = NULL;
	if(createUnknown){
		TiXmlDocument *doc = new TiXmlDocument("unimplementedobjects.xml");
		bool loadOkay = doc->LoadFile();
		uio=doc->FirstChildElement("unimplemented_objects");
		if(uio==NULL){
			uio = new TiXmlElement("unimplemented_objects");
			doc->LinkEndChild(uio);
			TiXmlComment *comment = new TiXmlComment("These elements are the most specific elements that make up the maps that you have converted\nIf something doesn't look right in the converted map, try to fix the object here,\nspecify how it should look in Minecraft, and copy that object into the settings.xml file");
			uio->LinkEndChild(comment);
		}
	}

	//DFHack::Maps * Maps = DF->getMaps();

	// init the map
    if(!DF->InitMap())
    {
        cerr << "Can't init map." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 103;
    }
    DF->getSize(x_max,y_max,z_max);

	printf("DF Map size in \'blocks\' %dx%d with %d levels (a 3x3 block is one embark space)\n",x_max,y_max,z_max);
	printf("DF Map size in squares %d, %d, %d\n",x_max*SQUARESPERBLOCK,y_max*SQUARESPERBLOCK,z_max);


	//level area limit
	x_max=min(x_max,limitxmax);
	y_max=min(y_max,limitymax);
	uint32_t xoffset = limitxmin;
	uint32_t yoffset = limitymin;
	//uint32_t zlevelcount = z_max;

	if( xoffset>x_max || yoffset>y_max ){
		printf("Invalid level size.\n");
		return 21;
	}
	
	memset(&limitz,0,1000);


	//setup level area limit
	if(limittype == LIMITTOP){
		if(limitairtokeep < 1 || limitairtokeep > (z_max/2) ){
			//keep top limitlevels, disreguard air only levels
			for(uint32_t zz=z_max-limitlevels;zz<z_max;zz++){
				limitz[zz]=255;
			}
			cloudheight=5;
		}else{
			//keep top limitlevels, but only airtokeep air levels
			findLevels(DF,xoffset,x_max,yoffset,y_max,z_max,(1<<EMPTY)^0xffff);

			//ok levels that are not all air are now marked
			//we need to add 'airtokeep' levels to the top and then limit it to limitlevels
			uint32_t top = z_max;
			for(uint32_t i=z_max;i>0;i--){
				if(limitz[i]>0){
					top=i;
					break;
				}
			}
			top+=limitairtokeep;
			if(top>z_max) top = z_max;
			uint32_t bottom = top - limitlevels;
			//reset level marks
			for(uint32_t i=0;i<z_max;i++){
				if(i>top || i<=bottom){
					limitz[i]=0;
				}else{
					limitz[i]=255;
				}
			}
			cloudheight=limitairtokeep/2;
		}
	}else if(limittype == LIMITRANGE){
		if(limittoplevel<=0){
			limittoplevel = z_max;
		}
		for(uint32_t i=limittoplevel;i>(limittoplevel-limitlevels);i--){
			limitz[i]=255;
		}
		cloudheight=limitlevels;
	}else if(limittype == LIMITSMART){

		//keep 'interesting' levels and airtokeep air levels
		findLevels(DF,xoffset,x_max,yoffset,y_max,z_max, (1<<FLOOR) + (1<<PILLAR) + 
			(1<<FORTIFICATION) + (1<<RAMP) + (1<<RAMP_TOP) );

		//ok, interesting levels are marked, mark the airtokeep levels above the top most interesting level
		uint32_t top = z_max;
		for(uint32_t i=z_max;i>0;i--){
			if(limitz[i]>0){
				top=i;
				break;
			}
		}
		top+=limitairtokeep;
		if(top>z_max) top = z_max;
		for(uint32_t i=top;i>0;i--){
			if(limitz[i]==0){
				limitz[i]=255;
			}else{
				break;
			}
		}
		//now count marked levels
		uint32_t bottom = 0;
		uint32_t count = 0;
		for(uint32_t i=z_max;i>0;i--){
			if(limitz[i]>0){
				bottom=i;
				count++;
			}
		}
		if(count>limitlevels){
			//too many levels, remove from bottom
			for(uint32_t i=0;i<z_max&&count>=limitlevels;i++){
				if(limitz[i]>0){
					limitz[i]=0;
					count--;
				}
			}
		}else if(count<limitlevels){
			//too few levels, add one uninteresting level after each uninteresting
			int last=0;
			int oldcount = count-1;
			while(count<limitlevels&&oldcount<count){
				oldcount = count;
				for(int i=z_max;i>=0&&count<limitlevels;i--){
					if(limitz[i]==0 && last==255){
						limitz[i]=255;
						count++;
						last=0;
					}else{
						last=limitz[i];
					}
				}
			}
			//if we are still low, you get extra air
		}
		cloudheight=limitairtokeep/2;
	}else{

		memset(&limitz,255,1000);
		limitlevels = z_max;
		cloudheight=5;
	}


	//setup variables and get data from Dwarf Fortress
	int dfxsquares = (x_max-xoffset)*SQUARESPERBLOCK ;
	int dfysquares = (y_max-yoffset)*SQUARESPERBLOCK;
	int dfzsquares = (limitlevels);
	int mcxsquares = dfxsquares * squaresize;
	int mcysquares = dfysquares * squaresize;
	int mczsquares = dfzsquares * squaresize+1;

	printf("DF Map size cut down to %dx%d \'blocks\'\n",(x_max-xoffset),(y_max-yoffset));
	printf("DF Map size cut down to %d, %d, %d squares\n",dfxsquares,dfysquares, dfzsquares);
	printf("MC Map size cut down to %d, %d, %d squares\n",mcxsquares,mcysquares, mczsquares);
	if(mcxsquares > 512 || mcysquares > 512 || mczsquares > 128){
		printf("WARNING: maps larger than 512x512x128 can cause issues with Minecraft indev\n");
	}
	if(mcxsquares<0 || mcysquares<0 || mczsquares<0){
		printf("Area too small\n");
		return 20;
	}

    /*
    if(!Maps->ReadGlobalFeatures(global_features))
    {
        cerr << "Can't get global features." << endl;
        return 104; 
    }

    if(!Maps->ReadLocalFeatures(local_features))
    {
        cerr << "Can't get local features." << endl;
        return 105; 
    }
	*/

	// get region geology
    if(!DF->ReadGeology( layerassign ))
    {
        cerr << "Can't get region geology." << endl;
        return 106; 
    }


	std::vector<t_matgloss> stonematgloss;
	std::vector<t_matgloss> woodmatgloss;
	std::vector<t_matgloss> metalmatgloss;
	std::vector<t_matgloss> plantmatgloss;
	DF->ReadStoneMatgloss(stonematgloss);
    DF->ReadWoodMatgloss (woodmatgloss);
    DF->ReadMetalMatgloss(metalmatgloss);
    DF->ReadPlantMatgloss(plantmatgloss);


	//DFHack::Constructions *Cons = DF->getConstructions();
	uint32_t numConstr;
    //Cons->Start(numConstr);
	DF->InitReadConstructions(numConstr);

	printf("\nReading Plants... ");
	//DFHack::Vegetation * v = DF->getVegetation();
	uint32_t numVegs = 0;
    //v->Start(numVegs);
	DF->InitReadVegetation(numVegs);
	//read vegetation into a map for faster access later
	map<uint32_t,char*> vegs;
	for(uint32_t i =0; i < numVegs; i++){
		DFHack::t_tree_desc tree;
		DF->ReadVegetation(i,tree);
	
		if(tree.material.type!=2){
			if(tree.material.index>0 && tree.material.index<woodmatgloss.size()){
				vegs[getMapIndex(tree.x,tree.y,tree.z)] = woodmatgloss[tree.material.index].id;
			}else{
				vegs[getMapIndex(tree.x,tree.y,tree.z)] = "unknowntree";
			}
		}else{
			if(tree.material.index>0 && tree.material.index<plantmatgloss.size()){
				vegs[getMapIndex(tree.x,tree.y,tree.z)] = plantmatgloss[tree.material.index].id;
			}else{
				vegs[getMapIndex(tree.x,tree.y,tree.z)] = "unknownplant";
			}
			
		}
	}
	printf("%d\n",vegs.size());


	//read buildings into a map organized by location (10 bit each for x,y,z packed into an int)
	printf("Reading Buildings... ");
	//DFHack::Buildings * Bld = DF->getBuildings();
	//map <uint32_t, string> custom_workshop_types;
	uint32_t numBuildings;
	memory_info * mem = DF->getMemoryInfo();
    //DFHack::Position * Pos = DF->getPosition();

	map<uint32_t,myBuilding> Buildings;
    
	if(DF->InitReadBuildings(numBuildings)){
        //Bld->ReadCustomWorkshopTypes(custom_workshop_types);

		for(uint32_t i = 0; i < numBuildings; i++){
            DFHack::t_building temp;
			DF->ReadBuilding(i, temp);
            string typestr;
            mem->resolveClassIDToClassname(temp.type, typestr);
            //printf("Address 0x%x, type %d (%s), %d/%d to %d/%d on level %d\n",temp.origin, temp.type, typestr.c_str(), temp.x1,temp.y1,temp.x2,temp.y2,temp.z);
            //printf("Material %d %d\n", temp.material.type, temp.material.index);
            //int32_t custom;
            //if((custom = Bld->GetCustomWorkshopType(temp)) != -1){
				//printf("Custom workshop type %s (%d)\n",custom_workshop_types[custom].c_str(),custom);
				//typestr = custom_workshop_types[custom];
            //}

			char tempstr[256];
			if(typestr.find("building_")==0){
				strncpy(tempstr,typestr.c_str()+9,255);
				tempstr[255]='\0';
			}else{
				strncpy(tempstr,typestr.c_str(),255);
				tempstr[255]='\0';
			}

			for(uint32_t x=temp.x1;x<=temp.x2;x++){
				for(uint32_t y=temp.y1;y<=temp.y2;y++){
					myBuilding *mb = new myBuilding;

					strncpy(mb->type,tempstr,255);
					mb->type[255]='\0';
					mb->material.type = temp.material.type;
					mb->material.index = temp.material.index;

					if((temp.x1==temp.x2) && (temp.y1==temp.y2)){
						_snprintf(mb->desc,15,"only");
					}else if((temp.x1==temp.x2) && (y==temp.y2)){
						_snprintf(mb->desc,15,"xonlyymax");
					}else if((temp.x1==temp.x2) && (y<temp.y2)){
						_snprintf(mb->desc,15,"xonlyy%d",y-temp.y1);
					}else if((x==temp.x2) && (temp.y1==temp.y2)){
						_snprintf(mb->desc,15,"xmaxyonly");
					}else if((x<=temp.x2) && (temp.y1==temp.y2)){
						_snprintf(mb->desc,15,"x%dyonly",x-temp.x1);
					}else if((x==temp.x2) && (y==temp.y2)){
						_snprintf(mb->desc,15,"xmaxymax");
					}else if((x==temp.x2) && (y<temp.y2)){
						_snprintf(mb->desc,15,"xmaxy%d",y-temp.y1);
					}else if((x<temp.x2) && (y==temp.y2)){
						_snprintf(mb->desc,15,"x%dymax",x-temp.x1);
					}else{
						_snprintf(mb->desc,15,"x%dy%d",x-temp.x1,y-temp.y1);
					}
					mb->desc[15]='\0';

					uint32_t index = getMapIndex(x,y,temp.z);

					if(strcmp(mb->type,"stockpilest")==0){
						if(Buildings.find(index)==Buildings.end()){
							Buildings[index] = *mb;
//						}else{
//							printf("Not replacing a building with a stockpile\n");
						}
					}else{
						Buildings.erase(index);
						Buildings[index] = *mb;
					}
				}
			}
			
        }
	}
	printf("%d\n",Buildings.size());


	//allocate memory for blocks and data
	uint8_t* mclayers = new uint8_t[mcxsquares*mcysquares*mczsquares];
	uint8_t* mcdata = new uint8_t[mcxsquares*mcysquares*mczsquares];
	if( mclayers==NULL || mcdata == NULL){
		printf("Unable to allocate memory to store level data, exiting.");
		return 10;
	}
	memset(mclayers,0,sizeof(uint8_t)*(mcxsquares*mcysquares*mczsquares));
	memset(mcdata,0,sizeof(uint8_t)*(mcxsquares*mcysquares*mczsquares));

	

	//read DF map data and create MC map blocks and data arrays;
	printf("\nConverting Map...\n");
	DF->InitMap();

	// walk the DF map!
	uint32_t zcount = 0;
	for(uint32_t zzz = 0; zzz< z_max;zzz++){
		if(limitz[zzz]==0)
			continue;
		printf("Layer %d/%d\n",zzz,z_max);
		for(uint32_t dfblockx = xoffset; dfblockx< x_max;dfblockx++){
			for(uint32_t dfblocky = yoffset; dfblocky< y_max;dfblocky++){

				if(DF->isValidBlock(dfblockx,dfblocky,zzz)){
					
					convertDFBlock(DF, layerassign, 
						//global_features, local_features,
						stonematgloss, metalmatgloss, woodmatgloss,
						DF, numConstr, Buildings, vegs,
						uio, mclayers,mcdata,
						dfblockx, dfblocky, zzz, zcount, xoffset, yoffset, mcxsquares, mcysquares);
				
				}		
				
			}
		}
		printf(" unknown obj: %d  imperfect obj: %d  perfect obj: %d  new obj: %d\n",unknowncount,imperfectknown,perfectknown,prevunseen);
		zcount++;
	}

	if(uio!=NULL){
		if(uio->GetDocument()!=NULL){
			uio->GetDocument()->SaveFile();
			delete uio->GetDocument();
		}
	}


	//add bottom layer of adminium
	for(int oy=0;oy<mcxsquares;oy++){
		for(int ox=0;ox<mcysquares;ox++){

			//Index = x + (y * Depth + z) * Width //where y is up down
			int idx = ox + (oy) * mcxsquares;
			mclayers[idx]=7; // bedrock / adminium
		}
	}


	//place the spawn at DF cursor location, if within output area and not a wall
	printf("\nPlancing spawn location\n");
	//DFHack::Position *Pos = DF->getPosition();
    int32_t cx, cy, cz,ocx,ocy,ocz; 
	DF->InitViewAndCursor();
    DF->getCursorCoords(ocx,ocy,ocz);
	cx=ocx;cy=ocy;cz=ocz;
    if(cx != -30000 && cx>=0 && cy>=0 && cz>=0){
		//see if it is in an output layer (should be if a open space was chosen)
		if(limitz[cz]=255){
			if( ((limitxmin*SQUARESPERBLOCK)<=(uint32_t)cx) && ((limitxmax*SQUARESPERBLOCK)>=(uint32_t)cx) && 
				((limitymin*SQUARESPERBLOCK)<=(uint32_t)cy) && ((limitymax*SQUARESPERBLOCK)>=(uint32_t)cy) ){

				//convert to MC index - need to rotate - need to position from NW with +x going south and +y going west (I think)
				bool ok = false;
				int32_t temp = cx;
				cx = cy * squaresize - (yoffset*SQUARESPERBLOCK*squaresize) + (squaresize/2);
				cy = mcxsquares - ((temp * squaresize - (xoffset*SQUARESPERBLOCK*squaresize)) + (squaresize/2));
				int zcnt = 0; //we need to count the number of included levels from here to the bottom as that is the actual layer height in minecraft
				for(int k=cz-1;k>=0;k--){
					if(limitz[k]>0){
						zcnt++;
					}
				}
				//now chek if that location is or, or do we have to move up because it is a floor cube.
				for(int i=1;i<(squaresize)&&!ok;i++){//starting at 1 because of the one layer of adminium at the bottom of the map
					cz = zcnt * squaresize + i;
					int idx = cx + (cz * mcysquares +cy) * mcxsquares;
					int idx1 = cx + ((cz+1) * mcysquares +cy) * mcxsquares;

					if(mclayers[idx]==0 && mclayers[idx1]==0){
						//if this location and one above it are air, spawn location is ok
						ok = true;
					}
				}
				if(!ok){
					printf("DF Cursor not at location with enough space for spawn\nSetting spawn to top center of area\n");
					cx = -1;//never found a valid location - place in center
				}
			}else{
				printf("DF Cursor outside of area output on this level\nSetting spawn to top center of area\n");
				cx=-1;
			}
		}else{
			printf("DF Cursor on level not being output\nSetting spawn to top center of area\n");
			cx=-1;
		}
	}
	if(cx==30000 || cx<0){
		//need to set spawn to center of map;
		printf("Putting spawn at center of map...\n");
		cx = mcxsquares/2;
		cy = mcysquares/2;
		cz = mczsquares;
		uint8_t val = 0;
		int pos;
		while(val == 0 && cz>0){
			cz--;
			pos = (cx) + ((cz) * mcysquares + cy) * mcxsquares;
			val = mclayers[pos];
		};
		cz++;
		ocx =  ( (mcysquares - cy) / squaresize + (xoffset*SQUARESPERBLOCK)); 
		ocy = cx / squaresize + (yoffset*SQUARESPERBLOCK); 
		ocz = cz / squaresize;
		int cnt=0;//find the (cz / squaresize)th level set for output - that level is the level in DF
		for(int i=0;i<999;i++){
			if(limitz[i]>0){
				if(cnt==(cz / squaresize)){
					ocz=i;
				}
				cnt++;
			}
		}

	}
	printf("Putting spawn at %d,%d,%d in Minecraft\nwhich is at %d,%d,%d in Dwarf Fortress\n",cx,cy,cz,ocx,ocy,ocz);
	

	//save the level!
	char name[256];
	//strcpy(name,"out.mclevel");
	//DFHack::World *world;
	//world = DF->getWorld();
	//int year = world->ReadCurrentYear();
	//int month = (world->ReadCurrentMonth())%12;
	//int day = world->ReadCurrentDay();
	DFHack::t_settlement here;
	uint32_t numSettle;
	DF->InitReadSettlements(numSettle);
	DF->ReadCurrentSettlement(here);

	
	vector< vector<string> > englishWords;
    vector< vector<string> > foreignWords;
    if(DF->InitReadNameTables(englishWords,foreignWords)){
        string genericName = DF->TranslateName(here.name,englishWords,foreignWords,true);
		_snprintf(name,255,"%s.mclevel",genericName.c_str());
    }else{
		_snprintf(name,255,"out.mclevel");
	}
	name[255]='\0';
	printf("\n%s\n",name);

	return saveMCLevel(mclayers, mcdata, mcxsquares, mcysquares, mczsquares, cloudheight,cx,cy,cz,name);

}


int testLevel(){

	char *testdata[75];
	int i=0;
	testdata[i++]="wall.stone"; testdata[i++]="wall.stone"; testdata[i++]="wall.stone"; testdata[i++]="wall.stone"; testdata[i++]="wall.stone";
	testdata[i++]="wall.stone"; testdata[i++]="wall.vein"; testdata[i++]="wall.vein"; testdata[i++]="wall.vein"; testdata[i++]="wall.stone";
	testdata[i++]="wall.stone"; testdata[i++]="wall.vein"; testdata[i++]= "wall.soil"; testdata[i++]= "wall.vein"; testdata[i++]= "wall.stone"; 
	testdata[i++]="wall.stone"; testdata[i++]= "wall.vein"; testdata[i++]= "wall.vein"; testdata[i++]= "wall.vein"; testdata[i++]= "wall.stone"; 
	testdata[i++]="wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; 

	testdata[i++]="wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; 
	testdata[i++]="wall.stone"; testdata[i++]= "rampnw.featstone"; testdata[i++]= "rampn.featstone"; testdata[i++]= "rampne.featstone"; testdata[i++]= "wall.stone"; 
	testdata[i++]="wall.stone"; testdata[i++]= "rampw.soil"; testdata[i++]= "floor.soil"; testdata[i++]= "rampe.stone"; testdata[i++]= "wall.stone"; 
	testdata[i++]="wall.stone"; testdata[i++]= "rampsw.grass"; testdata[i++]= "ramps.constructed"; testdata[i++]= "rampse.vein"; testdata[i++]= "wall.stone";
	testdata[i++]="wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; testdata[i++]= "wall.stone"; 

	testdata[i++]="wall.stone"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "wall.featstone"; 
	testdata[i++]="empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; 
	testdata[i++]="empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; 
	testdata[i++]="empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; 
	testdata[i++]="wall.constructed"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "empty.air"; testdata[i++]= "wall.stone";

	uint32_t x_max = 5;
	uint32_t y_max=5;
	uint32_t z_max=3;
	int mcxsquares = x_max * squaresize;
	int mcysquares = y_max * squaresize;
	int mczsquares = z_max * squaresize;

	uint8_t* mclayers = new uint8_t[mcxsquares*mcysquares*mczsquares];
	uint8_t* mcdata = new uint8_t[mcxsquares*mcysquares*mczsquares];
	if( mclayers==NULL || mcdata == NULL){
		printf("Unable to allocate memory to store level data, exiting.");
		return 10;
	}
	memset(mclayers,0,sizeof(uint8_t)*(mcxsquares*mcysquares*mczsquares));
	memset(mcdata,0,sizeof(uint8_t)*(mcxsquares*mcysquares*mczsquares));

	for(uint32_t z = 0; z< z_max;z++){
		printf("Layer %d/%d ",z,z_max);
		for(uint32_t dfx = 0; dfx< x_max;dfx++){
			for(uint32_t dfy = 0; dfy< y_max;dfy++){

				uint8_t *object = NULL;

				char* location = testdata[dfx + (((z*y_max) + dfy)* x_max)];
				object=objects[location];


				//TODO, now copy object in to mclayer array
				int mcx = dfx * squaresize;
				int mcy = dfy * squaresize;
				int mcz = z * squaresize;
				int pos = 0;
				int pos2 = squaresize*squaresize*squaresize;

				for(int oz=0;oz<squaresize;oz++){
					for(int oy=0;oy<squaresize;oy++){
						for(int ox=0;ox<squaresize;ox++){

							//Index = x + (y * Depth + z) * Width //where y is up down
							int idx = (mcx+ox) + ((mcz+2-oz) * mcysquares + mcy + oy) * mcxsquares;
							mclayers[idx]=object[pos];
							mcdata[idx]=object[pos2];
							pos++;pos2++;

						}
					}
				}

			}
		}
	}

	int cx = mcxsquares/2;
	int cy = mcysquares/2;
	int cz = mczsquares;
	uint8_t val = 0;
	int pos;
	while(val == 0 && cz>0){
		cz--;
		pos = (cx) + ((cz) * mcysquares + cy) * mcxsquares;
		val = mclayers[pos];
	};
	cz+=2;//move up two so spawn isn't stuck in the ground

	return saveMCLevel(mclayers, mcdata, mcxsquares, mcysquares, mczsquares,0, cx,cy,cz,"out.mclevel");

}



int main (int argc, const char* argv[]){

	//load settings xml
	TiXmlDocument doc("settings.xml");
	bool loadOkay = doc.LoadFile();
	if (!loadOkay){
		printf("Could not load SETTINGS.XML\n");
		return 1;
	}

	//load basic settings
	settings = doc.FirstChildElement("settings");
	if(settings==NULL){
		printf("Could not load settings from SETTINGS.XML (corrupt xml file?)\n");
		return 2;
	}
	squaresize=3;
	if(settings->FirstChildElement("squaresize")==NULL){
		TiXmlElement * ss = new TiXmlElement( "squaresize" );  
		ss->SetAttribute("val",3);
		settings->LinkEndChild( ss );  
	}
	const char* res = settings->FirstChildElement("squaresize")->Attribute("val",&squaresize);
	if(res==NULL || squaresize<1 || squaresize>10){
		printf("Invalid square size setting\n");
		return 3;
	}

	if(settings->FirstChildElement("torchinsidepercent")==NULL){
		TiXmlElement * ss = new TiXmlElement( "torchinsidepercent" );
		settings->LinkEndChild( ss );  
	}
	if(settings->FirstChildElement("torchinsidepercent")->Attribute("val",&torchPerInside)==NULL){
		torchPerInside = 10;
		settings->FirstChildElement("torchinsidepercent")->SetAttribute("val",torchPerInside);
	}
	if(settings->FirstChildElement("torchdarkpercent")==NULL){
		TiXmlElement * ss = new TiXmlElement( "torchdarkpercent" );
		settings->LinkEndChild( ss );  
	}
	if(settings->FirstChildElement("torchdarkpercent")->Attribute("val",&torchPerDark)==NULL){
		torchPerDark=20;
		settings->FirstChildElement("torchdarkpercent")->SetAttribute("val",torchPerDark);
	}
	if(settings->FirstChildElement("torchsubterraneanpercent")==NULL){
		TiXmlElement * ss = new TiXmlElement( "torchsubterraneanpercent" );
		settings->LinkEndChild( ss );  
	}
	if(settings->FirstChildElement("torchsubterraneanpercent")->Attribute("val",&torchPerSubter)==NULL){
		torchPerSubter=30;
		settings->FirstChildElement("torchsubterraneanpercent")->SetAttribute("val",torchPerSubter);
	}
	
	int temp;
	if(settings->FirstChildElement("horizontalarea")!=NULL){
		if(settings->FirstChildElement("horizontalarea")->Attribute("xmin",&temp)!=NULL){
			limitxmin = temp;
		}
		if(settings->FirstChildElement("horizontalarea")->Attribute("xmax",&temp)!=NULL){
			limitxmax = temp;
		}
		if(settings->FirstChildElement("horizontalarea")->Attribute("ymin",&temp)!=NULL){
			limitymin = temp;
		}
		if(settings->FirstChildElement("horizontalarea")->Attribute("ymax",&temp)!=NULL){
			limitymax = temp;
		}
	}

	if(settings->FirstChildElement("verticalarea")!=NULL){
		const char *str;
		if((str=settings->FirstChildElement("verticalarea")->Attribute("type"))!=NULL){
			if( stricmp(str,"top")==0){
				limittype = LIMITTOP;
			}else if( stricmp(str,"range")==0){
				limittype = LIMITRANGE;
			}else if( stricmp(str,"smart")==0){
				limittype = LIMITSMART;
			}else{
				limittype = LIMITNONE;
			}
		}
		if(settings->FirstChildElement("verticalarea")->Attribute("levels",&temp)!=NULL){
			limitlevels = temp;
		}
		if(settings->FirstChildElement("verticalarea")->Attribute("airtokeep",&temp)!=NULL){
			limitairtokeep = temp;
		}
		if(settings->FirstChildElement("verticalarea")->Attribute("toplevel",&temp)!=NULL){
			limittoplevel = temp;
		}
	}
	



	//load MC material mappings
	mcMats.clear();
	dfMat2mcMat.clear();
	loadMcMats(&doc);

	//load objects
	xmlobjects = doc.FirstChildElement("objects");
	if(xmlobjects == NULL){
		xmlobjects = new TiXmlElement( "objects" );
		doc.LinkEndChild(xmlobjects);
		TiXmlComment *comment = new TiXmlComment("These are the objects we will convert from Dwarf Fortress and how we will represent them in Minecraft\nEach object has a list of mats that is squaresize^3 in size (separated by ',',';', or '|') that is ordered left to right, back to front, top to bottom (i.e. the first items listed move across a row, then the next row in that layer comes, and when one layer is finished, the next layer down is started)\nEach object also has a list of numbers, the same squaresize^3 in size, that represent state or data of the cooresponding material, used to control things like facing direction, etc.");
		xmlobjects->LinkEndChild(comment);
	}
	loadObjects();

	//test
	//int result = testLevel();
	//return result;

	//setup DFHHack
	//DFHack::ContextManager DFMgr("Memory.xml");
    //DFHack::Context *DF;
	DFHack::API DF ("Memory.xml");
    try
    {
        //DF = DFMgr.getSingleContext();
        DF.Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 100;
    }

	//DFHack::Materials * Mats = DF->getMaterials();
	//Mats->ReadAllMaterials();

	

	//convert the map
	int result = convertMaps(&DF/*,Mats*/);



	if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
    }

	doc.SaveFile("updated.xml");

    #ifndef LINUX_BUILD
        cin.ignore();
    #endif
	return result;
}

