#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string.h>
#include <math.h>
#include <queue>
#include <limits.h>
using namespace std;

unsigned long memAccessTime = 0;

struct page{
  int pgNum; //Unique name for each page
  int validBit; //1 in mem, 0 not in mem
  unsigned long accessTime; //for LRU
  int R; //for Clock
};

int main (int argc, char **argv){
	int pgNameCount = 0; //keeps track of the pg name for uniqueness
	int pageAlloc;
	ifstream plistRead; //read plist
	ifstream ptraceRead; //read ptrace
	plistRead.open("plist.txt");
	ptraceRead.open("ptrace.txt");
	int pgSize = atoi(argv[3]);
	string whichAlgo = argv[4];
	int totalProc = 0;
	std::vector<struct page *> pgTbls; //a vector that holds page arrays(page tables)
	int count = 0;
	std::vector<int> pgTblSizes; //keeps track of the size of each page table for each process
	string prePaging = argv[5]; //+ prepaging on, - demand paging on



	//initializes all the pages and the page tables
	for(int n; plistRead >> n;){
		if(count % 2 == 1){
			struct page* temp = (page*)malloc((int)ceil(n * 1.0 / pgSize) * sizeof(struct page)); //mallocs space for the pages for a process
			for(int i = 0; i < (int)ceil(n * 1.0 / pgSize); i++){
				temp[i].pgNum = pgNameCount;
				temp[i].validBit = 0;
				temp[i].accessTime = 0;
				pgNameCount++;
			}
			pgTbls.push_back(temp); //pushes the page table into the pgTbls vector
			pgTblSizes.push_back((int)ceil(n * 1.0 / pgSize)); //updates the size of the page table
		}
		count++;
	}
	count = 0;



	//default loading
	queue<struct page *> fifo[pgTbls.size()];
	int memoryAlloc = 512 / pgTbls.size();
	pageAlloc = memoryAlloc / pgSize; //how many pages each process gets to load into physical mem by default
	for(int i = 0; i < pgTbls.size(); i++){
		for(int j = 0; j < pageAlloc; j++){
			if (j == pgTblSizes.at(i)){ //if default loading is larger than num of pages break and continue onto next process
				break;
			}
			pgTbls.at(i)[j].validBit = 1; //page in mem
			pgTbls.at(i)[j].accessTime = memAccessTime;
			pgTbls.at(i)[j].R = 0; //set R = 0 at first
			memAccessTime++;
			fifo[i].push(&(pgTbls.at(i)[j])); //push the pointer to the page into a fifo queue for use later
		}
	}









	int faultCount = 0;
	int whichProgram = 0;
	count = 0;
	if(whichAlgo.compare("FIFO") == 0){ //start of FIFO
		for(int n; ptraceRead >> n;){ //reads from ftrace
			if(count % 2 == 0){ //gets the process number
				whichProgram = n;
			}
			else{
				int memoryLoc = (int)ceil(n * 1.0 / pgSize) - 1; //transfers mem location to page location
				if(pgTbls.at(whichProgram)[memoryLoc].validBit == 0){ //if not loaded into mem page fault
					faultCount++;
					//prepaging start here
					if(prePaging.compare("+") == 0){
						page *remove1 = fifo[whichProgram].front(); //first part same as demand paging
						remove1 -> validBit = 0;
						fifo[whichProgram].pop();
						pgTbls.at(whichProgram)[memoryLoc].validBit = 1;
						fifo[whichProgram].push(&(pgTbls.at(whichProgram)[memoryLoc]));						
						//start of second removal
						page *remove2 = fifo[whichProgram].front();
						int name = remove1 -> pgNum; //get the name of the page that was just removed
						int i;
						if(memoryLoc == pgTblSizes.at(whichProgram) - 1){ //if the first removed was the last page start at the beginning
							i = 0;
						}
						else{ //else start at the next mem location
							i = memoryLoc + 1;
						}
						while(true){
							if(memoryLoc == i){ //if it looped around there is no page not in mem. Do nothing
								break;
							}
							else if(i == pgTblSizes.at(whichProgram)){ //cant bring back pg that was just removed
								i = 0;
							}
							else if(pgTbls.at(whichProgram)[i].validBit == 0 && name != pgTbls.at(whichProgram)[i].pgNum){ //if the page is not in mem and it wasnt the page that was just removed
								remove2 -> validBit = 0; //remove next page from fifo
								fifo[whichProgram].pop();
								pgTbls.at(whichProgram)[i].validBit = 1; //load new page into mem and push to fifo
								fifo[whichProgram].push(&(pgTbls.at(whichProgram)[i]));
								break;
							}
							else{
								i++;
							}
						}
					}
					else{
						page *remove = fifo[whichProgram].front(); //if page fault remove the first thing from fifo from memory
						remove -> validBit = 0;
						fifo[whichProgram].pop();
						pgTbls.at(whichProgram)[memoryLoc].validBit = 1; //add the new page into memory and add it to the fifo queue
						fifo[whichProgram].push(&(pgTbls.at(whichProgram)[memoryLoc]));
					}
				}	
			}
			count++; //increment to get next number from ptrace
		}
	}
	else if(whichAlgo.compare("LRU") == 0){ //start of LRU
		for(int n; ptraceRead >> n;){			
			if(count % 2 == 0){
				whichProgram = n;
			}
			else{ 
				int memoryLoc = (int)ceil(n * 1.0 / pgSize) - 1;				
				if(pgTbls.at(whichProgram)[memoryLoc].validBit == 0){ //if not in mem load it in
					faultCount++;
					if(prePaging.compare("+") == 0){//braces off in here
						//first removal
						struct page *LRU1 = NULL; //first removal same as demand paging
						unsigned long minTime = ULONG_MAX;
						for(int i = 0; i < pgTblSizes.at(whichProgram); i++){
							if(pgTbls.at(whichProgram)[i].validBit == 1 && pgTbls.at(whichProgram)[i].accessTime < minTime){
									LRU1 = &(pgTbls.at(whichProgram)[i]);
									minTime = pgTbls.at(whichProgram)[i].accessTime;
							}						
						}					
						LRU1 -> validBit = 0;
						pgTbls.at(whichProgram)[memoryLoc].validBit = 1;
						pgTbls.at(whichProgram)[memoryLoc].accessTime = memAccessTime;
						memAccessTime++;
						//start of second removal
						struct page *LRU2 = NULL;
						minTime = ULONG_MAX;
						int name = LRU1 -> pgNum; //gets the name of the page that was just removed
						int i;
						if(memoryLoc == pgTblSizes.at(whichProgram) - 1){ //if the first removed was the last page start at the beginning
							i = 0;
						}
						else{
							i = memoryLoc + 1; //else start at the next mem location
						}
						while(true){
							if(memoryLoc == i){ //if looped all around then all pages are in mem
								break;
							}
							else if(i == pgTblSizes.at(whichProgram)){ //cant bring back pg that was just removed
								i = 0;
							}
							else if(pgTbls.at(whichProgram)[i].validBit == 0 && name != pgTbls.at(whichProgram)[i].pgNum){ //if page is not in mem and its not the one that was just removed
								for(int j = 0; j < pgTblSizes.at(whichProgram); j++){ //performs LRU again
									if(pgTbls.at(whichProgram)[j].validBit == 1 && pgTbls.at(whichProgram)[j].accessTime < minTime && name != pgTbls.at(whichProgram)[j].pgNum){
										LRU2 = &(pgTbls.at(whichProgram)[j]);
										minTime = pgTbls.at(whichProgram)[j].accessTime;
									}						
								}					
								LRU2 -> validBit = 0;
								pgTbls.at(whichProgram)[i].validBit = 1;
								pgTbls.at(whichProgram)[i].accessTime = memAccessTime;
								break;
							}
							else{
								i++;
							}
						}
					}
					else{
						struct page *LRU = NULL; //storage for page that has oldest access time
						unsigned long minTime = ULONG_MAX; //keeps track of lowest access time
						for(int i = 0; i < pgTblSizes.at(whichProgram); i++){ //loops through all pages in the processes pg table
							if(pgTbls.at(whichProgram)[i].validBit == 1 && pgTbls.at(whichProgram)[i].accessTime < minTime){ //if page is in mem and it has a older access time than the current one
								LRU = &(pgTbls.at(whichProgram)[i]); //change the new oldest page
								minTime = pgTbls.at(whichProgram)[i].accessTime;
							}						
						}					
						LRU -> validBit = 0; //remove oldest page from mem
						pgTbls.at(whichProgram)[memoryLoc].validBit = 1; //load new page into mem
						pgTbls.at(whichProgram)[memoryLoc].accessTime = memAccessTime;
					}
				}
				else{ //if already in mem
					pgTbls.at(whichProgram)[memoryLoc].accessTime = memAccessTime; //if page is already in mem just update its access time
				}
				memAccessTime++; //incremets access time after every page access
			}
			count++;
		}
	}
	else if(whichAlgo.compare("Clock") == 0){ //start if Clock
		for(int n; ptraceRead >> n;){
			if(count % 2 == 0){
				whichProgram = n;
			}
			else{
				int memoryLoc = (int)ceil(n * 1.0 / pgSize) - 1;
				if (pgTbls.at(whichProgram)[memoryLoc].validBit == 0){
					faultCount++;
					if(prePaging.compare("+") == 0){ //start if prepaging
						bool found = false; //same as demand paging to remove first page
						struct page *hand1 = NULL;
						while(!found){						
							hand1 = fifo[whichProgram].front(); 
							if(hand1-> R == 1){
								hand1 -> R = 0;
								fifo[whichProgram].push(hand1);
								fifo[whichProgram].pop();																				
							}
							else if(hand1 -> R == 0){
								found = true;
								hand1 -> validBit = 0;
								fifo[whichProgram].pop();
								pgTbls.at(whichProgram)[memoryLoc].R = 1;
								pgTbls.at(whichProgram)[memoryLoc].validBit = 1; //when load in new give it a second chance
								fifo[whichProgram].push(&(pgTbls.at(whichProgram)[memoryLoc]));
							}
						} //end of first removal						
						struct page *hand2 = NULL;
						int name = hand1 -> pgNum; //get name of first page that was removec
						int i;
						if(memoryLoc == pgTblSizes.at(whichProgram) - 1){ //if page removed was at the end of the page table start at the beginning
							i = 0;
						}
						else{ //if not start at the next mem location
							i = memoryLoc + 1;
						}
						while(true){
							if(memoryLoc == i){ //if checked all pages then all pages are in mem. Do nothing
								break;
							}
							else if(i == pgTblSizes.at(whichProgram)){ //cant bring back pg that was just removed
								i = 0;
							}
							else if(pgTbls.at(whichProgram)[i].validBit == 0 && name != pgTbls.at(whichProgram)[i].pgNum){ //if page is not in mem and it wasnt the page that was just removed
								found = false; //run Clock algo again (just like we did for the first page)
								while(!found){						
									hand2 = fifo[whichProgram].front();
									if(hand2-> R == 1){
										hand2 -> R = 0;
										fifo[whichProgram].push(hand2);
										fifo[whichProgram].pop();																				
									}
									else if(hand2 -> R == 0){
										found = true;
										hand2 -> validBit = 0;
										fifo[whichProgram].pop();
										pgTbls.at(whichProgram)[i].R = 1;
										pgTbls.at(whichProgram)[i].validBit = 1; //when load in new give it a second chance
										fifo[whichProgram].push(&(pgTbls.at(whichProgram)[i]));
									}
								}
								//end		
								break;
							}
							else{
								i++;
							}
						}
					} //end of prepaging
					else{
						bool found = false; //flag to tell if a page was replaced
						struct page *hand = NULL;
						while(!found){						
							hand = fifo[whichProgram].front(); //get the page the hand is pointing to
							if(hand -> R == 1){ //if it has a second chance
								hand -> R = 0; //change its use bit to 0
								fifo[whichProgram].push(hand); //advance the hand
								fifo[whichProgram].pop();																				
							}
							else if(hand -> R == 0){ //if the page doesnt have a second chance
								found = true; //tell the program that a page has been found to be evicted
								hand -> validBit = 0; //remove the page from mem
								fifo[whichProgram].pop();
								pgTbls.at(whichProgram)[memoryLoc].R = 1; //when load in new give it a second chance
								pgTbls.at(whichProgram)[memoryLoc].validBit = 1; //load page into mem
								fifo[whichProgram].push(&(pgTbls.at(whichProgram)[memoryLoc])); //push it to fifo to replace old page
							}
						}
					}
				}
				else{ //if already in mem give it a second chance
					pgTbls.at(whichProgram)[memoryLoc].R = 1;
				}
			}
			count++;			
		}
	}
	printf("%i\n", faultCount);
}