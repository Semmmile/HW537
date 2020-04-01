//include files
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "mapreduce.h"

#define TABLE_SIZE 10 * 100
#define HashPar 10
//data structures
/**
 * hash
 * 0    - key1 - key2 - key3 -----
 *         v1     v1
 *         v2     v2
 *         v3
 * 
 * 1    - key1 - key2 - key3 -----
 *         v1     v1
 *         v2     v2
 *         v3
 * 2
 */
//
// a) store mappers output to be accessed by combiner (Arraylist)
// b) store combiners output to be accessed by reducers.

// hashtable's chain list
struct mapper_par_t
{
    Mapper Mapfun;
    char *arg;
};

struct KeyMem_t
{
    char* key;
    struct KeyNode_t *lastkey;
    //ValueNode *Vnxt;
    //struct Node *last;
};

struct KeyHead_t {
    //char* key;
   struct  KeyNode_t *Knxt;
    //ValueNode *Vnxt;
    //struct Node *last;
};

struct KeyNode_t {
    char* key;
    struct KeyNode_t *Knxt;
    struct ValueNode_t *Vnxt;
  //  struct ValueNode_t *Vend;
    int num;
    //struct Node *last;
};

struct ValueNode_t
{
    char *value;
    struct ValueNode_t *V_nxt;
};

// hash table
struct HashMap_t
{
     struct KeyHead_t* hashlist[HashPar];
    pid_t ProcessID;
};

typedef struct HashMap_t HashMap;
typedef struct ValueNode_t ValueNode;
typedef struct KeyHead_t KeyHead;
typedef struct KeyNode_t KeyNode;
typedef struct KeyMem_t keymen;
typedef struct mapper_par_t map_par;

    //TODO: init data structure

    void
    KeyNode_init(KeyNode *node)
{
    node->key = NULL;
    node->Knxt = NULL;
    node->Vnxt = NULL;
   // node->Vend = NULL;
    node->num = 0;
};

 void ValueNode_init(ValueNode* vnode){
    vnode->value = NULL;
    vnode->V_nxt = NULL;

  //  ValueNode a;

}

void HashMap_init(HashMap *hmp)
{   
    //KeyHead head[HashPar];
    for(int i=0; i<HashPar; i++)
    {
        //head[i].Knxt = NULL;
        //hmp->hashlist[i] = &head[i];
        KeyHead *head_test;
        head_test = malloc(sizeof(KeyHead));
        head_test->Knxt = NULL;
        hmp->hashlist[i] = head_test;
    }

    hmp->ProcessID =0;

}

// first go through the list and find the key, then insert the value
KeyNode* KeyNode_insert(KeyHead *khead, char* key, char* value){
    //init 

    printf("============\n");
    KeyNode* Kcurrent;
    ValueNode* Vcurrent;

    // find the key;
    // if the list is empty
    if(khead->Knxt == NULL){
        printf("insert to the first, key = %s, val = %s \n", key, value);
        KeyNode *Kpair = malloc(sizeof(KeyNode));
        ValueNode *Vpair = malloc(sizeof(ValueNode));

        KeyNode_init(Kpair);
        ValueNode_init(Vpair);
        Kpair->key = key;
        Vpair->value = value;

        Kpair->Vnxt = Vpair;
      //  Kpair->Vend = Vpair;
        khead->Knxt = Kpair;

        return Kpair;

    }
    else 
    {
        Kcurrent=khead->Knxt;

        if (strcmp(Kcurrent->key, key) == 0)
        {

            ValueNode *Vpair = malloc(sizeof(ValueNode));
            ValueNode_init(Vpair);
            Vpair->value = value;
            Vpair->V_nxt = Kcurrent->Vnxt;
            Kcurrent->Vnxt = Vpair;

            printf("insert in the middle\n");
            return Kcurrent;
        }

        while (Kcurrent->Knxt != NULL)
        {
            // if the key exists
            if (strcmp(Kcurrent->key,key)==0) {

                ValueNode *Vpair = malloc(sizeof(ValueNode));
                ValueNode_init(Vpair);
                Vpair->value = value;
                Vpair->V_nxt = Kcurrent->Vnxt;
                Kcurrent->Vnxt = Vpair;
        
                printf("insert in the middle\n");
                return Kcurrent;
                break;
            }
            else{
                Kcurrent = Kcurrent->Knxt;
            }
        }
        // if key is new:
        printf("insert to the end \n");
        KeyNode *Kpair = malloc(sizeof(KeyNode));
        ValueNode *Vpair = malloc(sizeof(ValueNode));
        KeyNode_init(Kpair);
        ValueNode_init(Vpair);
        Kpair->key = key;
        Vpair->value = value;
        Kpair->Vnxt = Vpair;

        Kcurrent->Knxt = Kpair;
     //   Kcurrent->Vend = Vpair;
        return Kpair;
    }

}


void free_Vnode(KeyNode* knode){
    
    ValueNode* vnode;
    vnode = knode->Vnxt;

    knode->Vnxt = knode->Vnxt->V_nxt;
    free(vnode);
}


static HashMap* MAPMEM[10];
static Combiner MRcomb;
static keymen* keylast;
static map_par *map_paramer;

//ReduceGetter RedGetter( char * key, int partition_num);
extern  char* GetterComb(char* key){

    printf("----getrun----- \n");
    KeyNode * key_sel;
    key_sel = (KeyNode *)key;

    ValueNode *vsel;
    char * revalue;
    revalue = malloc(sizeof(ValueNode));
    //printf("in the getter, value is %s\n", key);

    if (key_sel->Vnxt == NULL){
        printf("* now it is empty, key is %s \n", key_sel->key);
        return NULL;
    }
    else
    {
        printf("* not empty \n");
        vsel = key_sel->Vnxt;
        if (key_sel->Vnxt->V_nxt == NULL){
            key_sel->Vnxt = NULL;
          //  key_sel->Vend = NULL;
            printf("*only one \n");
        }else
        {
            key_sel->Vnxt = vsel->V_nxt;
        }
        
        revalue =  vsel->value;
        free (vsel);
        printf("* combiner get (key: %s, value:%s) \n", key_sel->key, revalue);
        return revalue;
    }
        
        
   // key_sel->Vnxt = vsel->V_nxt;
}

//get_nxt function used to iterate over values that need to be merged

/**
 * @brief propogate data from a mapper to its respective combiner
 * take k/v pairs from single mapper 
 * and temporarily store them so that combine()can retrive and merge its values for each key
 */
void MR_EmitToCombiner(char *key, char *value){
    unsigned long hsn = MR_DefaultHashPartition(key, 10);
    //printf("this is in the EmitToCombiner \n ");
    printf("(%s,%s) hash:%ld \n",key, value, hsn);
    //printf("in the main function :%s \n", MAPMEM->hashlist[0]->Knxt->key);
    KeyNode *keytree = KeyNode_insert(MAPMEM[0]->hashlist[hsn], key, value);
    printf("read from map: %s \n", MAPMEM[0]->hashlist[hsn]->Knxt->key);

    if (strcmp(keylast->key , "0000")==0)
    {
        keylast->key = key;
        keylast->lastkey = keytree;
    }
    else{

        if(strcmp(keylast->key , key)!=0){

            //keylast->key = key;
            void *addr;

            if (keylast->lastkey == NULL){
                 addr = keytree;
                }
            else
                {
                addr = keylast->lastkey;
                keylast->key = key;
                keylast->lastkey = keytree;
            }

        Combiner combineself;
        combineself = MRcomb;
        printf("the keyword to be combined is %s",keylast->lastkey->key );
        combineself(addr, GetterComb);
        }
       /* else if (keylast->key != NULL)
        {
            void *addr;
            addr = keytree;
            keylast->key = NULL;
            keylast->lastkey = NULL;
            Combiner combineself;
            combineself = MRcomb;
            // printf("the keyword to be combined is %s", keylast->lastkey->key);
            combineself(addr, GetterComb);
        }*/
    }
  /*

    */

        printf("-----Emit to combiner-------\n");
}


/**
 * @brief take k/v pairs from the many different combiners 
 * store them in a way that reducers can access them
 * @param key 
 * @param value 
 */
void MR_EmitToReducer(char *key, char *value){
    KeyNode *key_sel;
    key_sel = (KeyNode *)key;

    ValueNode *Vcomb;
    Vcomb = malloc(sizeof(ValueNode));
    ValueNode_init(Vcomb);

    printf("value is %s\n", value);
    Vcomb->value = value;
    key_sel->Vnxt = Vcomb;
    printf("this is emit to reducer: (%s, %s) \n",key_sel->key, key_sel->Vnxt->value);
    
    printf(" -------------all finish----------------------\n");

}


/**
 * @brief hash function 
 * 
 * @param key 
 * @param num_partitions 
 * @return unsigned long 
 */
unsigned long MR_DefaultHashPartition(char *key, int num_partitions){
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)!= '\0'){
        hash = hash *33+c;
    }
    return hash%num_partitions;
}

void * mapper_wrap(){
Mapper map2 = map_paramer->Mapfun;

    map2(map_paramer->arg);

    void *addr;
    addr = keylast->lastkey;

    Combiner combineself;
    combineself = MRcomb;
    printf("the keyword to be combined is %s", keylast->lastkey->key);
    combineself(addr, GetterComb);
}

void *reducer_wrap()
{
   
}
/*
TODO:
 step0: create data sturcture to hold intermediate data
 step1: launch some threads to run map function
 step2: launch reduced threads to proocess intermediate data
*/

/**
 * @brief this is the function called by the map threads
 * we will invoke the mappper here
 * after mapper is done, invoke the combiner here.
 * @param mapper_args 
 * @return void* 
 */



void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Combiner combine,
            Partitioner partition){

pthread_t map_threads[num_mappers];//unsigned long int 


MRcomb = combine;
keylast = malloc(sizeof(keymen));
map_paramer = malloc(sizeof(map_par));
keylast->key = "0000";
keylast->lastkey = NULL;



for (int i=0; i<num_mappers;i++){
    MAPMEM[i] = malloc(sizeof(HashMap));   // the place to save the received data
    HashMap_init(MAPMEM[i]);
}
    //}
    map_paramer->Mapfun = map;

        for (int i = 0; i < num_mappers; i++)
    {
        map_paramer->arg = argv[i+1];
        pthread_create(&map_threads[i], NULL, mapper_wrap, NULL);
    }

    // wait for threads to join;
    for (int i = 0; i < num_mappers ; i++)
    {
        pthread_join(map_threads[i],NULL);
    }

  //  printf("     read from map: %s \n", MAPMEM[0]->hashlist[0]->Knxt->key);
   // printf("     read from value: %s \n", MAPMEM[0]->hashlist[0]->Knxt->Vend->value);

    //printf("read from map: %d \n", MAPMEM[0]->hashlist[0]->Knxt->sum);

    /*
    for (int i = 0; i < num_mappers; i++)
    {
        //  mapper_args = argv[i+1];
        //   pthread_create(&map_threads[i],NULL,mapper_wrapper,argv[i+1]);
        pthread_create(&map_threads[i], NULL, (void *)map, argv[i + 1]);
    }

*/
}