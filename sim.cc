#include <iostream>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <cmath>

#include <sstream>
#include <string>
#include <fstream>
#include <bits/stdc++.h>
#include <vector>
#include "sim.h"

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/

using namespace std;
struct cache_level
{
    bool valid;
    int dirty;
    int LRU;
    uint32_t address;

  cache_level() : valid(false), dirty(0), LRU(-1), address(0) {}
};

uint32_t L1_RD = 0, L1_RM = 0, L1_WT=0, L1_WM=0, L1_WB=0;
uint32_t L2_RD = 0, L2_RM = 0, L2_WT=0, L2_WM=0, L2_WB=0;

struct output
{
  uint32_t data;
  int dirty;
};

#define Max_L2_set 1000
#define Max_L2_Assoc 100

//////////////////  Functions Declaration  //////////////
void ReadWrite_Hit(cache_level Level[],int idx, uint32_t Assoc, uint32_t l1_l2, uint32_t r_w);
void Read_Miss(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2,uint32_t r_w);
void Write_Miss(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2, uint32_t r_w);
void ReadWrite_Check(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2, uint32_t r_w);
uint32_t customLog (uint32_t x);
uint32_t Shifter (uint32_t x);
uint32_t LRU_Index (cache_level Level[], uint32_t Assoc);
void LRU_Update (cache_level Level[], uint32_t idx, uint32_t Assoc);
/////////////////////////////////////////////////////////


//////////////////  Log Calculation  ////////////////////
uint32_t customLog (uint32_t x)
{
    uint32_t base = 2,result=0;
    while (x >= base) 
    {		
	x /= base;
	result++;
    }
    return result;
}
/////////////////////////////////////////////////////////

//////////////////  Parsing Logic  //////////////////////
uint32_t Shifter (uint32_t x)
{
    uint32_t i = x, shift = 1;
    for (;i>0;i--)
    {
	shift *= 2;
    }
    shift -= 1;
    return shift;
}
//////////////////////////////////////////////////////////

/////////////////  Max LRU's Tag / Index  /////////////////
uint32_t LRU_Addr_Reframe(uint32_t Tag, uint32_t Idx, uint32_t L1idx ,uint32_t L2idx, uint32_t T_I)
{
  int Addr = (Tag<<L1idx)|Idx;
  int Index = Shifter(L2idx);
  Index = Index & Addr;
  int Tag_addr = Addr>>L2idx;
  if(T_I == 1)
  {
  return Tag_addr;
  }
  else
  {
  return Index;
  }
}
//////////////////////////////////////////////////////////


//////////////////  Max LRU's Index   ////////////////////
uint32_t LRU_Index (cache_level Level[], uint32_t Assoc)
{
   int idx = 0, temp = -1;
   for(uint32_t i = 0; i < Assoc; i++){
    if(Level[i].LRU > temp){
        temp = Level[i].LRU;
        idx = i;
    }
   }
   return idx;
}
//////////////////////////////////////////////////////////

//////////////////  LRU Update   //////////////////////////
void LRU_Update (cache_level Level[], uint32_t idx, uint32_t Assoc)
{
   int temp = Level[idx].LRU;
   for (uint32_t i = 0; i < Assoc; i++)
   {
      if (Level[i].LRU < temp)
      {
      Level[i].LRU = Level[i].LRU + 1;
      }
   }
   Level[idx].LRU = 0;
}
//////////////////////////////////////////////////////////

////////////////// Read / Write Hit //////////////////////
void ReadWrite_Hit(cache_level Level[],int idx, uint32_t Assoc, uint32_t l1_l2, uint32_t r_w)
{
   LRU_Update(Level,idx,Assoc);
   if(l1_l2==1)
   {
      if(r_w == 1)
      {
         L1_RD++;
      }
      else if(r_w == 2)
      {
         L1_WT++;
            if(Level[idx].dirty == 0)
            {
               Level[idx].dirty = 1;
            }
      }
   }
   else if(l1_l2==2)
   {
      if(r_w == 1)
      {
         L2_RD++;
      }
      else if(r_w == 2)
      {
         L2_WT++;
            if(Level[idx].dirty == 0)
            {
               Level[idx].dirty = 1;
            }
      }
   }
}
//////////////////////////////////////////////////////////

////////////////// Read Miss /////////////////////////////
void Read_Miss(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2,uint32_t r_w)
{
  if(l1_l2 == 1)
  {
   int maxLRU = LRU_Index(Level1, L1_ASSOC);
   LRU_Update(Level1, maxLRU, L1_ASSOC);    
   L1_RD++;
   L1_RM++;
   if (Level1[maxLRU].dirty == 1)
   {
      L1_WB++;
      Level1[maxLRU].dirty = 0;
      l1_l2 = 2;
      uint32_t Tag_L2_Ref = LRU_Addr_Reframe(Level1[maxLRU].address,Index_L1,L1idx,L2idx,1);
      uint32_t Index_L2_Ref = LRU_Addr_Reframe(Level1[maxLRU].address,Index_L1,L1idx,L2idx,2);
      ReadWrite_Check(Level1,Index_L1,Tag_L1,L1_ASSOC,level2Cache[Index_L2_Ref],Index_L2_Ref,Tag_L2_Ref,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,2);
   }
   l1_l2 = 2;
   ReadWrite_Check(Level1,Index_L1,Tag_L1,L1_ASSOC,Level2,Index_L2,Tag_L2,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,1);
   Level1[maxLRU].address = Tag_L1;
  }
  else if(l1_l2==2)
  {
    int maxLRU = LRU_Index(Level2, L2_ASSOC);
    L2_RM++;
    if(Level2[maxLRU].dirty == 0)
    {
      L2_RD++;
    }
    else
    {
      L2_RD++;
      L2_WB++;
      Level2[maxLRU].dirty=0;
    }
      Level2[maxLRU].address=Tag_L2;
    LRU_Update(Level2,maxLRU,L2_ASSOC);
  }
}
//////////////////////////////////////////////////////////


////////////////// Write Miss ////////////////////////////
void Write_Miss(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2, uint32_t r_w)
{
  if(l1_l2 == 1)
  {
    int maxLRU = LRU_Index(Level1, L1_ASSOC);
    LRU_Update(Level1,maxLRU,L1_ASSOC);
    L1_WT++;
    L1_WM++;
    if(Level1[maxLRU].dirty == 1)
    {
      L1_WB++;
      l1_l2 = 2;
      uint32_t Tag_L2_Ref = LRU_Addr_Reframe(Level1[maxLRU].address,Index_L1,L1idx,L2idx,1);
      uint32_t Index_L2_Ref = LRU_Addr_Reframe(Level1[maxLRU].address,Index_L1,L1idx,L2idx,2);
  
      ReadWrite_Check(Level1,Index_L1,Tag_L1,L1_ASSOC,level2Cache[Index_L2_Ref],Index_L2_Ref,Tag_L2_Ref,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,2);
    }
      Level1[maxLRU].address = Tag_L1;
      Level1[maxLRU].dirty=1;
    l1_l2 = 2;  
    ReadWrite_Check(Level1,Index_L1,Tag_L1,L1_ASSOC,Level2,Index_L2,Tag_L2,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,1);
  }
  else if(l1_l2 == 2)
  {
    int maxLRU = LRU_Index(Level2, L1_ASSOC);
    LRU_Update(Level2,maxLRU,L1_ASSOC);
    L2_RM++;
    if (Level2[maxLRU].dirty == 0)
    {
      L2_WT++;
      Level2[maxLRU].dirty = 1;
    }
    else if(Level2[maxLRU].dirty==1)
    {
      L2_WB++;
      L2_RD++;
    }
    Level2[maxLRU].address = Tag_L2;
  }
}
//////////////////////////////////////////////////////////


//////////////////  Read / Write check  //////////////////
void ReadWrite_Check(cache_level Level1[], uint32_t Index_L1, uint32_t Tag_L1, uint32_t L1_ASSOC,cache_level Level2[], uint32_t Index_L2, uint32_t Tag_L2, uint32_t L2_ASSOC, uint32_t L1idx, uint32_t L2idx, cache_level level2Cache[Max_L2_set][Max_L2_Assoc], uint32_t l1_l2, uint32_t r_w)
{
  int temp=1;
   if(l1_l2 == 1){
        for(uint32_t i=0;i<L1_ASSOC;i++){
            if(Level1[i].address==Tag_L1)
            {
               ReadWrite_Hit(Level1,i,L1_ASSOC,l1_l2,r_w);
               temp = 0;
            }
        }
   }
   else if(l1_l2 == 2){
        for(uint32_t i=0;i<L2_ASSOC;i++){
            if(Level2[i].address==Tag_L2)
            {
               ReadWrite_Hit(Level2,i,L2_ASSOC,l1_l2,r_w);
               temp = 0;
            }
        }
   }
  

   if(temp==1)
   {
      if(r_w == 1)
      {
         Read_Miss(Level1,Index_L1,Tag_L1,L1_ASSOC,Level2,Index_L2,Tag_L2,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,r_w);
      }
      else if(r_w == 2)
      {
         Write_Miss(Level1,Index_L1,Tag_L1,L1_ASSOC,Level2,Index_L2,Tag_L2,L2_ASSOC,L1idx,L2idx,level2Cache,l1_l2,r_w);
      }
   }
}
//////////////////////////////////////////////////////////


int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");

   //////////////////  Initial Calculation  ////////////////////
	uint32_t BO, L1_set, L1_idx, L1_tag, L2_set, L2_idx, L2_tag;
  uint32_t Max_bits = 32, r_w = 0, l1_l2 = 0;// address;

	L1_set = params.L1_SIZE / ( params.L1_ASSOC * params.BLOCKSIZE );
	L1_idx = customLog( L1_set );	
  BO = customLog( params.BLOCKSIZE);

 if (params.L2_SIZE!=0&&params.L2_ASSOC!=0) {   
	L2_set = params.L2_SIZE / ( params.L2_ASSOC * params.BLOCKSIZE );
	L2_idx = customLog( L2_set );
  L2_tag = Max_bits - L2_idx - BO;
   }
   else{
    L2_set = 0; 
    L2_idx = 0;
    L2_tag = 0;
   }

	L1_tag = Max_bits - L1_idx - BO;
	      
   ////////////////// Cache Declaration /////////////////////
  
   cache_level level1Cache[L1_set][params.L1_ASSOC];
   cache_level level2Cache[Max_L2_set][Max_L2_Assoc];

   //////////////// LRU Initial Update /////////////////////
  for (uint32_t i = 0; i < L1_set; i++)
  {
    for (uint32_t j = 0; j < params.L1_ASSOC; j++)
    {
      level1Cache[i][j].LRU = j;
    }
  }

   for (uint32_t i = 0; i < L2_set; i++)
   {
      for (uint32_t j = 0; j < params.L2_ASSOC; j++)
      {
      level2Cache[i][j].LRU = j;
      }
   }

   //////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////  

   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw == 'r')
      {
         r_w = 1;
      }
      else if (rw == 'w')
      {
         r_w = 2;       
      }
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }

   ////////////////// Parsing Initiation ////////////////////
      
	uint32_t Index_bits_L1, Tag_bits_L1, Index_bits_L2, Tag_bits_L2;//Block_Offset;
	uint32_t L1_IDX_shift = 1;
	uint32_t L1_Tag_shift = 1; 
	uint32_t L2_IDX_shift = 1;
	uint32_t L2_Tag_shift = 1;

	L1_IDX_shift = Shifter(L1_idx);
	L1_Tag_shift = Shifter(L1_tag);
	L2_IDX_shift = Shifter(L2_idx);
	L2_Tag_shift = Shifter(L2_tag);

	L1_IDX_shift = L1_IDX_shift << BO;
	Index_bits_L1 = addr & L1_IDX_shift;
	Index_bits_L1 = Index_bits_L1 >> BO;
	L2_IDX_shift = L2_IDX_shift << BO;
	Index_bits_L2 = addr & L2_IDX_shift;
	Index_bits_L2 = Index_bits_L2 >> BO;

	L1_Tag_shift = L1_Tag_shift << (BO + L1_idx);
	Tag_bits_L1 = addr & L1_Tag_shift;
	Tag_bits_L1 = Tag_bits_L1 >> (BO + L1_idx);
	L2_Tag_shift = L2_Tag_shift << (BO + L2_idx);
	Tag_bits_L2 = addr & L2_Tag_shift;
	Tag_bits_L2 = Tag_bits_L2 >> (BO + L2_idx);
	      
   l1_l2 = 1;
   ReadWrite_Check(level1Cache[Index_bits_L1], Index_bits_L1, Tag_bits_L1, params.L1_ASSOC,level2Cache[Index_bits_L2],Index_bits_L2,Tag_bits_L2,params.L2_ASSOC,L1_idx,L2_idx,level2Cache,l1_l2,r_w);
}

  cout << "===== L1 contents =====" << "\n";
  for (uint32_t i = 0; i < L1_set; i++)
  {
    cout<<"set";
    cout<<std::setw(7)<<i<<":"<<"\t";
    output temp[params.L1_ASSOC];
    for (uint32_t j = 0; j < params.L1_ASSOC; j++)
    {
      temp[level1Cache[i][j].LRU].data=level1Cache[i][j].address;
      temp[level1Cache[i][j].LRU].dirty=level1Cache[i][j].dirty;
    }
    for(uint32_t j=0;j<params.L1_ASSOC;j++){
      if(temp[j].dirty==1){
        printf("%x D\t", temp[j].data);
        printf("\t");
      }     
      else{
        printf("%x\t", temp[j].data);
        printf("\t");
      }
    }
    cout << "\n";
  }
  if(params.L2_ASSOC!=0)
cout << "\n"<< "===== L2 contents =====" << "\n";

  for (uint32_t i = 0; i < L2_set; i++)
  {
    // printf("set\t%d:", i);
    cout<<"set";
    cout<<std::setw(7)<<i<<":";
    cout<<"\t";
    output temp[params.L2_ASSOC];
    for (uint32_t j = 0; j < params.L2_ASSOC; j++)
    {
      temp[level2Cache[i][j].LRU].data=level2Cache[i][j].address;
      temp[level2Cache[i][j].LRU].dirty=level2Cache[i][j].dirty;

      
    }
    for(uint32_t j=0;j<params.L2_ASSOC;j++){
      if(temp[j].dirty==1){
        printf("%x D\t", temp[j].data);
        printf("\t");
      }     
      else{
        printf("%x\t", temp[j].data);
        printf("\t");
      }
    }
    cout << "\n";
  }
double L2_MR=0.0000;
double L1_MR=(double)(L1_WM+L1_RM)/(double)(L1_RD+L1_WT);
if(params.L2_ASSOC)
 L2_MR=(double)L2_RM/(double)L2_RD;
uint32_t MEM_Traffic=0;
if(params.L2_ASSOC)
MEM_Traffic=L2_RM+L2_WM+L2_WB;
else
MEM_Traffic=L1_RM+L1_WM+L1_WB;
uint32_t Prefetch=0;

if(params.L2_ASSOC==0)
 {
  L2_RD=0;
  L2_RM=0;
  L2_WT=0;
  L2_WM=0;
  L2_WB=0;
 }
 cout << dec << endl;
    cout << "===== Measurements =====" << endl;
    cout << "a. L1 reads:                   " << L1_RD << endl;
    cout << "b. L1 read misses:             " << L1_RM << endl;
    cout << "c. L1 writes:                  " << L1_WT<< endl;
    cout << "d. L1 write misses:            " << L1_WM << endl;
    cout << "e. L1 miss rate:               " << setprecision(4) << fixed << L1_MR << endl;
    cout << "f. L1 writebacks:              " << L1_WB << endl;
    cout << "g. L1 prefetches:              " << Prefetch << endl;
    cout << "h. L2 reads (demand):          " << L2_RD << endl;
    cout << "i. L2 read misses (demand):    " << L2_RM << endl;
    cout << "j. L2 reads (prefetch):        " << Prefetch << endl;
    cout << "k. L2 read misses (prefetch):  " << Prefetch << endl;
    cout << "l. L2 writes:                  " << L2_WT << endl;
    cout << "m. L2 write misses:            " << L2_WM << endl;
    cout << "n. L2 miss rate:               " << setprecision(4) << fixed << L2_MR << endl;
    cout << "o. L2 writebacks:              " << L2_WB << endl;
    cout << "p. L2 prefetches:              " << Prefetch << endl;
    cout << "q. memory traffic:             " << MEM_Traffic << endl;  
   return(0);
}
