/*
 * author: Hao Qin
 * email: hjq5024@psu.edu
 * parse_url.c for CMPSC 311 Fall 2019
 * last updated: 12/3/2019
 */
#include <parse_url.h>
#include <strings.h>
#include <stdio.h>
// returns 1 if the url starts with http:// (case insensitive)
// and returns 0 otherwise
// If the url is a proper http scheme url, parse out the three
// components of the url: host, port and path and copy
// the parsed strings to the supplied destinations.
// Remember to terminate your strings with 0, the null terminator.
// If the port is missing in the url, copy "80" to port.
// If the path is missing in the url, copy "/" to path.
int parse_url(const char *url, char *host, char *port, char *path) {
  char s1[] = "http://";
  const char *p1 = (char *)s1;
  //compare first 7 characters from url to see if it is "http://"
  //if url not vaild return 0
  if (strncasecmp(p1,url,7)){
    return 0;
  }
  //if it is valid url 
  else{
    int i;
    for (i=8;i<=strlen(url);i++){
      //find the first "/" or ":", add host
      if((strncasecmp(&url[i],"/",1)==0) || (strncasecmp(&url[i],":",1)==0)){
        strncpy(host,&url[7],i-7);
        strncpy(&host[i-7],"\0",1);
        //if it is "/", use default port, add path
        if (strncasecmp(&url[i],"/",1)==0){
          strncpy(path,&url[i],strlen(url)-i);
          strncpy(&path[strlen(url)-i],"\0",1);
          strncpy(port,"80\0",3);
        }
        //if it is ":" find next "/"
        else{
          int c;
          for (c=i+1;c<=strlen(url);c++){
            //c is the position of next "/"
            //if we found "/" after ":", add port use url position between "/" and ":", add path after "/"
            if((strncasecmp(&url[c],"/",1)==0)){
              strncpy(port,&url[i+1],c-i-1);
              strncpy(&port[c-i-1],"\0",1);
              strncpy(path,&url[c],strlen(url)-c);
              strncpy(&path[strlen(url)-c],"\0",1);
              break;
            }
          }
          //if no "/" is found after ":", use default path, add port after ":"
          if (c==(strlen(url)+1)){
            strncpy(port,&url[i+1],strlen(url)-i);
            strncpy(&port[strlen(url)-i],"\0",1);
            strncpy(path,"/\0",2);
          }
        }
        break;
      }
    }
    //if no "/" or ":" found in url, use default port and path, add host after "http://"
    if (i==strlen(url)+1){
      strncpy(port,"80\0",3);
      strncpy(path,"/\0",2);
      strncpy(host,&url[7],strlen(url)-7);
      strncpy(&host[strlen(url)-7],"\0",1);
    }
    return 1;
  }
}
