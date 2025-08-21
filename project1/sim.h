#ifndef SIM_CACHE_H
#define SIM_CACHE_H
#include <list>
#include <vector>
#include <queue>
#include <cmath>
using namespace std;



uint32_t getIndex(uint32_t size, uint32_t address, uint32_t blocksize, uint32_t assoc);
uint32_t getTag(uint32_t size, uint32_t address, uint32_t blocksize, uint32_t assoc);
uint32_t getBlockAddress(uint32_t address, uint32_t blocksize);
//void printContents();
//void printL1Measurements();
//void printL2Measurements();
//void printMeasurements();


typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

// Put additional data structures here as per your requirement.


class Block {
   public:
   Block(bool valid, bool dirty, uint32_t tag, uint32_t address) : valid(valid), dirty(dirty), tag(tag), address(address) {}
   bool valid = false;  // valid bit
   bool dirty = false;
   uint32_t tag = 0;  // tag identification of block  
   uint32_t address; 

};

class Set{
   public:
   list<Block> blocks;
};

class Buffer{
   public:
   bool valid;
   Buffer(){
      valid = false;
   }
   
   vector<Block> bufferVector;
};

class StreamBuffer{
   public:
   uint32_t numBuffers;
   uint32_t bufferSize;
   uint32_t SIZE;  
   uint32_t BLOCKSIZE; 
   uint32_t ASSOC;
   uint32_t prefetch;
   bool hit;
   list<Buffer*> streamList;
   StreamBuffer(uint32_t size, uint32_t blocksize, uint32_t assoc, uint32_t N, uint32_t M) : numBuffers(N), bufferSize(M), SIZE(size), BLOCKSIZE(blocksize), ASSOC(assoc){
      for(uint32_t i = 0; i<numBuffers; i++){
         Buffer* bufferInit = new Buffer();
         streamList.push_front(bufferInit);
      }
      for(auto it = streamList.begin(); it!=streamList.end(); it++){
         for(uint32_t x = 0; x<bufferSize; x++){
            Block* block = new Block(false, false , 0, 0);
            (*it)->bufferVector.push_back(*block);
         }
      }
   }

   bool access(uint32_t address, bool cacheHit){
      uint32_t BlockAddress = getBlockAddress(address, BLOCKSIZE);
      bool streamHit = false;
      //printf("0\r\n");
      for(auto i = streamList.begin(); i != streamList.end(); i++){
         //printf("1\r\n");
         //queue<Block> bufferCopy = (*i)->bufferQueue;
         //uint32_t counter = 0;
         if((*i)->valid){           
            for(size_t x = 0; x < (*i)->bufferVector.size(); x++){
               //counter++;
               if((*i)->bufferVector[x].address == BlockAddress){
                  for(size_t y = 0; y < bufferSize; y++){
                     //uint32_t addBlockAddress = getBlockAddress(address, BLOCKSIZE);
                     //(*i)->bufferVector[y].address = addBlockAddress + counter;
                     (*i)->bufferVector[y].address = (*i)->bufferVector[y].address + x + 1;
                     (*i)->bufferVector[y].valid = true;
                     
                  }
                  streamList.splice(streamList.begin(), streamList, i);
                  streamHit = true;
                  prefetch = prefetch + x + 1;
                  break;
               }
            }
            if(streamHit) break;
            
         }
         
      }

      //handling a miss in stream buffer with cache miss
      if(!streamHit && !cacheHit){
         auto fillBuffer = std::prev(streamList.end());
         (*fillBuffer)->bufferVector.clear();
            for(uint32_t i = 1; i<=bufferSize; i++){
               //Block* block = new Block(false, false , 0, address + i);
               uint32_t streamBlockAddr = getBlockAddress(address, BLOCKSIZE);
               //uint32_t tag = getTag(SIZE, address + i, BLOCKSIZE, ASSOC);
               Block block(true, false , 0, streamBlockAddr + i);
               (*fillBuffer)->bufferVector.push_back(block);
               prefetch++;
            }
         
         (*fillBuffer)->valid = true;
         //move vector to front (MRU)
         streamList.splice(streamList.begin(), streamList, fillBuffer);
         
      }
      //printStream();
      return streamHit;     
                      
   }
   
};

class Cache {
   public:
   //Cache configuration variables
   uint32_t BLOCKSIZE;
   uint32_t SIZE;
   uint32_t ASSOC;
   uint32_t N;
   uint32_t M;
   Cache *nextMem;
   StreamBuffer *streamBuffer;
   vector<Set*> CacheVector;
   
   //variables printed to console
   uint32_t writes;
   uint32_t reads;
   uint32_t writes_miss;
   uint32_t reads_miss;
   uint32_t writebacks;
   uint32_t reads_miss_prefetch;
   uint32_t reads_prefetch;
   //uint32_t prefetch;
   uint32_t mem_traffic; 
   
   //Cache Constructor
   Cache(uint32_t blocksize, uint32_t size, uint32_t assoc, uint32_t N, uint32_t M, Cache *nextMem) : BLOCKSIZE(blocksize), SIZE(size), ASSOC(assoc), N(N), M(M), nextMem(nextMem) {
      //initialize variables
      writes = 0;
      reads = 0;
      writes_miss = 0;
      reads_miss = 0;
      writebacks = 0;
      reads_miss_prefetch = 0;
      reads_prefetch = 0;
      //prefetch = 0;
      mem_traffic = 0;
      
      //initialize cache with invalid addresses
      int setsInit = size/(assoc*blocksize); 
      for(int i = 0; i<setsInit; i++){
         Set* setInit = new Set();
         CacheVector.push_back(setInit);
      }

      for(int i = 0; i < setsInit; i++){
         for(uint32_t x = 0; x<assoc; x++){
            Block* block = new Block(false, false , 0, 0);
            CacheVector[i]->blocks.push_front(*block);
         }
      }

      //initialize stream buffers
      if(N>0 && nextMem == nullptr) streamBuffer = new StreamBuffer(SIZE, BLOCKSIZE, ASSOC, N, M); 
  
      
   }

   int numsets = SIZE/(ASSOC*BLOCKSIZE);
   void request(char rw, uint32_t address){        
      bool miss = true;
      bool streamHit = false;
      int index = getIndex(SIZE, address, BLOCKSIZE, ASSOC);
      uint32_t tag = getTag(SIZE, address, BLOCKSIZE, ASSOC);

      //loop through the cache ASSOC
      //check tag of each 
      Set *SetIndex = CacheVector[index];      
      for(auto i = SetIndex->blocks.begin(); i != SetIndex->blocks.end(); i++){
         //handling a HIT
         if(i->tag == tag && i->valid){
            if(rw == 'r') reads++;
            else{
               i->tag = tag;
               i->dirty = true;
               writes++;                           
            }           

            //move block to front (block is MRU)
            SetIndex->blocks.splice(SetIndex->blocks.begin(), SetIndex->blocks, i);
            miss = false;
            if(N>0 && nextMem == nullptr){
               streamHit = streamBuffer->access(address, true);
               //printStream();
               //prefetch++;
            } 
            break;
         }         
      }

      //handling a miss, allocate block in cache
      if(miss){        
         bool Valid = true; 
         if(N>0 && nextMem == nullptr){
            streamHit = streamBuffer->access(address, false); 
            //printStream();
            //prefetch++;
         } 
         //else streamHit = false;

         
         for(auto i = SetIndex->blocks.begin(); i != SetIndex->blocks.end(); i++){ 
         //handling a miss and allocating into invalid block 
            if(!i->valid && rw == 'r'){
               i->valid = true;
               i->dirty = false;
               i->tag = tag;
               i->address = address;
               reads++;
               //reads_miss++;
               Valid = false;   
               SetIndex->blocks.splice(SetIndex->blocks.begin(), SetIndex->blocks, i);     
               break;          
            }
            else if(!i->valid && rw == 'w'){
               i->valid = true;
               i->dirty = true;
               i->tag = tag;
               i->address = address;   
               writes++;
               //writes_miss++;
               Valid = false;               
               SetIndex->blocks.splice(SetIndex->blocks.begin(), SetIndex->blocks, i); 
               break;              
            }
         }
         //handling a miss and allocating into valid block
         if(Valid){               
            auto victim = std::prev(SetIndex->blocks.end());
            if(victim->dirty){
               if(nextMem != nullptr) nextMem->request('w', victim->address);
               else mem_traffic++;
               writebacks++;
            }             

            if(rw == 'r'){                 
               victim->tag = tag;
               victim->dirty = false;
               victim->address = address;
               victim->valid = true;
               reads++;
               //reads_miss++;
            } 
            else{
               victim->tag = tag;
               victim->dirty = true;
               victim->address = address;
               victim->valid = true;
               writes++;
               //writes_miss++;
            }
            //move item to front of list (MRU)
            SetIndex->blocks.splice(SetIndex->blocks.begin(), SetIndex->blocks, victim);
            Valid = true;
         }

         //if there is a stream buffer, execute this
         if(N>0 && nextMem == nullptr){
            if(!streamHit){
               if(rw == 'r') reads_miss++;
               else writes_miss++;
            }       
         
         }
         //no stream buffer
         else{
            if(rw == 'r') reads_miss++;
            else writes_miss++;
         }

         //search next level of memory for the missed block
         if(nextMem != nullptr && !streamHit) nextMem->request('r', address);            
         else if(nextMem == nullptr && !streamHit) mem_traffic++;
         miss = false;
         
             
           
      }

      //printStream();   

   }

   void printL1Contents(){
      printf("===== L1 contents =====\n");
      for(int i = 0; i<numsets; i++){          
         printf("set      %d:   ", i);
         for(auto& it : CacheVector[i]->blocks){ //set index
            printf("%x ", it.tag);
            if(it.dirty) printf("D  "); 
            else printf("   ");                       
         }
         printf("\n");
         
      }   
      printf("\n");     
   
   }
   void printL2Contents(){     
      printf("===== L2 contents =====\n");
      for(int i = 0; i<numsets; i++){ //
         printf("set      %d:   ", i);
         for(auto& it : CacheVector[i]->blocks){ //set index
            printf("%x ", it.tag);
            if(it.dirty) printf("D  ");
            else printf("   ");                        
         }
         printf("\n");            
      }
      printf("\n");
      
   }

   void printStream(){
      printf("===== Stream Buffer(s) contents =====\n");
      for(auto& it: streamBuffer->streamList){
         if(it->valid){
            for(uint32_t i = 0; i<M; i++){
            printf(" %x  ", it->bufferVector[i].address);
            }
            printf("\n");
         }
         
      }
      printf("\n");     
   }
   
   void printL1Measurements(){      
      printf("===== Measurements =====\n");
      printf("a. L1 reads:                   %d\n",reads);
      printf("b. L1 read misses:             %d\n",reads_miss);
      printf("c. L1 writes:                  %d\n",writes);
      printf("d. L1 write misses:            %d\n",writes_miss);
      printf("e. L1 miss rate:               %.4f\n",(float)(reads_miss+writes_miss)/(reads+writes));
      printf("f. L1 writebacks:              %d\n",writebacks);
      if(N>0 && nextMem == nullptr)printf("g. L1 prefetches:              %d\n",streamBuffer->prefetch); 
      else printf("g. L1 prefetches:              0\n");  

   }

   void printL2Measurements(){
      printf("h. L2 reads (demand):          %d\n",reads);
      printf("i. L2 read misses (demand):    %d\n",reads_miss);
      printf("j. L2 reads (prefetch):        %d\n",reads_prefetch);
      printf("k. L2 read misses (prefetch):  %d\n",reads_miss_prefetch);
      printf("l. L2 writes:                  %d\n",writes);
      printf("m. L2 write misses:            %d\n",writes_miss);
      printf("n. L2 miss rate:               %.4f\n",(float)(reads_miss)/(reads));
      printf("o. L2 writebacks:              %d\n",writebacks);
      if(N>0 && nextMem == nullptr){
         printf("p. L2 prefetches:              %d\n",streamBuffer->prefetch);
         printf("q. memory traffic:             %d\n",mem_traffic + streamBuffer->prefetch);
      } 
      else{
         printf("p. L2 prefetches:              0\n");
         printf("q. memory traffic:             %d\n",mem_traffic);
      } 
      

   }

   void print0L2Measurements(){
      printf("h. L2 reads (demand):          0\n");
      printf("i. L2 read misses (demand):    0\n");
      printf("j. L2 reads (prefetch):        0\n");
      printf("k. L2 read misses (prefetch):  0\n");
      printf("l. L2 writes:                  0\n");
      printf("m. L2 write misses:            0\n");
      printf("n. L2 miss rate:               0.0000\n");
      printf("o. L2 writebacks:              0\n");
      printf("p. L2 prefetches:              0\n");
      if(N>0){
         printf("q. memory traffic:             %d\n",mem_traffic + streamBuffer->prefetch);
      } 
      else{
         printf("q. memory traffic:             %d\n",mem_traffic);
      }       
   }  

   
   

};


//external functions
uint32_t getBlockAddress(uint32_t address, uint32_t blocksize){
   int blockOffset = log2(blocksize);
   uint32_t BlockAddress = (address >> blockOffset);
   return BlockAddress;
}

uint32_t getIndex(uint32_t size, uint32_t address, uint32_t blocksize, uint32_t assoc){
   int sets = size/(assoc*blocksize);
   int blockOffset = log2(blocksize);
   int indexBits = log2(sets);
   int indexShifted = (1 << indexBits) - 1;
   uint32_t index = (address >> blockOffset) & indexShifted;
   return index;
}

uint32_t getTag(uint32_t size, uint32_t address, uint32_t blocksize, uint32_t assoc){
   int sets = size/(assoc*blocksize);
   int blockOffset = log2(blocksize);
   int indexBits = log2(sets);
   uint32_t tag = (address >> blockOffset >> indexBits);
   return tag;
}

#endif



/*while(!bufferCopy.empty()){
            if(bufferCopy.front().tag == tag){               
               //move block to front (block is MRU)
               streamList.splice(streamList.begin(), streamList, i);
               bufferCopy.pop();
               counter++; 
               hit = true;
               for(uint32_t it = 0; it<counter; it++){

               }
               break;               
            }
            bufferCopy.pop(); */


                     //cache miss, stream miss
         
         //cache miss, stream hit
         /*else if(miss && streamHit){
            if(rw == 'r') reads_miss++;
            else writes_miss++;
         }*/
         //cache hit, stream miss
         /*else if(!miss && !streamHit){
            if(rw == 'r') reads_miss++;
            else writes_miss++;
         }*/
         //cache hit, stream hit
        /* else if(!miss && streamHit){
            if(rw == 'r') reads_miss++;
            else writes_miss++; 
         }*/