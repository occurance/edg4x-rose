/* -----------------------------------------------------------
 * tps : 6th March 2009: RTED
 * RuntimeSystem called by each transformed source file
 * April : Changed this from C++ to C
 * -----------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>


#include "RuntimeSystem.h"
#include "rted_qt/rted_qt.h"


void debugOutput(const char* format, ...)
{
    va_list ap;
    va_start(ap,format);
#ifdef ROSE_WITH_ROSEQT
    vprintfGui(format,ap);
#else
    vprintf(format,ap);
#endif
}
// USE GUI for debugging
void Rted_debugDialog(const char* filename, int line) {
#ifdef ROSE_WITH_ROSEQT
  showDebugDialog(rtsi()->runtimeVariablesOnStack, rtsi()->runtimeVariablesOnStackEndIndex,
		  rtsi()->runtimeVariables, rtsi()->runtimeVariablesEndIndex,
		  rtsi()->runtimeMemory, rtsi()->runtimeMemoryEndIndex,
		  (char*)filename, line);
#endif
}


/*********************************************************
 * Declare a sole instance of the Runtime System using
 * C Syntax
 ********************************************************/
struct RuntimeSystem* rtsi() {
  static struct RuntimeSystem* rt_pi=0;
  if ( rt_pi == 0)  {
    debugOutput("Creating Instance for RUNTIMESYSTEM ------------------------------------\n");
    //pinstance = new RuntimeSystem(); // create sole instance
    rt_pi = (struct RuntimeSystem*) malloc(sizeof(struct RuntimeSystem)+1);
    if (rt_pi==NULL) {
      debugOutput("INIT FAILED: rt_pi==NULL\n");
      exit(1);
    }
    RuntimeSystem_Const_RuntimeSystem();
  }
  return rt_pi; // address of sole instance
}


/*********************************************************
 * Initialization of all runtime variables and the
 * output file. Result.txt will contain all results for
 * all files
 ********************************************************/
void
RuntimeSystem_Const_RuntimeSystem() {
  debugOutput(">>>>>>>>>>>>>>>>>>>>>>>>> CONSTRUCTOR \n ................\n\n" );

  //... perform necessary instance initializations
  // initialze arrays with 10 elements and later increase by 50 if too small
  int initElementSize=0;
  rtsi()->maxRuntimeVariablesOnStackEndIndex = rtsi()->maxMemoryEndIndex =  initElementSize;
  rtsi()->maxRuntimeVariablesEndIndex =  rtsi()->runtimeMemoryEndIndex = initElementSize;
  rtsi()->runtimeVariablesEndIndex=  rtsi()->runtimeVariablesOnStackEndIndex=0;
#if 0
  // fixme: the following is all zero, comment out
  rtsi()->runtimeVariablesOnStack = (struct RuntimeVariablesType*)malloc(sizeof(struct RuntimeVariablesType)*initElementSize);
  rtsi()->runtimeVariables = (struct RuntimeVariablesType*)malloc(sizeof(struct RuntimeVariablesType)*initElementSize);
  rtsi()->runtimeMemory = (struct MemoryType*)malloc(sizeof(struct MemoryType)*initElementSize);
#endif
  rtsi()->runtimeVariablesOnStack = NULL;
  rtsi()->runtimeVariables= NULL;
  rtsi()->runtimeMemory=NULL;

  rtsi()->myfile = fopen("result.txt","at");
  if (!rtsi()->myfile) {
    debugOutput("Rted Error ::: Cannot open output file!\n");
    exit(1);
  }
  rtsi()->violation=0; //false
  rtsi()->arrayDebug=1;//true;
  rtsi()->funccallDebug=1;//true;
}



// ***************************************** HELPER FUNCTIONS *************************************

/*********************************************************
 * What is the size in bytes of each type ?
 ********************************************************/
int getSizeOfSgType(const char* type) {
  int size =1;
  assert(type!=0);
  if (
      (strcmp(type,"SgTypeInt")==0) ||
      (strcmp(type,"SgPointerType")==0)
       )
    size = sizeof(int);
  else {
    debugOutput("Unknown Type : %s \n",type);
    exit(1);
  }

  return size;
}

/*********************************************************
 * resolve the meaning of a boolean value in plain text
 ********************************************************/
const char*
RuntimeSystem_resBool(int val) {
  if (val!=0)
    return (const char*)"true";
  return (const char*)"false";
}


/*********************************************************
 * return the tail of a string separated by underscore
 ********************************************************/
char*
RuntimeSystem_findLastUnderscore(char* s) {
  char* pos=NULL;
  char* end=NULL;
  char *iter=NULL;
  for ( iter= s; *iter != '\0'; ++iter) {
    if (*iter=='_')
      pos = iter;
    end=iter;
  }
  char* result = NULL;
  if (end!=pos && pos!=NULL) {
    int size = (end-pos);
    result = (char*) malloc(size+1);
    strcpy(result,pos);
  }
  return result;
}

/*********************************************************
 * This function is called whenever a violation is found
 ********************************************************/
void
RuntimeSystem_callExit(const char* filename, const char* line, const char* reason, const char* stmtStr) {
  // rtsi()->violation Found ... dont execute it - exit normally
  debugOutput("rtsi()->Violation found: %s\n  Reason: %s   in file: %s at line %s\n",stmtStr,reason,filename,line);
  fprintf(rtsi()->myfile,"rtsi()->Violation found: %s\n  Reason: %s   in file: %s at line %s\n",stmtStr,reason,filename,line);
  //*myfile << "rtsi()->Violation found: " << stmtStr << endl << "  Reason: " << reason <<
  //  "    in file : " << filename << " at line: " << line << endl;
  exit(0);
}


/*********************************************************
 * Convert an integer to const char*
 ********************************************************/
const char*
RuntimeSystem_roseConvertIntToString(int t) {
  int size = sizeof(int);
  char* text = (char*)malloc(size+1);
  if (text)
    sprintf(text,"%d",t);
  debugOutput("String converted from int : %s ",text);
  return text;
}

/*********************************************************
 * This function is closed when RTED finishes (Destructor)
 ********************************************************/
void
RuntimeSystem_roseRtedClose() {
  if (rtsi()->myfile)
    //    myfile->close();
    fclose(rtsi()->myfile);
  if (rtsi()->violation==0)  {
    debugOutput("RtedClose:: No rtsi()->violation found!! \n");
    exit(1);
  } else
    debugOutput("RtedClose:: rtsi()->Violation found. Good! \n" );
}

// ***************************************** HELPER FUNCTIONS *************************************








// ***************************************** ARRAY FUNCTIONS *************************************

/*********************************************************
 * Look at the variable at pos and check if it has
 * already a memory location assigned to it, if yes
 * then this might be a memory leak (if no other var
 * is assigned to the old address)
 ********************************************************/
int checkMemoryLeakIssues(int pos, int address,
			  const char* filename, const char* line, const char* stmtStr) {
  int problem =0;
  debugOutput("Checking for Memory Leak ... \n");
  // we assume that memory at pos exists
  struct RuntimeVariablesType* runtimevar = &(rtsi()->runtimeVariables[pos]);
  // get the variables assigned to the old address
  struct MemoryType* mem = runtimevar->address;
  assert(mem);
  // make sure this is not an access or assignment
  // within an element of the memory
  long int addr = mem->address;
  int size = mem->size;
  if (address>=addr && address <=(addr+size)) {
    // the new address is within the old address : ok.
    return problem;
  } // else continue

  struct MemoryVariableType* variables = mem->variables;
  if (variables) {
    // variables exist. Check each of them.
    // Actually, if there is more than one we are fine
    debugOutput("Checking for Memory Leak ... Memory has %d variable entries. \n",(mem->lastVariablePos));
    if (mem->lastVariablePos>1) {
      // ok.
    } else {
      // only one element at (0)
      // is it our current element, if yes, we have a memory leak
      struct RuntimeVariablesType* variable = mem->variables[0].variable;
      if (variable == runtimevar) {
	// problem
	RuntimeSystem_callExit(filename, line, (const char*)"Memory Leak: Assigning Pointer to Memory that has not been freed and no other pointer to that memory exists.", stmtStr);
      } else {
	debugOutput("The entry is different from the current var :  %s and %s\n",
	       variable->name, runtimevar->name);
      }
    }

  }

  return problem;
}

/*********************************************************
 * Increase the size of the array by multiples of 50
 * array stored all array variables that are used
 ********************************************************/
void
RuntimeSystem_increaseSizeMemory() {
  rtsi()->maxMemoryEndIndex+=50;
  struct MemoryType* arrays2D_tmp = (struct MemoryType*)malloc(sizeof(struct  MemoryType)*(rtsi()->maxMemoryEndIndex));
  if (arrays2D_tmp) {
    int i=0;
    for ( i=0;i<rtsi()->runtimeMemoryEndIndex;i++) {
      //printf(" %d rtsi()->arrays2D_tmp[i].name = %s\n",i,rtsi()->arrays[i].name);
      arrays2D_tmp[i].address=rtsi()->runtimeMemory[i].address;
      arrays2D_tmp[i].lastVariablePos=rtsi()->runtimeMemory[i].lastVariablePos;
      arrays2D_tmp[i].maxNrOfVariables=rtsi()->runtimeMemory[i].maxNrOfVariables;
      arrays2D_tmp[i].size=rtsi()->runtimeMemory[i].size;
      arrays2D_tmp[i].variables =rtsi()->runtimeMemory[i].variables;
    }
    free (rtsi()->runtimeMemory);
    rtsi()->runtimeMemory=arrays2D_tmp;
  }
  debugOutput( " Increased Memory to %d  -- current index %d\n", rtsi()->maxMemoryEndIndex, rtsi()->runtimeMemoryEndIndex );
}

/*********************************************************
 * Increase the size of the array by multiples of 2
 ********************************************************/
void
RuntimeSystem_increaseSizeMemoryVariables(  int pos) {
  //struct MemoryType* runtimeMemory = rtsi()->runtimeMemory[pos];
  rtsi()->runtimeMemory[pos].maxNrOfVariables+=2;
  struct MemoryVariableType* arrays2D_tmp = (struct MemoryVariableType*)malloc(sizeof(struct  MemoryVariableType)*(rtsi()->runtimeMemory[pos].maxNrOfVariables));
  if (arrays2D_tmp) {
    int i=0;
    for ( i=0;i<rtsi()->runtimeMemory[pos].lastVariablePos;i++) {
      arrays2D_tmp[i].variable=rtsi()->runtimeMemory[pos].variables[i].variable;
    }
    free (rtsi()->runtimeMemory[pos].variables);
    rtsi()->runtimeMemory[pos].variables=arrays2D_tmp;
  }
  debugOutput( " Increased VariableMemory to %d  -- current index %d\n",rtsi()->runtimeMemory[pos].maxNrOfVariables , rtsi()->runtimeMemory[pos].lastVariablePos) ;
}

/*********************************************************
 * Remove a variable from a memory location
 * Each memory location can have several variables pointed to
 ********************************************************/
void RuntimeSystem_RemoveVariableFromMemory(long int address,
					    struct RuntimeVariablesType* var) {
  int i=0;
  for ( i=0;i<rtsi()->runtimeMemoryEndIndex;i++) {
    long int addr = rtsi()->runtimeMemory[i].address;
    if (addr==address) {
      debugOutput("Removing Variable from Address %lld .\n",address);
      int endIndex = rtsi()->runtimeMemory[i].lastVariablePos;
      int deletePos=-1;
      int j=0;
      for ( j=0;j<endIndex;j++) {
	struct RuntimeVariablesType* variable = rtsi()->runtimeMemory[i].variables[j].variable;
	if (variable==var) {
	  // found the variable in the pool
	  deletePos=j;
	  break;
	}
      }
      if (deletePos==-1) {
	debugOutput("Something went wrong deleting the variable.\n");
	exit(1);
      } else {
	if (endIndex==1) {
	  // delete only element
	  endIndex--;
	} if (deletePos==endIndex-1) {
	  // delete the last element
	  endIndex--;
	} else {
	  // delete an element inbetween
	  rtsi()->runtimeMemory[i].variables[deletePos].variable=
	    rtsi()->runtimeMemory[i].variables[endIndex-1].variable;
	  endIndex--;
	}
      }
    }
  }
}


/*********************************************************
 * Whenever a variable is created, make sure
 * that the memory is tracked (for heap)
 ********************************************************/
struct MemoryType*
RuntimeSystem_AllocateMemory(long int address, int sizeArray,
			     struct RuntimeVariablesType* var) {
  // if this address already exists, we only
  // add the variable to it
  int i=0;
  for ( i=0;i<rtsi()->runtimeMemoryEndIndex;i++) {
    long int addr = rtsi()->runtimeMemory[i].address;
    if (addr==address) {
      debugOutput("Address exists. Adding variable if not present.\n");
      int j=0;
      int endIndex = rtsi()->runtimeMemory[i].lastVariablePos;
      int foundVar=0;
      for ( j=0;j<endIndex;j++) {
	struct RuntimeVariablesType* variable = rtsi()->runtimeMemory[i].variables[j].variable;
	if (variable==var) {
	  foundVar=1;
	  break;
	}
      }
      if (foundVar==1) {
	// this variable is already present at this position
	// lets return the memoryType to this position
	return &(rtsi()->runtimeMemory[i]);
      } else {
	// we need to add this variable to the current address
	if (rtsi()->runtimeMemory[i].lastVariablePos>=rtsi()->runtimeMemory[i].maxNrOfVariables) {
	  RuntimeSystem_increaseSizeMemoryVariables(i);
	}
	rtsi()->runtimeMemory[i].size=sizeArray;
	rtsi()->runtimeMemory[i].variables[rtsi()->runtimeMemory[i].lastVariablePos].variable=var;
	rtsi()->runtimeMemory[i].lastVariablePos++;
	return &(rtsi()->runtimeMemory[i]);
      }
    }
  }
  // if this address does not exits. Create address and add Variable
  if (rtsi()->runtimeMemoryEndIndex>=rtsi()->maxMemoryEndIndex) {
    RuntimeSystem_increaseSizeMemory();
  }
  rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].address=address;
  // now we add this variable
  if (rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].lastVariablePos>=rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].maxNrOfVariables) {
    RuntimeSystem_increaseSizeMemoryVariables(rtsi()->runtimeMemoryEndIndex);
  }
  rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].size=sizeArray;
  rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].variables[rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].lastVariablePos].variable=var;
  rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex].lastVariablePos++;
  rtsi()->runtimeMemoryEndIndex++;

  return &(rtsi()->runtimeMemory[rtsi()->runtimeMemoryEndIndex-1]);
}

/*********************************************************
 * Increase the size of the stack by multiples of 50
 * stack stored all variables passed via function calls
 ********************************************************/
void
RuntimeSystem_increaseSizeRuntimeVariablesOnStack() {
  rtsi()->maxRuntimeVariablesOnStackEndIndex+=50;
  struct RuntimeVariablesType* run_tmp = (struct RuntimeVariablesType*)malloc(sizeof(struct RuntimeVariablesType)*(rtsi()->maxRuntimeVariablesOnStackEndIndex));
  if (run_tmp) {
    int i=0;
    for ( i=0;i<rtsi()->runtimeVariablesOnStackEndIndex;i++) {
      run_tmp[i].name=rtsi()->runtimeVariablesOnStack[i].name;
      run_tmp[i].mangled_name =rtsi()->runtimeVariablesOnStack[i].mangled_name;
    }
    free( rtsi()->runtimeVariablesOnStack);
    rtsi()->runtimeVariablesOnStack=run_tmp;
  }
  debugOutput( " Increased rtsi()->runtimeVariablesOnStack to %d  -- current index %d\n",rtsi()->maxRuntimeVariablesOnStackEndIndex ,rtsi()->runtimeVariablesOnStackEndIndex  );
}

/*********************************************************
 * Increase the size of all variables stored for runtime
 * evaluation
 ********************************************************/
void
RuntimeSystem_increaseSizeRuntimeVariables() {
  rtsi()->maxRuntimeVariablesEndIndex+=50;
  struct RuntimeVariablesType* run_tmp = (struct RuntimeVariablesType*)malloc(sizeof(struct RuntimeVariablesType)*(rtsi()->maxRuntimeVariablesEndIndex));
  if (run_tmp) {
    int i=0;
    for ( i=0;i<rtsi()->runtimeVariablesEndIndex;i++) {
      run_tmp[i].name=rtsi()->runtimeVariables[i].name;
      run_tmp[i].mangled_name =rtsi()->runtimeVariables[i].mangled_name;
      run_tmp[i].type =rtsi()->runtimeVariables[i].type;
      run_tmp[i].initialized =rtsi()->runtimeVariables[i].initialized;
      run_tmp[i].fileOpen =rtsi()->runtimeVariables[i].fileOpen;
      run_tmp[i].address =rtsi()->runtimeVariables[i].address;
      run_tmp[i].value =rtsi()->runtimeVariables[i].value;

      run_tmp[i].arrays =rtsi()->runtimeVariables[i].arrays;
    }
    free( rtsi()->runtimeVariables);
    rtsi()->runtimeVariables=run_tmp;
  } else {
    debugOutput("No space for Variables\n");
    exit(1);
  }
  debugOutput( " Increased rtsi()->runtimeVariables to %d  -- current index %d\n",rtsi()->maxRuntimeVariablesEndIndex ,rtsi()->runtimeVariablesEndIndex  );
}

/*********************************************************
 * For a given variable name, check if it is present
 * on the stack and return the mangled_name
 ********************************************************/
const char*
RuntimeSystem_findVariablesOnStack(const char* name) {
  const char* mang_name = NULL;
  int i=0;
  for ( i=0;i<rtsi()->runtimeVariablesOnStackEndIndex;i++) {
    const char* n =rtsi()->runtimeVariablesOnStack[i].name;
    if (*name==*n) {
      mang_name=rtsi()->runtimeVariablesOnStack[i].mangled_name;
      break;
    }
  }
  return mang_name;
}




/*********************************************************
 * This function is called when an array is created
 * name      : variable name
 * manglname : variable mangl_name
 * type      : Sage Type
 * dimension : 1 or 2
 * sizeA     : size of dimension 1
 * sizeB     : size of dimension 2
 * ismalloc  : Allocated through malloc?
 * filename  : file location
 * line      : linenumber
 ********************************************************/
void
RuntimeSystem_roseCreateArray(const char* name, const char* mangl_name, int dimension,  long int sizeA, long int sizeB,
			      int ismalloc, const char* filename, const char* line){

  if (rtsi()->arrayDebug)
    debugOutput( " >>> Called : roseCreateArray : %s dim %d - [%ld][%ld] file : %s line: %s\n",
	    name,dimension,sizeA, sizeB, filename, line);
  // fixme: Check if this already exists?
  int isarray=0;
  // does this variable exist on the stack?
  const char* stackvar = RuntimeSystem_findVariablesOnStack(name);
  int variableFound=0;
  if (stackvar) {
    // it exists
    debugOutput("This variable exists on the stack: %s\n",stackvar);
    //exit(1);
    variableFound=1;
    mangl_name=stackvar;
  }
  int varpos = RuntimeSystem_findVariablesPos(mangl_name,&isarray);
  if (varpos>=0) {
    // found var : dont increase the array index but use the index found
    variableFound=1;
    debugOutput("Variable found...\n");
  } else
    varpos = rtsi()->runtimeVariablesEndIndex;

  // If this is another malloc to this variable without a free and without having another pointer
  // to this memory region, then we need to trigger an error



  if (dimension==1) {
    // We create a one dimentional array
    if (rtsi()->arrayDebug)
      debugOutput("rtsi()->arrays1DEndIndex : %d rtsi()->maxArrays1DEndIndex: %d  \n",rtsi()->runtimeVariablesEndIndex,rtsi()->maxRuntimeVariablesEndIndex);
    if (variableFound==0 && rtsi()->runtimeVariablesEndIndex>=rtsi()->maxRuntimeVariablesEndIndex) {
      //increase the size of the array
      //RuntimeSystem_increaseSizeArray();
      RuntimeSystem_increaseSizeRuntimeVariables();
    }
    rtsi()->runtimeVariables[varpos].name=name;
    rtsi()->runtimeVariables[varpos].mangled_name=mangl_name;
    rtsi()->runtimeVariables[varpos].arrays =
      (struct ArraysType*)malloc(sizeof(struct ArraysType)*1);
    rtsi()->runtimeVariables[varpos].arrays->name=mangl_name;
    rtsi()->runtimeVariables[varpos].arrays->dim=1;
    rtsi()->runtimeVariables[varpos].arrays->size1=sizeA;
    rtsi()->runtimeVariables[varpos].arrays->size2=-1;
    rtsi()->runtimeVariables[varpos].arrays->ismalloc=ismalloc;
    if (variableFound==0)
      rtsi()->runtimeVariablesEndIndex++;

    if (rtsi()->arrayDebug)
      debugOutput( ".. Creating 1Dim array - size : %ld \n", sizeA);
  }
  else if (dimension==2) {
    // We create a two dimentional array
    //int pos = RuntimeSystem_findArrayName(mangl_name);
    //printf("%d == %d   %s == %s   isarray: %d \n",varpos,pos, mangl_name, mangl_name,isarray);
    //assert(varpos==pos);
    //    if (pos!=-1) {
    if (isarray==1) {
      //if (variableFound==1) {
      // this array already exists and may only have one dimension initialized
      long int totalsize = rtsi()->runtimeVariables[varpos].arrays->size1;
      if (rtsi()->arrayDebug)
	debugOutput( "..    Expanding 2nd-run 2Dim array - sizeA : %ld  sizeB: %ld \n", sizeA , sizeB );
      if (sizeA<0 || sizeA>=totalsize) {
	debugOutput( " rtsi()->Violation detected :  Array too small to allocate more memory \n");
	rtsi()->violation=1;
	// this is a weird error, lets stop here for now.
	exit(1);
      } else {
	if (rtsi()->arrayDebug)
	  debugOutput( " >>> CREATING Array : arr [%ld][%ld]  alloc:[%ld]=%ld \n",totalsize,sizeB,sizeA,sizeB);
	rtsi()->runtimeVariables[varpos].arrays->size2=sizeB;
      }
    } else {
      // new array
      if ( variableFound==0 && rtsi()->runtimeVariablesEndIndex>=rtsi()->maxRuntimeVariablesEndIndex) {
	//increase the size of the array
	//RuntimeSystem_increaseSizeArray();
	RuntimeSystem_increaseSizeRuntimeVariables();
      }
      debugOutput("Creating 2 dim array: current size : %d  max:%d ",varpos, rtsi()->maxRuntimeVariablesEndIndex);
      rtsi()->runtimeVariables[varpos].name=name;
      rtsi()->runtimeVariables[varpos].mangled_name=mangl_name;
      rtsi()->runtimeVariables[varpos].arrays =
	(struct ArraysType*)malloc(sizeof(struct ArraysType)*1);
      rtsi()->runtimeVariables[varpos].arrays->name=mangl_name;
      rtsi()->runtimeVariables[varpos].arrays->dim=2;
      rtsi()->runtimeVariables[varpos].arrays->size1=sizeA;
      rtsi()->runtimeVariables[varpos].arrays->size2=0;
      rtsi()->runtimeVariables[varpos].arrays->ismalloc=ismalloc;
      if (variableFound==0)
	rtsi()->runtimeVariablesEndIndex++;


      if (rtsi()->arrayDebug)
	debugOutput( ".. Creating 2Dim array - size : %ld \n", sizeA);
      if (sizeB!=-1) {
	rtsi()->runtimeVariables[varpos].arrays->size2=sizeB;
	if (rtsi()->arrayDebug)
	  debugOutput( "..    Expanding 2Dim array - sizeA : %ld   sizeB: %ld \n", sizeA, sizeB);
      }
    }
  }

  Rted_debugDialog(filename, atoi(line));
}

/*********************************************************
 * This function is called when an array is accessed
 * name      : variable name
 * posA      : array[posA][]
 * posB      : array[][posB]
 * filename  : file location
 * line      : linenumber
 * stmtStr   : unparsed version of the line to be used for error message
 ********************************************************/
void
RuntimeSystem_roseArrayAccess(const char* name, int posA, int posB, const char* filename, const char* line, const char* stmtStr){
  if (rtsi()->arrayDebug)
    debugOutput( "    Called : roseArrayAccess : %s ... ", (name));

  // check the stack if the variable is part of a function call
  const char* mangl_name=RuntimeSystem_findVariablesOnStack(name);
  if (mangl_name)
    name=mangl_name;

  //int pos = RuntimeSystem_findArrayName(name);
  int isarray = 0;
  int pos = RuntimeSystem_findVariablesPos(name,&isarray);
  if (rtsi()->arrayDebug)
    debugOutput("roseArrayAccess: looking for variable %s,  found at pos: %d \n",name,pos);
  if (pos!=-1 && isarray==1) {
    if (rtsi()->runtimeVariables[pos].arrays &&
	rtsi()->runtimeVariables[pos].arrays->dim==1) {
      int size = rtsi()->runtimeVariables[pos].arrays->size1;
      if (rtsi()->arrayDebug)
	debugOutput( "       Found 1Dim array :  size: %d , access [%d] \n", size, posA);
      if (posB!=-1) {
	debugOutput( " Seems like this is not a valid 1Dim array : %s  line: %s\n", filename, line);
	exit(1);
      }
      if (posA>=size || posA<0) {
	const char* res1 = ((const char*)"Invalid Operation,  array size=");
	const char* res2 = ((const char*)"  accessing=");
	int sizeInt = 2*sizeof(int);
	char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
	sprintf(res,"%s%d%s%d",res1,size,res2,posA);
	rtsi()->violation=1;
	RuntimeSystem_callExit(filename, line, res, stmtStr);
      }
    }
    else if (rtsi()->runtimeVariables[pos].arrays &&
	     rtsi()->runtimeVariables[pos].arrays->dim==2) {
      int sizeA = rtsi()->runtimeVariables[pos].arrays->size1;
      int sizeB = rtsi()->runtimeVariables[pos].arrays->size2;
      if (rtsi()->arrayDebug)
	debugOutput( "  Found 2Dim array :  size: [%d][%d]  pos: [%d][%d] \n",
		sizeA, sizeB,  posA , posB);
      // allow arr[posA][posB] && arr[posA]  both 2Dim!
      if ((posA>=sizeA || posA<0) || posB>=sizeB || posB<0) {
	const char* res1 = ((const char*)"Invalid Operation,  array size: [");
	const char* res2 = ((const char*)"][");
	const char* res3 = ((const char*)"]   accessing: [");
	const char* res4 = ((const char*)"][");
	const char* res5 = ((const char*)"]");
	int sizeInt = 4*sizeof(int);
	char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
	sprintf(res,"%s%d%s%d%s%d%s%d%s",res1,sizeA,res2,sizeB,res3,posA,res4,posB,res5);
	rtsi()->violation=1;
	debugOutput("%s %s %s %s\n",filename,line,res,stmtStr);
	//exit(1);
	RuntimeSystem_callExit(filename, line, res,stmtStr);
      }
    }
  } else {
    debugOutput("\n");
    debugOutput( " >>> No such array was created. Can't access it. %s  line : %s \n" , filename, line);
    exit(1);
  }

  Rted_debugDialog(filename, atoi(line));
}

// ***************************************** ARRAY FUNCTIONS *************************************








// ***************************************** FUNCTION CALL *************************************
/*********************************************************
 * Check if a function call is interesting, i.e. contains a
 * call to a function that we need to check the parameters of
 ********************************************************/
int
RuntimeSystem_isInterestingFunctionCall(const char* name) {
  int interesting=0;//false;
  if ( ( strcmp(name,"memcpy")==0 ||
	 strcmp(name ,"memmove")==0 ||
	 strcmp(name ,"strcpy")==0 ||
	 strcmp(name ,"strncpy")==0 ||
	 strcmp(name ,"strcat")==0 ||
	 strcmp(name ,"strncat")==0 ||
	 strcmp(name ,"strlen")==0 ||
	 strcmp(name ,"strchr")==0 ||
	 strcmp(name ,"strpbrk")==0 ||
	 strcmp(name ,"strspn")==0  ||
	 strcmp(name ,"strstr")==0
	 )) {
    interesting=1;
  }
  return interesting;
}

/*********************************************************
 * Check if the function that is called is modifying
 * i.e. whether it changes memory or just accesses it.
 * If it accesses memory, we need to ensure that the
 * memory is NULL terminated if const char*
 * we assume that parameter 2 than has the \0 token
 ********************************************************/
int
RuntimeSystem_isModifyingOp(const char* name) {
  int modifying=0;//false;
  // we do not need the actualLength of the
  // first parameter for the following
  if ( ( strcmp(name,"memcpy")==0 ||
	 strcmp(name ,"memmove")==0 ||
	 strcmp(name ,"strcpy")==0 ||
	 strcmp(name ,"strncpy")==0
	 )) {
    modifying=1;
  }
  return modifying;
}

/*********************************************************
 * Return the number of parameters for a specific function call
 ********************************************************/
int
RuntimeSystem_getParamtersForFuncCall(const char* name) {
  int dim=0;
  if ( ( strcmp(name,"memcpy")==0 ||
	 strcmp(name ,"memmove")==0 ||
	 strcmp(name ,"strncat")==0 )) {
    dim=3;
  } else if ((
	      strcmp(name ,"strncpy")==0 ||
	      strcmp(name ,"strcpy")==0 ||
	      strcmp(name ,"strchr")==0 ||
	      strcmp(name ,"strpbrk")==0 ||
	      strcmp(name ,"strspn")==0  ||
	      strcmp(name ,"strstr")==0  ||
	      strcmp(name ,"strcat")==0 )) {
    dim=2;
  } else if ((
	      strcmp(name ,"strlen")==0
	      )) {
    dim=1;
  }
  return dim;
}

/*********************************************************
 * Check if a function call is a call to a function
 * on our ignore list. We do not want to check those
 * functions right now.
 * This check makes sure that we dont push variables
 * on the stack for functions that we dont check
 * and hence the generated code is cleaner
 ********************************************************/
int
RuntimeSystem_isFileIOFunctionCall(const char* name) {
  int interesting=0;//false;
  if ( ( strcmp(name,"fopen")==0 ||
	 strcmp(name ,"fgetc")==0 ||
	 strcmp(name ,"fputc")==0 ||
	 strcmp(name ,"fclose")==0
	 )) {
    interesting=1;
  }
  return interesting;
}



/*********************************************************
 * Return the number of parameters for a specific function call
 ********************************************************/
int
RuntimeSystem_isSizeOfVariableKnown(const char* mangled_name) {
  int size=-1;
  //int pos = RuntimeSystem_findArrayName(mangled_name);
  int isarray = 0;
  int pos = RuntimeSystem_findVariablesPos(mangled_name,&isarray);

  if (pos!=-1 && isarray==1) {
    if (rtsi()->runtimeVariables[pos].arrays->dim==1)
      size = rtsi()->runtimeVariables[pos].arrays->size1;
    else
      // the two dim case is not handled
      assert(0==1);
  }
  return size;
}

/*********************************************************
 * This function is called when one of the following functions in the code is called:
 * memcpy, memmove, strcpy, strncpy, strcat, strncat
 * fname     : function name that is being called
 * args      : arguments to that function
 *           : format : arg1 as string, arg1 as var name (if applicable),
 *           :          arg2 as string, arg2 as var name (if applicable),
 *           :          number of const char to copy (if applicable)
 * argSzie   : Number of arguments
 * filename  : file location
 * line      : linenumber
 * stmtStr   : unparsed version of the line to be used for error message
 ********************************************************/
void
RuntimeSystem_handleSpecialFunctionCalls(const char* fname,const char** args, int argsSize,
					 const char* filename, const char* line, const char* stmtStr, const char* leftHandSideVar) {
  assert(argsSize>=1);
  // parameter 1
  assert(args[0]);
  assert(args[1]);
  int parameters=RuntimeSystem_getParamtersForFuncCall(fname);
  if (parameters==2)
    assert(argsSize>=4);
  if (parameters==3)
    assert(argsSize>=5);

  const char* param1StringVal = args[0];
  int param1ActualLength =-1;
  int sizeKnown = RuntimeSystem_isSizeOfVariableKnown(args[1]);
  if (rtsi()->funccallDebug)
    debugOutput("Handling param1 - Size of second paramter is dynamic ? size = %d\n",sizeKnown);
  int param1AllocLength = -2;
  if (sizeKnown!=-1) { // unknown size meaning that variable not found
    param1AllocLength = sizeKnown;
    //param1ActualLength = 0;//sizeKnown-1;
  } else
    param1AllocLength = atoi(args[1]);
  assert(param1StringVal);

  if (rtsi()->funccallDebug)
    debugOutput("Handling param1-1 - dynamic ? size = %d   param1AllocLength = %d, param1ActualLength = %d\n",sizeKnown,param1AllocLength,param1ActualLength);

  // determine the actual size of each of the strings
  // we need to do this only for certain functions!
  // This is important because otherwise e.g. strcpy
  // will show null terminated string also this is not relevant
  // for str1=malloc(5);  strcpy(str1,"la")
  int modifyingCall = RuntimeSystem_isModifyingOp(fname);
  if (modifyingCall==0) {
    const char* end1 = NULL;
    const char *iter=NULL;
    int count=0;
    debugOutput("............ Printing : %d  \n",count);
    for ( iter = param1StringVal; *iter != '\0'; ++iter) {
      debugOutput("............ Printing : %d : '%s' \n",count++,iter);
      end1 = iter;
    }
    if (end1==NULL)
      end1= param1StringVal;
    assert(end1);
    int length1 =  (end1-param1StringVal)+1;
    param1ActualLength = length1;
  } else {
    // if the operation is modifying, e.g. memcpy, we want the actual size to be 0
    param1ActualLength=0;
  }

  if (rtsi()->funccallDebug)
    debugOutput("Handling param1-2 - dynamic ? size = %d   param1AllocLength = %d, param1ActualLength = %d     isModifyingCall : %d \n",sizeKnown,param1AllocLength,param1ActualLength, modifyingCall);

  assert(param1ActualLength>-1);
  if (rtsi()->funccallDebug)
    debugOutput("1: Number of parameters : %d -- param1AllocLength : %d %s  -- param1ActualLength %d\n",parameters, param1AllocLength, args[1], param1ActualLength);
  if (param1AllocLength==0) {
    // adjust the size of the allocation to the actual size +1
    param1AllocLength=param1ActualLength+1;
    if (rtsi()->funccallDebug)
      debugOutput("1: Expanding : Number of parameters : %d -- param1AllocLength : %d %s  -- param1ActualLength %d\n",parameters, param1AllocLength, args[1], param1ActualLength);
  }


  // parameter 2
  const char* param2StringVal = (const char*)"";
  int param2ActualLength =-1;
  int param2AllocLength = -2;
  // parameter 3
  int param3Size = 0;

  int sizeKnown2 =-1;
  if (parameters>=2) {
    param2StringVal = args[2];
    param2ActualLength =-1;
    sizeKnown2= RuntimeSystem_isSizeOfVariableKnown(args[3]);
    if (rtsi()->funccallDebug)
      debugOutput("\nHandling param2 - Size of second paramter is dynamic ? size = %d\n",sizeKnown2);
    if (sizeKnown2!=-1) {
      param2AllocLength = sizeKnown2;
      //      param2ActualLength = 0;//sizeKnown2-1;
    } else
      param2AllocLength = atoi(args[3]);
    assert(param2StringVal);
  }
  if (parameters==3) {
    // determine the 3rd parameter if present, e.g. strcpy(p1,p2,size);
    if (argsSize>4)
      param3Size = strtol(args[4],NULL,10);
  }

  if (rtsi()->funccallDebug)
    debugOutput("Handling param2-1 - dynamic ? size = %d   param2AllocLength = %d, param2ActualLength = %d\n",sizeKnown2,param2AllocLength,param2ActualLength);

  if (parameters>=2) {

    const char* end2 = NULL;
    const char *iter2=NULL;
    for ( iter2 = param2StringVal; *iter2 != '\0'; ++iter2) {
      end2 = iter2;
    }
    if (end2==NULL)
      end2= param2StringVal;
    assert(end2);
    int length2 =  (end2-param2StringVal)+1;
    param2ActualLength = length2;


    if (param3Size==0)
      param3Size = param2ActualLength;
    assert(param2ActualLength>-1);
    assert(param3Size>0);
    if (rtsi()->funccallDebug)
      debugOutput("2: Number of parameters : %d -- param2AllocLength : %d %s  -- param2ActualLength %d\n",parameters, param2AllocLength, args[3], param2ActualLength);
    if (param2AllocLength==0) {
      param2AllocLength=param2ActualLength+1;
      if (rtsi()->funccallDebug)
	debugOutput("2: Expanding : Number of parameters : %d -- param2AllocLength : %d %s  -- param2ActualLength %d\n",parameters, param2AllocLength, args[3], param2ActualLength);
    }
  }
  if (rtsi()->funccallDebug)
    debugOutput("Handling param2-2 - dynamic ? size = %d   param2AllocLength = %d, param2ActualLength = %d\n",sizeKnown2,param2AllocLength,param2ActualLength);



  debugOutput("\nChecking if memory overlaps ... \n");
  debugOutput("(param2StringVal <= param1StringVal) && (param2StringVal+param2AllocLength >= param1StringVal)\n");
  debugOutput("(             %d <= %d             ) && (                                %d >= %d)\n",
	 param2StringVal, param1StringVal, (param2StringVal+param2ActualLength), param1StringVal);
  debugOutput("(param1StringVal <= param2StringVal) && (param1StringVal+param1AllocLength >= param2StringVal)\n");
  debugOutput("(             %d <= %d             ) && (                                %d >= %d)\n",
	 param1StringVal, param2StringVal, (param1StringVal+param1ActualLength), param2StringVal);
  // check for overlapping memory regions
  if (parameters>=2 &&
      (param2StringVal <= param1StringVal) && (param2StringVal+param2AllocLength>=param1StringVal) ||
      (param1StringVal <= param2StringVal) && (param1StringVal+param1AllocLength>=param2StringVal)) {
    if (rtsi()->funccallDebug)
      debugOutput( " >>>> Error : Memory regions overlap!   Size1: %d  Size2: %d\n",param1ActualLength , param2ActualLength);
    RuntimeSystem_callExit(filename, line, (const char*)"Memory regions overlap", stmtStr);
  }

  debugOutput("\nChecking if String NULL terminated ... \n");
  const char *iter4=NULL;
  int zero1pos=0;
  if (modifyingCall==0) {
    // do not check for the end const character of first operator if this is a modifying call, e.g. strcpy
    for ( iter4 = param1StringVal; *iter4 != '\0'; ++iter4) {
      debugOutput("%c",*iter4); zero1pos++;
    } debugOutput("---1 !!!!!!! Found 0 at pos : %d    param1ActualLength %d \n",zero1pos,param1ActualLength);
  }
  // check if the actual size is larger than the allocated size
  if ( zero1pos>param1ActualLength) {
    const char* res1 = ((const char*)"Param1 : String not NULL terminated. zero1pos: ");
    const char* res2 = ((const char*)"  > ActualLength:");
    int sizeInt = 2*sizeof(int);
    char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
    sprintf(res,"%s%d%s%d",res1,zero1pos,res2,param1ActualLength);
    RuntimeSystem_callExit(filename, line, res, stmtStr);
  } else if (param1ActualLength>=param1AllocLength) {
    const char* res1 = ((const char*)"Param1 : Writing/Reading outside allocated memory.  ActualLength = ");
    const char* res2 = ((const char*)" >= AllocLength = ");
    int sizeInt = 2*sizeof(int);
    char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
    sprintf(res,"%s%d%s%d",res1,param1ActualLength,res2,param1AllocLength);
    RuntimeSystem_callExit(filename, line, res, stmtStr);
  }

  if (parameters>=2) {
    const char *iter3=NULL;
    int zero2pos =0;
    for ( iter3 = param2StringVal; *iter3 != '\0'; ++iter3) {
      debugOutput("%c",*iter3); zero2pos++;
    } debugOutput("---2 !!!!!!! Found 0 at pos : %d     param2ActualLength %d \n",zero2pos,param2ActualLength);

    // check if the actual size is larger than the allocated size
    if ( zero2pos>param2ActualLength) {
      const char* res1 = ((const char*)"Param2 : String not NULL terminated. zero2pos: ");
      const char* res2 = ((const char*)"  > ActualLength:");
      int sizeInt = 2*sizeof(int);
      char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
      sprintf(res,"%s%d%s%d",res1,zero2pos,res2,param2ActualLength);
      RuntimeSystem_callExit(filename, line, res, stmtStr);
    } else if (param2ActualLength>=param2AllocLength) {
      const char* res1 = ((const char*)"Param2 : Writing/Reading outside allocated memory. ActualLength:");
      const char* res2 = ((const char*)" >= AllocLength:");
      int sizeInt = 2*sizeof(int);
      char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
      sprintf(res,"%s%d%s%d",res1,param2ActualLength,res2,param2AllocLength);
      RuntimeSystem_callExit(filename, line, res, stmtStr);
    }
  }

  if ( ( strcmp(fname,"strlen")==0 ||  // 1 param
	 strcmp(fname,"strchr")==0 // 1 param
	 )) {
    // checking one parameter for strlen(const char* without \0)
    if (rtsi()->funccallDebug)
      debugOutput("CHECK: Special Function call %s  p1: %s act1: %d alloc1: %d   \n", fname,
	     param1StringVal, param1ActualLength, param1AllocLength);
  }
  else
    if ( ( strcmp(fname ,"strcat")==0  || // 2 param
	   strcmp(fname ,"strncat")==0   // 3 param
	   )) {
      if (rtsi()->funccallDebug)
	debugOutput("CHECK: Special Function call - %s  p1: %s act1: %d alloc1: %d     p2: %s act2: %d alloc2: %d     p3: %d\n", fname,
	       param1StringVal, param1ActualLength, param1AllocLength,
	       param2StringVal, param2ActualLength, param2AllocLength, param3Size);
      // not handled yet
      if (parameters==2) {
	if ((param1ActualLength+param2AllocLength)>=param1AllocLength) {
	  // concatenation above the size of param1AllocLength
	  RuntimeSystem_callExit(filename, line, (const char*)"Writing beyond memory allocation for 1st parameter", stmtStr);
	}
      } else
	if (parameters==3) {
	  if ((param1ActualLength+param3Size)>=param1AllocLength) {
	    // concatenation above the size of param1AllocLength
	    RuntimeSystem_callExit(filename, line, (const char*)"Writing beyond memory allocation for 1st parameter", stmtStr);
	  }
	}

    }
    else
      if ( ( strcmp(fname,"memcpy")==0   || // 3 param
	     strcmp(fname ,"memmove")==0 || // 3 param
	     strcmp(fname ,"strcpy")==0  || // 2 param
	     strcmp(fname ,"strncpy")==0 || // 3 param
	     strcmp(fname ,"strpbrk")==0 || // 2 param
	     strcmp(fname ,"strspn")==0  || // 2 param
	     strcmp(fname ,"strstr")==0  // 2 param
	     )) {
	if (rtsi()->funccallDebug)
	  debugOutput("CHECK: Special Function call - %s  p1: %s act1: %d alloc1: %d     p2: %s act2: %d alloc2: %d   param3Size: %d\n",fname,
		 param1StringVal, param1ActualLength, param1AllocLength,
		 param2StringVal, param2ActualLength, param2AllocLength, param3Size);
	if (parameters==2) {
	  if ((param2AllocLength>param1AllocLength)) {
	    const char* res1 = ((const char*)"p2 Invalid Operation,  operand2:");
	    const char* res2 = ((const char*)"  >  operand1:");
	    int sizeInt = 2*sizeof(int);
	    char *res = (char*)malloc(strlen(res1) + strlen(res2) +sizeInt+ 1);
	    sprintf(res,"%s%d%s%d",res1,param2AllocLength,res2,param1AllocLength);
	    RuntimeSystem_callExit(filename, line, res, stmtStr);
	  }
	} else if (parameters==3) {
	  if ((param3Size>param1AllocLength || param3Size>param2AllocLength)) {
	    // make sure that if the strings do not overlap, they are both smaller than the amount of const chars to copy
	    const char* res1 = ((const char*)"p3 Invalid Operation,  operand3:");
	    const char* res2 = ((const char*)"  >  operand1:");
	    const char* res3 = ((const char*)"  or   operand3:");
	    const char* res4 = ((const char*)"  >  operand2:");
	    int sizeInt = 4*sizeof(int);
	     char *res = ( char*)malloc(strlen(res1) + strlen(res2) +strlen(res3) + strlen(res4)+sizeInt+ 1);
	    sprintf(res,"%s%d%s%d%s%d%s%d",res1,param3Size,res2,param1AllocLength,res3,
		    param3Size,res4,param2AllocLength);
	    RuntimeSystem_callExit(filename, line, res, stmtStr);
	  }
	} else assert(1==0);
      }
      else {
	// not handled yet. Need to check if this operation is leagal
	debugOutput("Unhandled special function: Checking special op : %s\n",fname);
	assert(1==0);
      }

  Rted_debugDialog(filename, atoi(line));

}





/*********************************************************
 * This function is called when one of the following functions in the code is called:
 * fopen, fgetc
 * fname     : function name that is being called
 * args      : arguments to that function
 *           : format : arg1 as string, arg1 as var name (if applicable),
 *           :          arg2 as string, arg2 as var name (if applicable),
 *           :          number of const char to copy (if applicable)
 * argSzie   : Number of arguments
 * filename  : file location
 * line      : linenumber
 * stmtStr   : unparsed version of the line to be used for error message
 ********************************************************/
void
RuntimeSystem_handleIOFunctionCall(const char* fname,const char** args,
				   int argsSize, const char* filename, const char* line,
				   const char* stmtStr, const char* leftHandSideVar) {
  assert(argsSize>=1);
  // parameter 1
  int parameters=RuntimeSystem_getParamtersForFuncCall(fname);
  if  (strcmp(fname,"fopen")==0) {
    // need 4 parameters, var, size, var, size
    assert(argsSize>=4);
    // we need to mark the variable with fopen that it is open for read/write
    const char* file = args[0];
    const char* fileLength = args[1];
    const char* readwrite = args[2];
    const char* readwriteLength = args[3];
    if(!leftHandSideVar) assert(0==1);
    struct RuntimeVariablesType* rvar = RuntimeSystem_findVariables(leftHandSideVar);
    if (leftHandSideVar) {
      debugOutput("Making sure that the var %s is set to : %s \n",leftHandSideVar,readwrite);
      rvar->fileOpen=readwrite;
      // make sure that the file exists
      FILE* fp = fopen(file,"r");
      if (fp==0) {
	// file does not exist
	RuntimeSystem_callExit(filename, line, (const char*)"No such file. Can not open this file.", stmtStr);
      }
    } else {
      debugOutput("File opend: This must be an error, The variable %s in fopen should be available.\n",leftHandSideVar);
      exit(1);
    }
  }
  else  if  (strcmp(fname,"fclose")==0) {
    // need 4 parameters, var, size, var, size
    assert(argsSize==1);
    // we need to mark the variable with fopen that it is open for read/write
    const char* var = args[0];
    assert(var);
    struct RuntimeVariablesType* rvar = RuntimeSystem_findVariables(var);
    if (rvar==NULL) {
      const char* stackvar = RuntimeSystem_findVariablesOnStack(var);
      if (stackvar) {
	debugOutput("Found the variable on the stack : %s \n",stackvar);
	rvar = RuntimeSystem_findVariables(stackvar);
      }
    }
    if (rvar) {
      if (strcmp(rvar->fileOpen,"no")==0) {
	// file closed although it never was opend
	RuntimeSystem_callExit(filename, line, (const char*)"Closing a file that never was opened.", stmtStr);
      } else {
	debugOutput("File closed for var: %s \n",rvar->name);
	rvar->fileOpen=(const char*)"no";
      }
    } else {
      debugOutput("File closed : This must be an error, The variable %s in fopen should be available.\n",var);
      exit(1);
    }
  }
  else if  (strcmp(fname,"fgetc")==0 ) {
    // only one parameter
    assert(argsSize==1);
    const char* var = args[0];
    // check if the variable used : fgetc(var) is a pointer to a file
    // that has been opened already, i.e. is file open?
    struct RuntimeVariablesType* rvar = RuntimeSystem_findVariables(var);
    if (rvar) {
      debugOutput("RuntimeSystem : RuntimeSystem_handleIOFunctionCall : found Variable %s (%s)  fileOpen? %s \n",
	     var, rvar->name, rvar->fileOpen);
      if (strcmp(rvar->fileOpen,"no")==0) {
	// the file is not open, we cant read/write
	RuntimeSystem_callExit(filename, line, (const char*)"File not open. Can't read/write to file.", stmtStr);
      }
      if (strstr( rvar->fileOpen, "r" )==0) {
	// reading a file that is not marked for reading
	RuntimeSystem_callExit(filename, line, (const char*)"Can not read from File. File not opened for reading.", stmtStr);
      }
    }
  }
  else if  (strcmp(fname,"fputc")==0 ) {
    // only one parameter
    assert(argsSize==2);
    const char* filestream = args[0];
    const char* var = args[1];
    // check if the variable used : fgetc(var) is a pointer to a file
    // that has been opened already, i.e. is file open?
    struct RuntimeVariablesType* rvar = RuntimeSystem_findVariables(var);
    if (var) {
      debugOutput("RuntimeSystem : RuntimeSystem_handleIOFunctionCall : found Variable %s (%s)  fileOpen? %s \n",
	     var, rvar->name, rvar->fileOpen);
      if (strcmp(rvar->fileOpen,"no")==0) {
	// the file is not open, we cant read/write
	RuntimeSystem_callExit(filename, line, (const char*)"File not open. Can't read/write to file.", stmtStr);
      }
      if (strstr( rvar->fileOpen, "w" )==0) {
	// reading a file that is not marked for reading
	RuntimeSystem_callExit(filename, line, (const char*)"Can not write to File. File not opened for writing.", stmtStr);
      }

    }
  }

  Rted_debugDialog(filename, atoi(line));

}





/*********************************************************
 * This function is called when a function is called
 * The number of parameters is variable but some of the are fixed:
 * count        : number of parameters
 * name         : variable name if it is a variable to be put on the stack
 *              : function name if it is a function that is being called
 * mangled_name : mangled name of the above
 * scope        : Scope indicator (might not be necessary)
 * insertBefore : Indicates whether to push or pop a variable form the stack
 * filename     : file location
 * line         : linenumber
 * stmtStr      : unparsed version of the line to be used for error message
 * ...          : variable amount of additional parameters
 ********************************************************/
void
RuntimeSystem_roseFunctionCall(int count, ...) {
  // handle the parameters within this call
  debugOutput("RTED - Function Call\n");
  va_list vl;
  va_start(vl,count);
  const char** args = (const char**)malloc(sizeof(const char*)*count+1);
  int posArgs=0;
  const char* name = NULL;
  const char* filename = NULL;
  const char* line=NULL;
  const char* stmtStr=NULL;
  const char* leftVar=NULL;
  //cerr << "arguments : " <<  count << endl;
  int i=0;
  for ( i=0;i<count;i++)    {
    const char* val=  va_arg(vl,const char*);
    if (val) // && i<4)
      debugOutput("  %d      val : '%s' ---",i,val);
    const char *iter2=NULL;
    int size =0;
    for ( iter2 = val; *iter2 != '\0'; ++iter2) {
      debugOutput("%c",*iter2); size++;
    } debugOutput("--- size : %d \n",size);

    if (i==0) name = val;
    else if (i==1) filename =  val;
    else if (i==2) line = val;
    else if (i==3) stmtStr = val;
    else if (i==4) leftVar = val;
    else {
      args[posArgs++]=val;
    }
  }
  va_end(vl);


  if (rtsi()->funccallDebug)
    debugOutput( "roseFunctionCall :: %s \n", name );
  if (RuntimeSystem_isInterestingFunctionCall(name)==1) {
    // if the string name is one of the above, we handle it specially
    RuntimeSystem_handleSpecialFunctionCalls(name, args, posArgs, filename, line, stmtStr, leftVar);
  } else if (RuntimeSystem_isFileIOFunctionCall(name)==1) {
    RuntimeSystem_handleIOFunctionCall(name, args, posArgs, filename, line, stmtStr, leftVar);
  } else {
    debugOutput("Unknown Function call to RuntimeSystem!\n");
    exit(1);
  }
}


/*********************************************************
 * This function is called when a variable is put or dropped from stack
 * stack variables are used to keep track of what variables are passed
 * to functions. Their mangled_names help to identify the real definitions.
 * name         : variable name if it is a variable to be put on the stack
 * mangled_name : mangled name of the above
 * insertBefore : Indicates whether to push or pop a variable form the stack
 ********************************************************/
void
RuntimeSystem_roseCallStack(const char* name, const char* mangl_name,
			    const char* beforeStr,
			    const char* filename, const char* line) {
  // we want to remember the varRefs that are passed via function calls to functions
  // if before ==true
  // add the current varRef (name) on stack
  // else remove from stack
  int before =0; // false
  if (strcmp(beforeStr,"true")==0)
    before=1;

  // check if this variable is on the stack, if yes, we want to use that mangled_name!
  const char* stackvar = RuntimeSystem_findVariablesOnStack(name);
  if (stackvar) {
    // it exists
    debugOutput("This variable exists on the stack: %s\n",stackvar);
    //exit(1);
    mangl_name=stackvar;
  }

  // before we add a variable to the stack we want to make sure that all
  // variables for that function are initialized!
  // find the variable and make sure it is initialized
  struct RuntimeVariablesType* rvar = RuntimeSystem_findVariables(mangl_name);
  if (rvar==NULL) {
    debugOutput("Variable not found .... %s\n",mangl_name);
    // variable not found?!
    exit(1);
  }
  int initialized = rvar->initialized;
  debugOutput("CallStack: Checking if %s is initialized: %d.\n",name,initialized);
  if (initialized==0) {
    // lets not trigger this error right now
    // fixme
    //RuntimeSystem_callExit(filename, line, (const char*)"Variable is not initialized:", name);
  } else
    debugOutput("CallStack: Variable is initialized.\n");

  if (before) {
    if (rtsi()->runtimeVariablesOnStackEndIndex>=rtsi()->maxRuntimeVariablesOnStackEndIndex) {
      //increase the size of the array
      RuntimeSystem_increaseSizeRuntimeVariablesOnStack();
    }
    rtsi()->runtimeVariablesOnStack[rtsi()->runtimeVariablesOnStackEndIndex].name=name;
    rtsi()->runtimeVariablesOnStack[rtsi()->runtimeVariablesOnStackEndIndex].mangled_name=mangl_name;
    rtsi()->runtimeVariablesOnStackEndIndex++;

  }
  else {
    rtsi()->runtimeVariablesOnStackEndIndex--;
    //      rtsi()->runtimeVariablesOnStack[rtsi()->runtimeVariablesOnStackEndIndex].name=NULL;
    //rtsi()->runtimeVariablesOnStack[rtsi()->runtimeVariablesOnStackEndIndex].mangled_name=NULL;

    //RuntimeVariables* var = rtsi()->runtimeVariablesOnStack.back();
    //rtsi()->runtimeVariablesOnStack.pop_back();
    //delete var;
  }


}
// ***************************************** FUNCTION CALL *************************************





// ***************************************** VARIABLE DECLARATIONS *************************************
/*********************************************************
 * This function tells the runtime system that a variable is created
 * we store the type of the variable and whether it has been intialized
 ********************************************************/
void RuntimeSystem_roseCreateVariable( const char* name,
				      const char* mangled_name,
				      const char* type,
				      int init,
				      const char* fOpen,
				      const char* filename, const char* line,
				      const char* lineTransformed) {
  if (rtsi()->runtimeVariablesEndIndex>=rtsi()->maxRuntimeVariablesEndIndex) {
    //increase the size of the array
    RuntimeSystem_increaseSizeRuntimeVariables();
  }
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].name=name;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].mangled_name=mangled_name;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].type=type;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].initialized=init;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].fileOpen=fOpen;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].address=0;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].value=-1;
  rtsi()->runtimeVariables[rtsi()->runtimeVariablesEndIndex].arrays=0;
  rtsi()->runtimeVariablesEndIndex++;


  Rted_debugDialog(filename, atoi(line));

  debugOutput("CreateVariable: You have just created a run-time variable:\n");
  debugOutput("  name: %s \n", name);
  debugOutput("  mangl_name: %s \n",mangled_name);
  debugOutput("  type: %s \n",type);
  debugOutput("  initialized: %d \n",init);
  debugOutput("  fileOpen: %s \n",fOpen);
  debugOutput("  runtimeVariablesEndIndex: %d \n\n",rtsi()->runtimeVariablesEndIndex);
}

/*********************************************************
 * For a given variable name, check if it is present
 * in the pool of variables created and return variable
 ********************************************************/
struct RuntimeVariablesType*
RuntimeSystem_findVariables(const char* mangled_name) {
  struct RuntimeVariablesType* var=NULL;
  int i=0;
  for ( i=0;i<rtsi()->runtimeVariablesEndIndex;i++) {
    const char* n =rtsi()->runtimeVariables[i].mangled_name;
    if (strcmp(mangled_name,n)==0) {
      var =&(rtsi()->runtimeVariables[i]);
      //debugOutput("var : %s   (%s == %s) \n",var, mangled_name, n );
      break;
    }
  }
  return var;
}

/*********************************************************
 * For a given address, return the variables associated with it
 ********************************************************/
struct MemoryVariableType*
RuntimeSystem_findMemory(long int address) {
  struct MemoryVariableType** var=NULL;
  int i=0;
  for ( i=0;i<rtsi()->runtimeMemoryEndIndex;i++) {
    long int addr =rtsi()->runtimeMemory[i].address;
    //    if (addrM) {
    //long int addr = addrM->address;
    if (addr==address) {
      if (rtsi()->runtimeMemory[i].variables) {
	var =&(rtsi()->runtimeMemory[i].variables);
	//debugOutput("var : %s   (%s == %s) \n",var, mangled_name, n );
	break;
      } //else
      //break;
    }
    //}
  }
  return *var;
}

/*********************************************************
 * For a given variable name, check if it is present
 * in the pool of variables created and return variable
 ********************************************************/
int
RuntimeSystem_findVariablesPos(const char* mangled_name, int* isarray) {
  int i=0;
  for ( i=0;i<rtsi()->runtimeVariablesEndIndex;i++) {
    const char* n =rtsi()->runtimeVariables[i].mangled_name;
    if (strcmp(mangled_name,n)==0) {
      struct ArraysType* array = rtsi()->runtimeVariables[i].arrays;
      //debugOutput("..........findArrayName : name : %s, array ? %s \n",mangled_name,array);
      if (array)
	*isarray=1;
      return i;
    }
  }
  return -1;
}

/*********************************************************
 * For a given variable name, check if it is present
 * in the pool of variables created and return mangled_name
 ********************************************************/
void
RuntimeSystem_roseInitVariable(const char* name,
			       const char* mangled_name,
			       const char* typeOfVar2,
			       const char* baseType,
			       unsigned long long address,
			       unsigned long long value,
			       int ismalloc,
			       const char* filename, const char* line, const char* stmtStr) {
  debugOutput("InitVariable: Request for %s.    address: %d   value: %d   type: %s \n",mangled_name, address, value, typeOfVar2);
  int i=0;
  int varFound=0;
  int assgnToPointer = 0;

  const char* stackvar = RuntimeSystem_findVariablesOnStack(name);
  if (stackvar) {
    // it exists
    debugOutput("This variable exists on the stack: %s\n",stackvar);
    //exit(1);
    varFound=1;
    mangled_name=stackvar;
  }

  // go through all variables and check if it is present.
  for ( i=0;i<rtsi()->runtimeVariablesEndIndex;i++) {
    const char* n =rtsi()->runtimeVariables[i].mangled_name;
    if (strcmp(mangled_name,n)==0){
      varFound=1;
      debugOutput("Found the variable %s at index %d \n",n, i);

      if( strcmp( "SgPointerType", rtsi()->runtimeVariables[ i].type) == 0)
        // assignee is a pointer which may be the last one to point to a
        // particular region of memory
        assgnToPointer = 1;

      // variable exists, lets init it
      // create init on heap
      //const char* init = (const char*)"true";
      int init = 1; //true;
      rtsi()->runtimeVariables[i].initialized=init;
      rtsi()->runtimeVariables[i].value=value;
      // we don't expect the var type to change
      assert( strcmp( rtsi()->runtimeVariables[i].type, typeOfVar2) == 0);
      rtsi()->runtimeVariables[i].type=typeOfVar2;

      //int ismalloc = 0;
      int sizeArray =0;
      if (rtsi()->runtimeVariables[i].arrays) {
        //ismalloc=rtsi()->runtimeVariables[i].arrays->ismalloc;
        sizeArray = rtsi()->runtimeVariables[i].arrays->size1;
        if (rtsi()->runtimeVariables[i].arrays->dim==2)
          sizeArray = sizeArray*rtsi()->runtimeVariables[i].arrays->size2;
        sizeArray = sizeArray * getSizeOfSgType(rtsi()->runtimeVariables[i].type);
      }


      // create memory association only if it is a heap variable
      if ( assgnToPointer) {
        if (rtsi()->runtimeVariables[i].address) {
          // this variable has an address, we are re-assigning its
          // address or variable ?
          debugOutput("\n>>>> >> Already allocated memory at address %lld  and size %d  for  %s\n",
              rtsi()->runtimeVariables[i].address->address, rtsi()->runtimeVariables[i].address->size, stmtStr);
          // check if this variable is allocating memory through
          // a pointer that was not freed before
          int result = checkMemoryLeakIssues(i, address, filename, line,stmtStr);
          if (result==0) {
            // there are no issues and we should re-assign the
            // variable address and size
            // we need to remove that variable from that address
            struct RuntimeVariablesType* runtimevar = &(rtsi()->runtimeVariables[i]);
            RuntimeSystem_RemoveVariableFromMemory(address,runtimevar);
            // and add it to another address
            struct MemoryType* tmp_memory = RuntimeSystem_AllocateMemory(address,sizeArray,runtimevar);
            rtsi()->runtimeVariables[i].address = tmp_memory;
            debugOutput("\n>>>> >> Re-Allocated memory at address %lld  and size %d for   %s \n",
                rtsi()->runtimeVariables[i].address->address, rtsi()->runtimeVariables[i].address->size, stmtStr);
          }
        } else {
          // this variable has no previous address, we need to fix this
          struct RuntimeVariablesType* runtimevar = &(rtsi()->runtimeVariables[i]);
          struct MemoryType* tmp_memory = RuntimeSystem_AllocateMemory(address,sizeArray,runtimevar);
          rtsi()->runtimeVariables[i].address = tmp_memory;
          debugOutput("\n>>>> >> Allocated memory at address %lld  and size %d   for   %s \n",
              rtsi()->runtimeVariables[i].address->address, rtsi()->runtimeVariables[i].address->size, stmtStr);
        }
      }

      //debugOutput(">> InitVariable: Found variable: %s as initialized.    address: %lld   value: %lld   ismalloc:%d \n",
      //	     mangled_name, address, value, ismalloc);
      break;
    }
  } // for

  Rted_debugDialog(filename, atoi(line));

  if (varFound==0) {
    debugOutput("No such variable was found\n");
  }
}


/*********************************************************
 * This function tells the runtime system that a variable is used
 ********************************************************/
void RuntimeSystem_roseAccessVariable( const char* name,
				       const char* mangled_name,
				       const char* filename, const char* line,
				       const char* lineTransformed,
				       const char* stmtStr) {
  debugOutput("AccessVariable: Request for %s  %s.    \n",name, mangled_name);
  int i=0;
  int varFound=0;

  const char* stackvar = RuntimeSystem_findVariablesOnStack(name);
  if (stackvar) {
    // it exists
    debugOutput("This variable exists on the stack: %s\n",stackvar);
    //exit(1);
    varFound=1;
    mangled_name=stackvar;
  }

  // go through all variables and check if it is present.
  for ( i=0;i<rtsi()->runtimeVariablesEndIndex;i++) {
    const char* n =rtsi()->runtimeVariables[i].mangled_name;
    if (strcmp(mangled_name,n)==0){
      varFound=1;
      debugOutput("Found the variable %s at index %d \n",n, i);

      // variable exists, lets make sure it is initialized - if not than that is not good.
      int init = rtsi()->runtimeVariables[i].initialized;
      if (init==0) {
	// it is not initialized. Stop
	RuntimeSystem_callExit(filename, line, (const char*)"This variable has not been initialized.", stmtStr);
      }
      break;
    } // if
  } // for

  Rted_debugDialog(filename, atoi(line));

  if (varFound==0) {
    debugOutput("No such variable was found\n");
    exit(1); // should not happen!
  }
}

// ***************************************** VARIABLE DECLARATIONS *************************************
