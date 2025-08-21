#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
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
//void printL1Measurements();
//void printL2Measurements();
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.
   //uint32_t L1_sets;
   //uint32_t L2_sets;
   //uint32_t L1reads;
   //uint32_t L2reads;
   
   

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


   Cache *L2 = nullptr;
   if(params.L2_SIZE != 0){
      L2 = new Cache(params.BLOCKSIZE, params.L2_SIZE , params.L2_ASSOC, params.PREF_N, params.PREF_M, nullptr);      
   }
   Cache L1(params.BLOCKSIZE, params.L1_SIZE , params.L1_ASSOC, params.PREF_N, params.PREF_M, L2);

   //int count;
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      //printf("b\r\n");
      /*if (rw == 'r')
         printf("r %x\n", addr);
      else if (rw == 'w'){         
         printf("w %x\n", addr);
         
      }
      else {
         printf("Error: Unknown request type %c.\n", rw);
	      exit(EXIT_FAILURE);
      }

      

      ///////////////////////////////////////////////////////
      // Issue the request to the L1 cache instance here.
      ///////////////////////////////////////////////////////*/
      /*if (count == 99915)
      {
         printf("99915 FOUND\r\n");
      }
      if (count == 99922){
         printf("99922 FOUND\r\n");
      }
      if (count == 99923){
         printf("99923 FOUND\r\n");
      }*/
      //count++;
      //printf("%d\r\n", count);
      L1.request(rw, addr); 
      
    }

    
      L1.printL1Contents();      
      if(L2 != nullptr) L2->printL2Contents();
      if(L2 != nullptr && params.PREF_N>0) L2->printStream();
      else if(params.PREF_N>0) L1.printStream();
      L1.printL1Measurements();
      if(L2 != nullptr) L2->printL2Measurements();
      else L1.print0L2Measurements();
      

      


    return(0);
}





