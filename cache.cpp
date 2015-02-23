/*
* File name: cache.cpp
* File abstract: this is a programm to simulate cache coherent, 
*                this file does implementations for all public functions,
*                designing data structure, extracting internal fuctions, handling I/O
* 
* Version: 1.0
* Author: Xiaoming Sun
* Date: 2014-04-27
*/

#include "cache.h" // reference to cache head files

/*
* memory_size is the size of memory
* cache_line_size is the size of cache line
* cache_size_l3 is the size of l3
*/
obj_size memory_size, cache_line_size, cache_size_l3;

/*
* replacement_speed is the speed of cache replacement
* broadcast_speed is the speed of broadcast
* memory_access_speed is the access speed of memory
* cache_access_speed_l3 is the access speed of cache L3
*/
obj_time replacement_speed, broadcast_speed, memory_access_speed, cache_access_speed_l3;

/*
* memory_pages is the total pages of memory, divided by cache line size
* total_block_l3 is the total blocks in cache L3, divided by cache line size
*/
unsigned long memory_pages, total_block_l3;

/*
* number_of_chips is the number of chips, default is 1
* array_usage_l3 is an array to track the usage of cache L3
*/
int number_of_chips=1, *array_usage_l3;

/*
* commands[9] is an array to record commands for ordering and usage
* funcNames[9] is an array to record function names for ordering and usage
*/
bool commands[9];
string funcNames[9] = {"memorySize", "numOfChips",
                       "numOfCores", "cacheLineSize",
                       "cacheSize", "cacheAccessSpeed",
                       "replacementSpeed", "broadcastSpeed", "memoryAccessSpeed"};
/*
* l2ID is to record all block IDs used in cache L2
* l2State is to record all all state used in cache L2
* l3ID is to record all block IDs used in cache L3
* l3State is to record all all state used in cache L3
*/
string l2ID = "";
string l2State = "";
string l3ID = "";
string l3State = "";
        
/*
* struct entry
*	valid is int to record this entry is valid or not, initial value is 0, means invalid
* 	block_id is the ID of using block in cache
*   memory_page is the page ID of using page in memory
*   state is the status of block, four values(M,E,S,I)
*/
struct entry
{
    int valid;
    int block_id;
    unsigned long memory_page;
    char state;     
};

entry *tlb_l3; // TLB for cache L3

/*
* struct chip
*	number_of_core is the number of cores in this chip
* 	cache_size_l2 is the size of cache L2
*   cache_access_speed_l2 is the access speed of cache L2
*   total_block_l2 is the total blocks of cache L2
*   array_usage_l2 is an array to track the usage of cache L2
*   tlb_l2 is TLB for cache L2
*/
struct chip
{
    int number_of_core;
    obj_size cache_size_l2;
    obj_time cache_access_speed_l2;
    int total_block_l2;
    int *array_usage_l2;
    entry *tlb_l2;            
};

chip *array_chips; // an array to record all chips

/*
* struct opt_time
*	opt is the operation, like miss, hit, write, read
* 	time is the elapse time of each operation
*   count is the number of each operation
*/
struct opt_time
{
       string opt;
       obj_time time;
       int count;
};

opt_time result_time[100]; // an array to record time results
int result_index=0; // an index to tell how many time results it has

/*
* declare internal functions
*/
void _checkValidInput(string strLine); // check if input is valid
string _getFunctionName(const string strLine); // get function name from input string
obj_size _getSize(const string strLine, bool isRW); // get size from input string, isRW is 1, means it is read or write
obj_time _getTime(const string strLine); // get time from input string
int _getNumber(const string strLine, bool isRW); // get number from input string
int _getChipID(const string strLine); // get chip ID from input string
string _getAddress(const string strLine); // get address from input string
unsigned long _caculateTotalBlocks(obj_size size); // caculate total blocks
unsigned long _caculateNeedBlocks(unsigned long address, obj_size size); // culate need blocks in cache
int _checkCacheL2(int chipID, unsigned long loadingPage, bool isAddTime); // check whether it is existing in cache L2
int _checkCacheL3(int chipID, unsigned long loadingPage, bool isAddTime); // check whether it is existing in cache L3
void _addResultTime(string opt, obj_time time); // add operation and time into result
int _findAvailableBlockInCacheL2(int chipID); // find empty block in cache L2
int _findAvailableBlockInCacheL3(); // find empty block in cache L3
entry _takeTheFirstOutByLRU(int chipID); // take one out according to LRU from cache L2
entry _takeTheFirstOutL3ByLRU(); // take one out according to LRU from cache L3
void _swapTLBByLRU(int chipID, int target, int source); // swap the latest access to the last one in L2 TLB by LRU
void _swapTLBL3ByLRU(int target, int source); // swap the latest access to the last one in L3 TLB by LRU
int _getTLBL2UsedBlocks(int chipID); // get how many TLB entries in L2
int _getTLBL3UsedBlocks(); // get how many TLB entries in L3
void _readFromCacheL2(int chipID, int coreID, int tblIndex); // read data from cache L2
void _readFromCacheL3(int chipID, int coreID, int tblIndex); // read data from cache L3
void _loadMemToCacheL2(int chipID, int coreID, int loadingPage); // load data from memory to L2
void _writeToCacheL2(int chipID, int coreID, int loadingPage); // write data into cache L2
void _writeToCacheL3(int chipID, int coreID, int loadingPage, bool isRead); // write data into cache L3
void _rewriteToCacheL2(int chipID, int coreID, int tlbIndex); // rewrite data into cache L2
void _rewriteToCacheL3(int chipID, int coreID, int tlbIndex, int loadingPage); // rewrite data into cache L3
void _printResult(int chipID, int pages[], int pageSize); // print out result
void _checkCommandsOrder(int index); // check commands are in correct order
void _checkCommandsReady(); // check commands are enough
void _checkValidIDs(int chipID, int coreID); // check chipID and coreID is valid
string _num2str(int number); // convert number to string

/*
* main function, this is the program entry to invoke all other functions
*/
int main(int argc, char *argv[])
{
    string strLine;
    
    for(int i=0; i<9; i++)
    {
        commands[i] = 0;
    }
    
    while(getline(cin, strLine))
    {
        if(strLine[0] == '#') continue;
        
        cout << strLine << endl;
        
        // check if input is valid
        _checkValidInput(strLine);
        
        // get the function name from the input
        string name = _getFunctionName(strLine);
        
        // call memorySize function
        if(name == "memorySize")
        {
                commands[0] = 1;
                memorySize(_getSize(strLine, 0));
                continue;
        }// call numOfChips function, optional, if not input, means 1 chip 
        else if(name == "numOfChips")
        {
                if(commands[0])
                {
                   if(!commands[2])
                   {
                       commands[1] = 1;
                   }
                   else
                   {
                       cout << "Input error, this must be the second command!" << endl;
                       exit(1);
                   }
                }
                else
                {
                   cout << "Not input memory size, it must be the first command!" << endl;
                   exit(1); 
                }
                
                numOfChips(_getNumber(strLine, 0));
                continue;
        }// call numOfCores function, if not input Chip ID, means the same number for all chips 
        else if(name == "numOfCores")
        {       
                if(commands[1])
                {
                    commands[2] = 1;
                }
                else if(commands[0])
                {
                    commands[2] = 1; 
                }
                else
                {
                   cout << "Not input memory size, it must be the first command!" << endl;
                   exit(1); 
                }
                
                numOfCores(_getChipID(strLine), _getNumber(strLine, 0));
                continue;
        }// call cacheLineSize function 
        else if(name == "cacheLineSize")
        {
                _checkCommandsOrder(3);
                
                cacheLineSize(_getSize(strLine, 0));
                continue;
        }// call cacheSize function
        else if(name == "cacheSize")
        {
                _checkCommandsOrder(4);
             
                cacheSize(_getChipID(strLine), _getSize(strLine, 0));
                continue;
        }// call cacheAccessSpeed function
        else if(name == "cacheAccessSpeed")
        {
                _checkCommandsOrder(5);
                
                cacheAccessSpeed(_getChipID(strLine), _getTime(strLine));
                continue;
        }// call replacementSpeed function
        else if(name == "replacementSpeed")
        {
                _checkCommandsOrder(6);
                replacementSpeed(_getTime(strLine));
                continue;
        }// call broadcastSpeed function
        else if(name == "broadcastSpeed")
        {
                _checkCommandsOrder(7);
                broadcastSpeed(_getTime(strLine));
                continue;
        }// call memoryAccessSpeed function
        else if(name == "memoryAccessSpeed")
        {
                _checkCommandsOrder(8);
                memoryAccessSpeed(_getTime(strLine));
                continue;
        }// call write function
        else if(name == "read")
        {
                _checkCommandsReady();
                read(_getChipID(strLine), _getNumber(strLine, 1), _getAddress(strLine), _getSize(strLine, 1));
                continue;
        }// call write function
        else if(name == "write")
        {
                _checkCommandsReady();
                write(_getChipID(strLine), _getNumber(strLine, 1), _getAddress(strLine), _getSize(strLine, 1));
                continue;
        }// not match with any function name
        else
        {
            cout << "Not found expected function, skip it!" << endl;
        }
    }
    
    return EXIT_SUCCESS;
}

void memorySize(obj_size size)
{
     memory_size = size;
     //cout << "Memory Size=" << memory_size.data << UnitSizeNames[memory_size.unit] << endl;
}

void numOfChips(int number)
{
     number_of_chips = number;
     
     // initialize array_chips as expected number
     array_chips = new chip[number_of_chips];
     
     //cout << "Total Chips=" << number_of_chips << endl;
}

void numOfCores(int chipID, int number)
{
     // if only one chip, initialize array_chips as 1
     if(number_of_chips == 1) 
     {
         array_chips = new chip[1];               
     }

     // if chipID=-1, means no chipID from input, then all chips have the same number of cores
     // if chipID!=-1, set specified core number for that chip 
     if(chipID == -1) 
     {
         for(int i=0; i<number_of_chips; i++) 
         {
             array_chips[i].number_of_core = number;
         }
     } 
     else 
     {
         array_chips[chipID].number_of_core = number;  
     }
     
     // print chip id and core number
     //for(int i=0; i<number_of_chips; i++) 
//     {
//         cout << "Chip " << i << ": " << array_chips[i].number_of_core << " cores" << endl;
//     }
}

void cacheLineSize(obj_size size)
{
      cache_line_size = size;
      
      // caculate the total memory pages
      memory_pages = _caculateTotalBlocks(memory_size);
       
      //cout<< "Total Memory Pages=" << memory_pages << endl; 
}

void cacheSize(int chipID, obj_size size)
{
     // check cacheLineSize and cacheSize
     if(cache_line_size.unit > size.unit || (cache_line_size.unit == size.unit && cache_line_size.data > size.data))
     {
         cout << "_checkValidInput::CacheLineSize is bigger than CacheSize, invalid input!" << endl;
         exit(1);                       
     }
     
     // if chipID=-1, means no chipID from input
     // if chipID!=-1, set specified number for that chip 
     if(chipID == -1) 
     {
         if(number_of_chips > 1)
         {
             cache_size_l3 = size;
             total_block_l3 = _caculateTotalBlocks(size);
             
             // construct and initialize TLB for L3
             tlb_l3 = new entry[total_block_l3];
             
             for(int i=0; i<total_block_l3; i++) 
             {
                 tlb_l3[i].valid = 0;
                 tlb_l3[i].state = 'I';
                 tlb_l3[i].block_id = -1;
                 tlb_l3[i].memory_page = 0;   
             }
             
             // construct and initialize the L3 usage array
             array_usage_l3 = new int[total_block_l3];
             
             for(int i=0; i<total_block_l3; i++)
             {
                 array_usage_l3[i] = -1;            
             }
             
             cout << "L3=" << cache_size_l3.data << UnitSizeNames[cache_size_l3.unit] << endl;
             cout<< "Total L3 Blocks=" << total_block_l3 << endl;
             return;                  
         }
         else
         {
             chipID = 0;
         }
     } 
    
     // initialize L2 of current chip
     chip currentChip;
     
     currentChip.cache_size_l2 = size;
     currentChip.total_block_l2 = _caculateTotalBlocks(size);
     
     // construct and initialize TLB for L2
     int length = currentChip.total_block_l2;
     currentChip.tlb_l2 = new entry[length];
         
     for(int i=0; i<length; i++) 
     {
         currentChip.tlb_l2[i].valid = 0;
         currentChip.tlb_l2[i].state = 'I';
         currentChip.tlb_l2[i].block_id = -1;
         currentChip.tlb_l2[i].memory_page = 0;        
     } 
         
     // construct and initialize the L2 usage array
     currentChip.array_usage_l2 = new int[length];
              
     for(int i=0; i<length; i++)
     {
         currentChip.array_usage_l2[i] = -1;            
     }    
     
     // set it into array_chips
     array_chips[chipID] = currentChip;
     
     // print chip info
    
     //cout << "Chip " << chipID << ", L2=" << array_chips[chipID].cache_size_l2.data
     //<< UnitSizeNames[array_chips[chipID].cache_size_l2.unit] 
     //<< ", Total L2 Blocks=" << array_chips[chipID].total_block_l2 << endl;
}

void cacheAccessSpeed(int chipID, obj_time time)
{
     // if chipID=-1, means no chipID from input
     // if chipID!=-1, set specified number for that chip 
     if(chipID == -1) 
     {
         if(number_of_chips > 1)
         {
             cache_access_speed_l3 = time;
             
             cout << "L3 Access Speed=" << cache_access_speed_l3.data << UnitTimeNames[cache_access_speed_l3.unit] << endl;
             return;                  
         }
         else
         {
              chipID = 0;
         }
     } 
    
     array_chips[chipID].cache_access_speed_l2= time;
     
     // print chip info
     //cout << "Chip " << chipID << ", L2 Access Speed=" << array_chips[chipID].cache_access_speed_l2.data 
     //<< UnitTimeNames[array_chips[chipID].cache_access_speed_l2.unit] << endl;
}

void replacementSpeed(obj_time time)
{
     replacement_speed = time;
     
     //cout << "Replacement Speed=" << replacement_speed.data << UnitTimeNames[replacement_speed.unit] << endl;
}

void broadcastSpeed(obj_time time)
{
     broadcast_speed = time;
     
     //cout << "Broadcast Speed=" << broadcast_speed.data << UnitTimeNames[broadcast_speed.unit] << endl;
}

void memoryAccessSpeed(obj_time time)
{
     memory_access_speed = time;
     
     //cout << "Memory Access Speed=" << memory_access_speed.data << UnitTimeNames[memory_access_speed.unit] << endl;
}

void read(int chipID, int coreID, string address, obj_size size)
{
   // if(chipID == -1)
//    {
//        cout << "coreID=" << coreID 
//        << ",address=" << address << ",size=" << size.data << UnitSizeNames[size.unit] << endl;      
//    }
//    else
//    {
//        cout << "chipID=" << chipID << ",coreID=" << coreID 
//        << ",address=" << address << ",size=" << size.data << UnitSizeNames[size.unit] << endl;
//    }
    
    unsigned long loadingPage = strtoul(address.c_str(), NULL, 16)/cache_line_size.data;
    
    if(loadingPage > memory_pages)
    {
        cout << "Invalid address, out of memory pages range, ignore this command!" << endl;
        return;               
    }
    
    int loadingPageSize = _caculateNeedBlocks(strtoul(address.c_str(), NULL, 16), size);
    
    cout << "loading page is " << loadingPage << ",page size is " << loadingPageSize << endl;
    
    int pages[loadingPageSize];
     
    if(chipID == -1) 
    {
        chipID = 0;           
    }
    
    _checkValidIDs(chipID, coreID);
    
    chip currentChip = array_chips[chipID];
    
    for(int i=0; i<loadingPageSize; i++, loadingPage++) 
    {  
       pages[i] = loadingPage;
       
       int isExisting = _checkCacheL2(chipID, loadingPage, 1);
       
       // if not -1, find it in L2, read it
       if(isExisting != -1)
       {
           _readFromCacheL2(chipID, coreID, isExisting);   
       }
       else
       {
           // not in L2 ,try L3
           if(number_of_chips > 1)
           {
               isExisting = _checkCacheL3(chipID, loadingPage, 1);
               
               // if not -1, find it in L3
               if(isExisting != -1)
               {
                   _readFromCacheL3(chipID, coreID, isExisting);
               }
               else
               {
                   // load from memory and write L2
                   _loadMemToCacheL2(chipID, coreID, loadingPage);
                   
                   // write L3
                   _writeToCacheL3(chipID, coreID, loadingPage, 1);
               }
           }
           else
           {
               // load from memory and write L2
               _loadMemToCacheL2(chipID, coreID, loadingPage);
           }
       }    
    }
    
    _printResult(chipID, pages, loadingPageSize);
}

void write(int chipID, int coreID, string address, obj_size size)
{
    //cout << "write test enter" << endl;
    
    //if(chipID == -1)
//    {
//        cout << "coreID=" << coreID 
//        << ",address=" << address << ",size=" << size.data << UnitSizeNames[size.unit] << endl;      
//    }
//    else
//    {
//        cout << "chipID=" << chipID << ",coreID=" << coreID 
//        << ",address=" << address << ",size=" << size.data << UnitSizeNames[size.unit] << endl;
//    }
    
    unsigned long loadingPage = strtoul(address.c_str(), NULL, 16)/cache_line_size.data;
    
    if(loadingPage > memory_pages)
    {
        cout << "Invalid address, out of memory pages range, ignore this command!" << endl << endl;
        return;               
    }
    
    int loadingPageSize = _caculateNeedBlocks(strtoul(address.c_str(), NULL, 16), size);
    
    cout << "loading page is " << loadingPage << ",page size is " << loadingPageSize << endl;
     
    if(chipID == -1) 
    {
        chipID = 0;           
    }
    
    _checkValidIDs(chipID, coreID);
    
    int pages[loadingPageSize];
    chip currentChip = array_chips[chipID];
    
    for(int i=0; i<loadingPageSize; i++, loadingPage++) 
    {
       pages[i] = loadingPage;
       
       int isExisting = _checkCacheL2(chipID, loadingPage, 1);
       
       if(isExisting != -1)
       {
           // rewrite L2
           _rewriteToCacheL2(chipID, coreID, isExisting);
           
           if(number_of_chips > 1)
           {
               isExisting = _checkCacheL3(chipID, loadingPage, 0);
               
               // rewrite L3 and mark other L2 as invalid
               _rewriteToCacheL3(chipID, coreID, isExisting, loadingPage);    
           }
       }
       else
       {
           // write L2
           _writeToCacheL2(chipID, coreID, loadingPage);
           
           if(number_of_chips > 1)
           {
               isExisting = _checkCacheL3(chipID, loadingPage, 1);
               
               if(isExisting != -1)
               {
                   // if exist in L3, rewrite L3 and mark other L2 as invalid
                   _rewriteToCacheL3(chipID, coreID, isExisting, loadingPage); 
               }
               else
               {
                   // if not exist, write L3
                   _writeToCacheL3(chipID, coreID, loadingPage, 0);
               }   
           }
       }    
    }
        
    _printResult(chipID, pages, loadingPageSize);   
}

string _getFunctionName(const string strLine)
{   
    string:: size_type position;
    
    position = strLine.find("(");
    
    return strLine.substr(0, position);
}

obj_size _getSize(const string strLine, bool isRW)
{
    //cout << "_getSize test enter" << endl;     
    string:: size_type pos_start;
    string:: size_type pos_end;
       
    obj_size size;
    
    if(isRW)
    {
        if(_getChipID(strLine) == -1) 
        {
            pos_start = strLine.find(",");
            pos_start = strLine.find(",", pos_start+1);
            pos_end = strLine.find(")");
        }
        else
        {
            pos_start = strLine.find(",");
            pos_start = strLine.find(",", pos_start+1);
            pos_start = strLine.find(",", pos_start+1);
            pos_end = strLine.find(")");
        }
        
        // remove the space before the number
        while(isspace(strLine.at(pos_start+1))) {pos_start++;};    
    }
    else
    {
        if(_getChipID(strLine) == -1) 
        {
            pos_start = strLine.find("(");
            pos_end = strLine.find(")");
        }
        else
        {
            pos_start = strLine.find(",");
            pos_end = strLine.find(")");
            
            // remove the space before the number
            while(isspace(strLine.at(pos_start+1))) {pos_start++;};    
        }
    }
    
    //cout << "pos_start is " << strLine[pos_start] << endl;
    //cout << "pos_end is " << strLine[pos_end] << endl;
       
    if(pos_start != strLine.npos && pos_end != strLine.npos && pos_start < pos_end) 
    {
        string temp = "";
        
        if(!isdigit(strLine.at(pos_start+1))) 
        {
            cout << "_getSize::Not found expected number, invalid input!" << endl;
            exit(1);
        }
        
        // get the number part
        while(isdigit(strLine.at(++pos_start))) {temp+=strLine.at(pos_start);};
        size.data = atol(temp.c_str());
        
        // remove the space between number and unit
        while(isspace(strLine.at(pos_start))) {pos_start++;};
        
        // get the unit part, like B, KB, MB, or GB
        if(strLine[pos_start] == 'B' && strLine[pos_start+1] == ')') 
        {
           size.unit = B;             
        } 
        else if(strLine[pos_start] == 'K' && strLine[pos_start+1] == 'B' && strLine[pos_start+2] == ')') 
        {
           size.unit = KB;     
        } 
        else if(strLine[pos_start] == 'M' && strLine[pos_start+1] == 'B' && strLine[pos_start+2] == ')') 
        {
           size.unit = MB; 
        } 
        else if(strLine[pos_start] == 'G' && strLine[pos_start+1] == 'B' && strLine[pos_start+2] == ')') 
        {
           size.unit = GB;     
        } 
        else 
        {
           cout << "_getSize::Not found expected unit(B, KB, MB or GB), invalid input!" << endl;
           exit(1);
        }
        
    } 
    else 
    {
        cout << "_getSize::Not found expected character, invalid input!" << endl;
        exit(1);
    }
    
    //cout << "_getSize test end" << endl;
    
    return size;
}

obj_time _getTime(const string strLine)
{
    //cout << "_getTime test enter" << endl;
    string:: size_type pos_start;
    string:: size_type pos_end;
       
    obj_time time;
    
    if(_getChipID(strLine) == -1) 
    {
        pos_start = strLine.find("(");
        pos_end = strLine.find(")");
    }
    else
    {
        pos_start = strLine.find(",");
        pos_end = strLine.find(")");
        
        // remove the space before the number
        while(isspace(strLine.at(pos_start+1))) {pos_start++;};    
    }
    
    //cout << "pos_start is " << strLine[pos_start] << endl;
    //cout << "pos_end is " << strLine[pos_end] << endl;
       
    if(pos_start != strLine.npos && pos_end != strLine.npos && pos_start < pos_end) 
    {
        string temp = "";
        
        if(!isdigit(strLine.at(pos_start+1))) 
        {
            cout << "_getTime::Not found expected number, invalid input!" << endl;
            exit(1);
        }
        
        // get the number part
        while(isdigit(strLine.at(++pos_start))) {temp+=strLine.at(pos_start);};
        time.data = atol(temp.c_str());
        
        // remove the space between number and unit
        while(isspace(strLine.at(pos_start))) {pos_start++;};
        
        // get the unit part, like us, ns
       
        if(strLine[pos_start] == 'u' && strLine[pos_start+1] == 's' && strLine[pos_start+2] == ')') 
        {
           time.unit = us;     
        } 
        else if(strLine[pos_start] == 'n' && strLine[pos_start+1] == 's' && strLine[pos_start+2] == ')') 
        {
           time.unit = ns; 
        } 
        else 
        {
           cout << "_getTime::Not found expected unit(us, ns), invalid input!" << endl;
           exit(1);
        }
        
    } 
    else 
    {
        cout << "_getTime::Not found expected character, invalid input!" << endl;
        exit(1);
    }
    
    //cout << "_getTime test end" << endl;
    
    return time;
}

int _getNumber(const string strLine, bool isRW)
{
    //cout << "_getNumber test enter" << endl; 
    string:: size_type pos_start;
    string:: size_type pos_end;
       
    int data;
    
    if(isRW)
    {
        if(_getChipID(strLine) == -1) 
        {
            pos_start = strLine.find("(");
            pos_end = strLine.find(",");
        }
        else
        {
            pos_start = strLine.find(",");
            pos_end = strLine.find(",", pos_start+1);
            
            // remove the space before the number
            while(isspace(strLine.at(pos_start+1))) {pos_start++;};    
        }    
    }
    else
    {
        if(_getChipID(strLine) == -1) 
        {
            pos_start = strLine.find("(");
            pos_end = strLine.find(")");
        }
        else
        {
            pos_start = strLine.find(",");
            pos_end = strLine.find(")");
            
            // remove the space before the number
            while(isspace(strLine.at(pos_start+1))) {pos_start++;};    
        }
    }
   
    //cout << "pos_start is " << strLine[pos_start] << endl;
    //cout << "pos_end is " << strLine[pos_end] << endl;
    
    if(pos_start != strLine.npos && pos_end != strLine.npos && pos_start < pos_end) 
    {
        string temp = "";
        
        if(!isdigit(strLine.at(pos_start+1))) 
        {
            cout << "_getNumber::Not found expected number, invalid input!" << endl;
            exit(1);
        }
        
        // get the number part
        while(isdigit(strLine.at(++pos_start))) {temp+=strLine.at(pos_start);};
        data = atol(temp.c_str());
        
        if(isRW)
        {
            if(strLine[pos_start] != ',') 
            {
               cout << "_getNumber::Not found expected ',', invalid input!" << endl;
               exit(1);
            }    
        }
        else
        {
            if(strLine[pos_start] != ')') 
            {
               cout << "_getNumber::Not found expected ')', invalid input!" << endl;
               exit(1);
            }
        }
    } 
    else 
    {
        cout << "_getNumber::Not found expected character, invalid input!" << endl;
        exit(1);
    }
    
    //cout << "_getNumber test end" << endl;
    
    return data;
}

string _getAddress(const string strLine)
{
    //cout << "_getAddress test enter" << endl;
    string:: size_type pos_start;
    string:: size_type pos_end;
       
    string data;
    
    if(_getChipID(strLine) == -1) 
    {
        // if not chipID input, address is between the first "," and the second ","
        pos_start = strLine.find(",");
        pos_end = strLine.find(",", pos_start+1);
    }
    else
    {   
        // if not chipID input, address is between the second "," and the third ","
        pos_start = strLine.find(",");
        pos_start = strLine.find(",", pos_start+1);
        pos_end = strLine.find(",", pos_start+1);
    }
    
    //cout << "pos_start is " << strLine[pos_start] << endl;
    //cout << "pos_end is " << strLine[pos_end] << endl;
    
    // start from the next of ","
    pos_start++;
    
    // remove the space before the number
    while(isspace(strLine.at(pos_start))) {pos_start++;};     
   
    if(pos_start != string::npos && pos_end != string::npos && pos_start < pos_end) 
    {
        data = strLine.substr(pos_start, pos_end - pos_start);   
    } 
    else 
    {
        cout << "_getAddress::Not found expected character, invalid input!" << endl;
        exit(1);
    }
    
    //cout << "_getAddress test end" << endl;
    
    return data;
}

int _getChipID(const string strLine)
{    
    //cout << "_getChipID test enter" << endl; 
    string:: size_type pos_start;
    string:: size_type pos_end;
    
    // return value;   
    int data;
    
    pos_start = strLine.find(")");
    pos_end = strLine.find(",");
    
    // find ",", if not find, return -1, means no input chip ID
    // if "," is behind ")", return -1, means no input chip ID
    if(pos_end == string::npos || pos_end > pos_start)
    {
        //cout << "_getChipID test1" << endl;       
        return -1;
    }
    else
    {
        // if find ",", count the number of ",", if result is 2, return -1
        // means no input chip ID for read and write
        int count=0;
        //pos_start = strLine.find(")");
        
        while(pos_end != string::npos && pos_start != string::npos)
        {
            count++;
            pos_end = strLine.find(",", pos_end+1);
            if(pos_end > pos_start) {break;}
        }
        
        if(count == 2)
        {
            //cout << "_getChipID test2" << endl; 
            return -1;
        }
        
    }
    
    // if get here, means input chip ID, then try to get it
    pos_start = strLine.find("(");
    pos_end = strLine.find(",");
   
    if(pos_start != strLine.npos && pos_end != strLine.npos && pos_start < pos_end) 
    {
        string temp = "";
        
        if(!isdigit(strLine.at(pos_start+1))) 
        {
            cout << "_getChipID::Not found expected number, invalid input!" << endl;
            exit(1);
        }
        
        // get the number part
        while(isdigit(strLine.at(++pos_start))) {temp+=strLine.at(pos_start);};
        data = atol(temp.c_str());
        
        if(strLine[pos_start] != ',') 
        {
           cout << "_getChipID::Not found expected ',', invalid input!" << endl;
           exit(1);
        }
        
    } 
    else
    {
         cout << "_getChipID::Invalid input!" << endl;
         exit(1);
    }
    
    //cout << "_getChipID test end" << endl;
     
    //cout << "chip ID=" << data << endl;
    return data;
}

void _checkValidInput(string strLine)
{
    string:: size_type pos_start;
    string:: size_type pos_end;
    
    pos_start = strLine.find("(");
    
    if(pos_start == strLine.npos)
    {
        cout << "_checkValidInput::Not found expected '(', invalid input!" << endl;
        exit(1); 
    }
    
    for(int i=0; i<pos_start; i++)
    {
        if(!isalpha(strLine[i]))
        {
            cout << "_checkValidInput::Invalid function name!" << endl;
            exit(1);
        }
    }
    
    pos_end = strLine.find(")");
    
    if(pos_end == strLine.npos)
    {
        cout << "_checkValidInput::Not found expected ')', invalid input!" << endl;
        exit(1); 
    }
    
    if(pos_start > pos_end)
    {
        cout << "_checkValidInput::')' is before '(', invalid input!" << endl;
        exit(1);
    }
}

unsigned long _caculateTotalBlocks(obj_size size)
{
    unsigned long total_block;
    
    if(size.unit < cache_line_size.unit) 
    {
        cout << "Cache line size is bigger than memory or cache size, invalid input!" << endl;
        exit(1);
    } 
    else 
    {
        int step = size.unit - cache_line_size.unit;
        total_block = (long)(size.data * pow(2,step*10)/cache_line_size.data);
    }
    
    return total_block;
}

unsigned long _caculateNeedBlocks(unsigned long address, obj_size size)
{
    unsigned long need_blocks;
    
    int modulo = address % cache_line_size.data;
    
    if(modulo != 0)
    {
        if(size.unit < cache_line_size.unit) 
        {
            need_blocks = 1;
        }
        else
        {
            int step = size.unit - cache_line_size.unit;
            unsigned long temp1 = address / cache_line_size.data;
            unsigned long temp2 = (long)((address + size.data * pow(2,step*10)) / cache_line_size.data);
            need_blocks = temp2 - temp1 + 1; 
        }                   
    }
    else 
    {
        if(size.unit < cache_line_size.unit) 
        {
            need_blocks = 1;
        } 
        else 
        {
            int step = size.unit - cache_line_size.unit;
            need_blocks = (long)(size.data * pow(2,step*10)/cache_line_size.data);
            modulo = (long)(size.data * pow(2,step*10))%cache_line_size.data;
            
            if(modulo != 0)
            {
                need_blocks++;
            }
        }
    }
    
    return need_blocks;
}

int _checkCacheL2(int chipID, unsigned long loadingPage, bool isAddTime)
{
    int r = -1;
    chip currentChip = array_chips[chipID];
    int tlbLength = currentChip.total_block_l2;
    
    for(int i=0; i<tlbLength; i++)
    {
        if(currentChip.tlb_l2[i].memory_page == loadingPage && currentChip.tlb_l2[i].valid == 1)
        {
            r = i;
            break;
        }
    }
    
    if(isAddTime)
    {
        string opt="";
        
        if(r != -1)
        {
            opt = "L2hit";    
        }
        else
        {
            opt = "L2miss";
        }
        
        _addResultTime(opt, currentChip.cache_access_speed_l2);
    }
        
    return r;
}

int _checkCacheL3(int chipID, unsigned long loadingPage, bool isAddTime)
{
    int r = -1;
    
    for(int i=0; i<total_block_l3; i++)
    {
        if(tlb_l3[i].memory_page == loadingPage)
        {
            r = i;
            break;
        }
    }
    
    if(isAddTime)
    {
        string opt="";
        
        if(r != -1)
        {
            opt = "L3hit";    
        }
        else
        {
            opt = "L3miss";
        }
        
        _addResultTime(opt, cache_access_speed_l3);
    }
        
    return r;
}

void _addResultTime(string opt, obj_time time)
{
    for(int i=0; i<result_index; i++)
    {
        if(result_time[i].opt == opt)
        {
            result_time[i].count++;
            return;
        }
    }
    
    result_time[result_index].opt = opt;        
    result_time[result_index].time = time;
    result_time[result_index].count = 1;
    result_index++;
}

int _findAvailableBlockInCacheL2(int chipID)
{
    int availableBlock = -1;
    chip currentChip = array_chips[chipID];
    
    for(int i=0; i<currentChip.total_block_l2; i++)
    {
        if(currentChip.array_usage_l2[i] == -1)
        {
            availableBlock = i;
            break;
        }
    }
    
    return availableBlock;
}

int _findAvailableBlockInCacheL3()
{
    int availableBlock = -1;
    
    for(int i=0; i<total_block_l3; i++)
    {
        if(array_usage_l3[i] == -1)
        {
            availableBlock = i;
            break;
        }
    }
    
    return availableBlock;
}

entry _takeTheFirstOutByLRU(int chipID)
{ 
    chip currentChip = array_chips[chipID];
    entry oldEntry = currentChip.tlb_l2[0];
    
    for(int i=0; i<currentChip.total_block_l2-1; i++)
    {
        currentChip.tlb_l2[i] = currentChip.tlb_l2[i+1];       
    }
    
    return oldEntry;
}

entry _takeTheFirstOutL3ByLRU()
{ 
    entry oldEntry = tlb_l3[0];
    
    for(int i=0; i<total_block_l3-1; i++)
    {
        tlb_l3[i] = tlb_l3[i+1];       
    }
    
    return oldEntry;
}

void _swapTLBByLRU(int chipID, int target, int source)
{
    chip currentChip = array_chips[chipID];
    
    entry temp = currentChip.tlb_l2[target];
    
    for(int i=target; i<source; i++)
    {
        currentChip.tlb_l2[i] = currentChip.tlb_l2[i+1];
    }
    
    currentChip.tlb_l2[source] = temp;
    
    //// print TLB
//    for(int i=0; i<currentChip.total_block_l2; i++)
//    {
//        cout << "TLB loadingPage is " << currentChip.tlb_l2[i].memory_page
//        << ", block ID is " << currentChip.tlb_l2[i].block_id << endl;    
//    }
    
}

void _swapTLBL3ByLRU(int target, int source)
{
    entry temp = tlb_l3[target];
    
    for(int i=target; i<source; i++)
    {
        tlb_l3[i] = tlb_l3[i+1];
    }
    
    tlb_l3[source] = temp;
}

int _getTLBL2UsedBlocks(int chipID)
{
    chip currentChip = array_chips[chipID];
    int number = 0;
    
    for(int i=0; i< currentChip.total_block_l2; i++)
    {
        if(currentChip.tlb_l2[i].block_id != -1)
        {
            number++;
        }
        else
        {
            break;
        }
    }
    
    return number;
}

int _getTLBL3UsedBlocks()
{
    int number = 0;
    
    for(int i=0; i< total_block_l3; i++)
    {
        if(tlb_l3[i].block_id != -1)
        {
            number++;
        }
        else
        {
            break;
        }
    }
    
    return number;
}

void _printResult(int chipID, int pages[], int pageSize)
{
    chip currentChip = array_chips[chipID];
    
    //// print TLB
//    for(int i=0; i<currentChip.total_block_l2; i++)
//    {
//        cout << "TLB loadingPage is " << currentChip.tlb_l2[i].memory_page
//        << ", block ID is " << currentChip.tlb_l2[i].block_id << endl;    
//    }
    
    //// print L2 usage
//    for(int i=0; i<currentChip.total_block_l2; i++)
//    {
//        cout << "L2 usage is " << currentChip.array_usage_l2[i] << endl;
//    }
    
//    // print L2 usage
//    for(int i=0; i<total_block_l3; i++)
//    {
//        cout << "L3 usage is " << array_usage_l3[i] << endl;
//    }

    // print block id and state
    char temp[10];
    obj_time totalTime={0,ns};
     
    cout << "L2idx=" << l2ID.substr(0, l2ID.length()-1);
    
    if(number_of_chips > 1)
    {
        cout << " L3idx=" << l3ID.substr(0, l3ID.length()-1);
    }
    
    // print time
    cout << " time(";
    
    for(int i=0; i<result_index; i++)
    {
        totalTime.data += result_time[i].time.data * result_time[i].count;
        totalTime.unit = result_time[i].time.unit;
        
        cout << result_time[i].opt << "=" << result_time[i].time.data 
        << UnitTimeNames[result_time[i].time.unit];
        
        if(result_time[i].count > 1) cout << "*" << result_time[i].count;
        cout << ", ";
    }
    
    cout << "total=" << totalTime.data << UnitTimeNames[totalTime.unit] << ")";
    
    result_index = 0;
    
    if(number_of_chips > 1)
    {
        cout << " L2state=" << l2State.substr(0, l2State.length()-1) 
             << " L3state=" << l3State.substr(0, l3State.length()-1) << endl;
    }
    else
    {
        cout << " state=" << l2State.substr(0, l2State.length()-1) << endl;
    }
    
    cout << endl;
    
    l2ID = "";
    l2State = "";
    l3ID = "";
    l3State = "";
}

void _readFromCacheL2(int chipID, int coreID, int tlbIndex)
{
    chip currentChip = array_chips[chipID];
    
    int blockID = currentChip.tlb_l2[tlbIndex].block_id;
              
    if(currentChip.array_usage_l2[blockID] != coreID)
    {
      if(currentChip.tlb_l2[tlbIndex].state == 'M')
      {
          _addResultTime("L2writeback", memory_access_speed);
      }
      
      currentChip.array_usage_l2[blockID] = coreID;                                    
      currentChip.tlb_l2[tlbIndex].state = 'S';                                       
    }
    else
    {
      if(currentChip.tlb_l2[tlbIndex].state == 'M')
      {
          currentChip.tlb_l2[tlbIndex].state = 'E';
          //_addResultTime("L2writeback", memory_access_speed);
      }
    }
    
    l2ID += _num2str(blockID);
    l2ID += "&";
    l2State += currentChip.tlb_l2[tlbIndex].state;
    l2State += "&";
    
    _addResultTime("L2read", currentChip.cache_access_speed_l2);
    
    int length = _getTLBL2UsedBlocks(chipID);
     
    // swap it with the last one, preparing for LRU
    _swapTLBByLRU(chipID, tlbIndex, length - 1);   
}

void _readFromCacheL3(int chipID, int coreID, int tblIndex)
{
    int blockID = tlb_l3[tblIndex].block_id;
                   
    // check if it is shared, if yes, call broadcast
    if(array_usage_l3[blockID] != chipID * 10 + coreID)
    {
      array_usage_l3[blockID] = chipID * 10 + coreID;                                    
      tlb_l3[tblIndex].state = 'S';                                       
    }
    
    l3ID += _num2str(blockID);
    l3ID += "&";
    l3State += tlb_l3[tblIndex].state;
    l3State += "&";
       
    _addResultTime("L3read", cache_access_speed_l3);
    
    int length = _getTLBL3UsedBlocks();
    
    // swap it with the last one, preparing for LRU
    _swapTLBL3ByLRU(tblIndex, length - 1); 
}

void _loadMemToCacheL2(int chipID, int coreID, int loadingPage)
{
    chip currentChip = array_chips[chipID];
    
    int availableBlock = _findAvailableBlockInCacheL2(chipID);
                   
    //cout << "availableBlock in L2 is " << availableBlock << endl;
    
    // not -1, means there is empty block, use it
    // or it is full, call take out
    if(availableBlock != -1)
    {
       // mark it as used               
       currentChip.array_usage_l2[availableBlock] = coreID;
       
       // add into TLB        
       currentChip.tlb_l2[availableBlock].block_id = availableBlock;
       currentChip.tlb_l2[availableBlock].memory_page = loadingPage;
       currentChip.tlb_l2[availableBlock].state = 'E';
       currentChip.tlb_l2[availableBlock].valid = 1;
       
       l2ID += _num2str(availableBlock);
       l2ID += "&";
       l2State += currentChip.tlb_l2[availableBlock].state;
       l2State += "&";
       
       // add memory loading time
       _addResultTime("mem_read", memory_access_speed);
       
       // add L2 read time
       _addResultTime("L2read", currentChip.cache_access_speed_l2);
       
    }
    else
    {
       // take the first one out by LRU
       entry oldEntry = _takeTheFirstOutByLRU(chipID);
       int indexOfLastOne = currentChip.total_block_l2-1;
       
       // mark it as used
       currentChip.array_usage_l2[oldEntry.block_id] = coreID;
       
       // add into TLB
       currentChip.tlb_l2[indexOfLastOne].block_id = oldEntry.block_id;
       currentChip.tlb_l2[indexOfLastOne].memory_page = loadingPage;
       currentChip.tlb_l2[indexOfLastOne].state = 'E';
       currentChip.tlb_l2[indexOfLastOne].valid = 1;
       
       l2ID += _num2str(oldEntry.block_id);
       l2ID += "&";
       l2State += currentChip.tlb_l2[indexOfLastOne].state;
       l2State += "&";
       
       // add times
       _addResultTime("replace", replacement_speed);
       
       if(oldEntry.state == 'M')
       {
           _addResultTime("L2writeback", memory_access_speed);
       }
       
       _addResultTime("mem_read", memory_access_speed);
        
       _addResultTime("L2read", currentChip.cache_access_speed_l2);
    }
}

void _writeToCacheL3(int chipID, int coreID, int loadingPage, bool isRead)
{
    int availableBlock = _findAvailableBlockInCacheL3();
    
    // not -1, means there is empty block, use it
    // or it is full, call take out               
    if(availableBlock != -1)
    {
       // mark it as used
       array_usage_l3[availableBlock] = chipID * 10 + coreID;
       
       // add into TLB                           
       tlb_l3[availableBlock].block_id = availableBlock;
       tlb_l3[availableBlock].memory_page = loadingPage;
       
       // set state
       if(isRead)
       {
           tlb_l3[availableBlock].state = 'E';
       }
       else
       {
           tlb_l3[availableBlock].state = 'M';
       }
       
       tlb_l3[availableBlock].valid = 1;
       
       l3ID += _num2str(availableBlock);
       l3ID += "&";
       l3State += tlb_l3[availableBlock].state;
       l3State += "&";
       
       // add write time
       _addResultTime("L3write", cache_access_speed_l3);   
    }
    else
    {
      // take the first one out by LRU
       entry oldEntry = _takeTheFirstOutL3ByLRU();
       int indexOfLastOne = total_block_l3-1;
       
       // mark it as used
       array_usage_l3[availableBlock] = chipID * 10 + coreID;
       
       // add into TLB
       tlb_l3[indexOfLastOne].block_id = oldEntry.block_id;
       tlb_l3[indexOfLastOne].memory_page = loadingPage;
       
       if(isRead)
       {
           tlb_l3[indexOfLastOne].state = 'E';
       }
       else
       {
           tlb_l3[indexOfLastOne].state = 'M';
       }
       
       tlb_l3[indexOfLastOne].valid = 1;
       
       l3ID += _num2str(oldEntry.block_id);
       l3ID += "&";
       l3State += tlb_l3[indexOfLastOne].state;
       l3State += "&";
       
       _addResultTime("replace", replacement_speed);
       
       if(oldEntry.state == 'M')
       {
           _addResultTime("L3writeback", memory_access_speed);
       }
       
       if(oldEntry.state == 'M' || oldEntry.state == 'S')
       {
           array_usage_l3[oldEntry.block_id] = chipID * 10 + coreID;
           _addResultTime("broadcast", broadcast_speed);
       } 
    }
}

void _writeToCacheL2(int chipID, int coreID, int loadingPage)
{
    chip currentChip = array_chips[chipID];
    
    int availableBlock = _findAvailableBlockInCacheL2(chipID);
           
    //cout << "availableBlock is " << availableBlock << endl;
    
    // not -1, means there is empty block, use it
    // or it is full, call take out 
    if(availableBlock != -1)
    {
       // mark it as used
       currentChip.array_usage_l2[availableBlock] = coreID;
       
       // add into TLB
       currentChip.tlb_l2[availableBlock].block_id = availableBlock;
       currentChip.tlb_l2[availableBlock].memory_page = loadingPage;
       currentChip.tlb_l2[availableBlock].state = 'M';
       currentChip.tlb_l2[availableBlock].valid = 1;
       
       l2ID += _num2str(availableBlock);
       l2ID += "&";
       l2State += currentChip.tlb_l2[availableBlock].state;
       l2State += "&";
       
       // add write time
       _addResultTime("L2write", currentChip.cache_access_speed_l2);
    }
    else
    {
       // take the first one out by LRU
       entry oldEntry = _takeTheFirstOutByLRU(chipID);
       int indexOfLastOne = currentChip.total_block_l2-1;
       
       // add inot TLB
       currentChip.tlb_l2[indexOfLastOne].block_id = oldEntry.block_id;
       currentChip.tlb_l2[indexOfLastOne].memory_page = loadingPage;
       currentChip.tlb_l2[indexOfLastOne].state = 'M';
       currentChip.tlb_l2[indexOfLastOne].valid = 1;
       
       l2ID += _num2str(oldEntry.block_id);
       l2ID += "&";
       l2State += currentChip.tlb_l2[indexOfLastOne].state;
       l2State += "&";
       
       // check if it is shared, if yes, call broadcast
       if(oldEntry.state == 'S')
       {      
           _addResultTime("broadcast", broadcast_speed);
       }
       
       // mark as used by me
       currentChip.array_usage_l2[oldEntry.block_id] = coreID;
       
       _addResultTime("replace", replacement_speed);
            
       _addResultTime("L2write", currentChip.cache_access_speed_l2);
    }
}

void _rewriteToCacheL2(int chipID, int coreID, int tlbIndex)
{
    chip currentChip = array_chips[chipID];
    int blockID = currentChip.tlb_l2[tlbIndex].block_id;
           
    // check if it is shared, if yes, call broadcast
    if(currentChip.tlb_l2[tlbIndex].state == 'S')
    {                                          
      _addResultTime("broadcast", broadcast_speed);
    }
    
    // mark as used by me
    currentChip.array_usage_l2[blockID] = coreID;
    currentChip.tlb_l2[tlbIndex].state = 'M';
    
    l2ID += _num2str(blockID);
    l2ID += "&";
    l2State += currentChip.tlb_l2[tlbIndex].state;
    l2State += "&";
    
    _addResultTime("L2write", currentChip.cache_access_speed_l2);
    
    int length = _getTLBL2UsedBlocks(chipID);
     
    // swap it with the last one, preparing for LRU
    _swapTLBByLRU(chipID, tlbIndex, length - 1);
}

void _rewriteToCacheL3(int chipID, int coreID, int tlbIndex, int loadingPage)
{
    int blockID = tlb_l3[tlbIndex].block_id;
           
    // check if it is shared, if yes, call broadcast
    if(tlb_l3[tlbIndex].state == 'M' || tlb_l3[tlbIndex].state == 'S')
    {
      // mark other L2 as invalid
      int otherChipID = array_usage_l3[blockID]/10;
      int otherChipTLBIndex = _checkCacheL2(otherChipID, loadingPage, 0);
      
      array_chips[otherChipID].tlb_l2[otherChipTLBIndex].valid = 0;
      array_chips[otherChipID].tlb_l2[otherChipTLBIndex].state = 'I';
                              
      // mark as used by me
      array_usage_l3[blockID] = chipID * 10 + coreID;                                          
      _addResultTime("broadcast", broadcast_speed);
    }
    
    tlb_l3[tlbIndex].state = 'M';
    
    l3ID += _num2str(blockID);
    l3ID += "&";
    l3State += tlb_l3[tlbIndex].state;
    l3State += "&";
       
    _addResultTime("L3write", cache_access_speed_l3);
    
    int length = _getTLBL3UsedBlocks();
     
    // swap it with the last one, preparing for LRU
    _swapTLBL3ByLRU(tlbIndex, length - 1);
}

void _checkCommandsOrder(int index)
{
    if((commands[0] && commands[1] && commands[2]) || (commands[0] && commands[2]))
    {
       commands[index] = 1;
    }
    else
    {
       cout << "Not input memory size, it must be the first command!" << endl;
       exit(1); 
    }
}

void _checkCommandsReady()
{
     for(int i=2; i<9; i++)
     {
         if(!commands[i])
         {
             cout << "Missing command:" << funcNames[i] << " for reading and writing, please input again!" << endl;
             exit(1);
         }
     }
}

void _checkValidIDs(int chipID, int coreID)
{     
     if(chipID >= number_of_chips || chipID < -1)
     {
         cout << "Invalid chip ID, please input again!" << endl;
         exit(1);
     }
     
     if(coreID >= array_chips[chipID].number_of_core || coreID < 0)
     {
         cout << "Invalid core ID, please input again!" << endl;
         exit(1);
     }
}

string _num2str(int number)
{
    stringstream ss;
    ss << number;
    return ss.str();
}

