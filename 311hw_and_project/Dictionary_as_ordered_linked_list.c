/*
 * author: Hao Qin
 * email: hjq5024@psu.edu
 * dict.c for CMPSC 311 Fall 2019
 * last updated: 10/15/2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"

// count number of keys in a dict.
int countKeys(const dictNode *dict) {
  //while loop to go through the entirty of the dictionary to count the number of keys in the dictionary
  int count = 0;
  while (dict){
  dict=dict->next;
  count++;
  }
  return count;
  ;
}

// given a key, look up its corresponding value in the
// dictionary, returns -1 if the value is not in the dictionary.
// your search for key should end when the key in the next node
// is bigger than the lookup key or you reached the end of the
// list.
int lookupKey(const dictNode *dict, const char *key) {
  //while pointer is not null, compare all the keys in the dictionary to the given key
  while (dict){
    if (strcmp(dict->key,key)==0){
      return dict->value;
    }
    //if the key in the next node is bigger than the lookup key
    if (strcmp(dict->key,key)>0) {
    return -1;
    }
    dict=dict->next;
  }
  //reached the end of dictionary
  return -1;
}
// delete the node in a dict with given key, return the value of
// the deleted node if it was found, return -1 if it wasn't found.
int deleteKey(dictNode **dict, const char *key) {
  dictNode *prev=NULL;
  dictNode *temp=NULL;
  dictNode *first=NULL;
  while(*dict){
    //keep the first node's pointer
    if (prev == NULL){
      first=(*dict);
    }
    //find the matching key
    if (strcmp((*dict)->key,key)==0){
      //the first node is the wanted node
      if (prev == NULL){
        //only one node in dictionary
        if ((*dict)->next == NULL){
          freeNode(*dict);
          return (*dict)->value;;
        }
        //the first node but more than one node in dict
        else{
        temp=(*dict);
        (*dict)=(*dict)->next;
        freeNode(temp);
        return (temp)->value;;
        }
      }
      //not the first node 
      else
      {
        //not the last node
        if ((*dict)->next!=NULL){
        temp= (*dict);
        prev->next=(*dict)->next;
        freeNode(temp);
        *dict=first;
        return (temp)->value;
        }
        //last node
        else{
          temp = (*dict);
          prev->next=NULL;
          freeNode(*dict);
          *dict=first;
          return (temp)->value;
        }
      }  
    }
    // not in the dictionary
    if (strcmp((*dict)->key,key)>0) {
      *dict=first;
      return -1;
    }
    prev=*dict;
    *dict=(*dict)->next;
  }
  //reached the end of dictionary
  return -1;

}

// given a key/value pair, first lookup the key in the dictionary,
// if it is already there, update the dictionary with the new
// value; if it is not in the dictionary, insert a new node into
// the dictionary, still make sure that the key is in alphabetical
// order.
// IMPORTANT: When creating a new node, make sure you dynamically
// allocate memory to store a copy of the key in the memory. You
// may use strdup function. DO NOT STORE the input key from the 
// argument directly into the node. There is no guarantee that key
// pointer's value will stay the same. 
// YOU MUST KEEP THE ALPHABETICAL ORDER OF THE KEY in the dictionary.
void addKey(dictNode **dict, const char *key, int value) {
  dictNode *first=NULL;
  dictNode *prev=NULL;
  //if dictionary is null create new node
  if ((*dict)==NULL){
    dictNode *new = (dictNode*)malloc(sizeof(dictNode));
    new->key=strdup(key);
    new->value=value;
    *dict=new;
    return;
  }
  //if dictionary exists
  else{
    //keep the first node's pointer
    while (*dict){
      if (prev == NULL){
        first=(*dict);
      }
      //replace the value of key is found
      if (strcmp((*dict)->key,key)==0){
        (*dict)->value = value;
        (*dict)=first;
        return;
      }
      if (strcmp((*dict)->key,key)>0) 
      //add not the last node
      {
        //if it is to be the first node in the dictionary
        if (prev==NULL){
          dictNode *new = (dictNode*)malloc(sizeof(dictNode));
          new->key=strdup(key);
          new->value=value;
          new->next=(*dict);
          (*dict)=new;
          return;
        }
        //it is to be added in the middle of the dictionary
        else{
          dictNode *new = (dictNode*)malloc(sizeof(dictNode));
          new->key=strdup(key);
          new->value=value;
          new->next=(*dict);
          prev->next=new;
          (*dict)=first;
          return;
        }
      }
      prev=*dict;
      *dict=(*dict)->next;
    }
    //add at the last node of the dictionary
    dictNode *new = (dictNode*)malloc(sizeof(dictNode));
    new->key=strdup(key);
    new->value=value;
    new->next=NULL;
    prev->next=new;
    (*dict)=first;
    return;
  }
  return;
}
