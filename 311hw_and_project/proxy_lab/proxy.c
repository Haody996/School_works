#include <stdio.h>
#include "cache.h"
#include "csapp.h"
#include <string.h>
#define MAX_URL 400
#define REQUEST_HEADER 1200

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

CacheList *list;

void doit(int);

char *recv_request(int connfd);
char *get_request(char *, char *, char *, char *);

/*main funtion to get everything going*/
int main(int argc, char const *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s [port]\n", argv[0]);
    return 1;
  }

  char listen_port[100];
  strcpy(listen_port, argv[1]);
  list = (CacheList *)Malloc(sizeof(CacheList));
  cache_init(list);
  char hostname[MAXLINE + 1];
  char port[MAXLINE + 1];
  struct sockaddr_in clientaddr;
  Signal(SIGPIPE, SIG_IGN);
  int listenfd = Open_listenfd(listen_port);
  while (1)
  {
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection form (%s, %s)\n", hostname, port);
    doit(connfd);
    close(connfd);
  }

  return 0;
}

// extract content-length from http header
// return -1 if statue code is not 200
// or no content-length field
int extract_length(const char *header)
{

  char temp[100];
  int code;
  // check status code
  if (sscanf(header, "%s %d", temp, &code) != 2 || code != 200)
  {
    printf("code %d\n", code);
    return -1;
  }

  size_t i = 0;
  size_t n = strlen(header);

  while (i < n - strlen("Content-length:") && strncasecmp(header + i, "Content-length:", strlen("Content-length:")) != 0)
  {
    i++;
  }

  if (i >= n - strlen("Content-length:"))
  {
    return -1;
  }
  int length = 0;
  if (sscanf(header + i, "%s %d", temp, &length) == 2)
  {
    return length;
  }

  return -1;
}

void doit(int connfd)
{
  char *request = recv_request(connfd);
  char *header = (char *)Malloc(REQUEST_HEADER);
  char *port = (char *)Malloc(20);
  char *subdir = (char *)Malloc(MAX_URL);
  char *host = get_request(request, header, port, subdir);
  //if there is no port
  if (strlen(port) == 0)
  {
    strcpy(port, "80");
  }
  char *url = (char *)Malloc(REQUEST_HEADER);
  //print url
  sprintf(url, "%s:%s%s", host, port, subdir);
  CachedItem *cache = find(url, list);

  int n;
  if (cache) //found the cache, send from cache
  {
    n = send(connfd, cache->headers, strlen(cache->headers), 0);
    n = send(connfd, cache->item_p, cache->size, 0);

    Free(header);
    Free(port);
    Free(host);
    Free(subdir);
    Free(url);
    printf("find url: %p %s \n", cache->headers, cache->url);
    return;
  }

  //open client fd
  int clientfd = Open_clientfd(host, port);
  if (clientfd <= 0)
  {
    perror("connect");
    Free(header);
    Free(port);
    Free(host);
    Free(subdir);
    Free(url);
    return;
  }

  n = send(clientfd, header, strlen(header) + 1, 0);
  if (n < 0)
  {
    perror("Send");
    Free(header);
    Free(port);
    Free(host);
    Free(subdir);
    Free(url);
    return;
  }

  char buffer[MAXBUF + 1];
  char *response_header = NULL;
  void *response_item = NULL;

  int length = -2;
  int copied_length = 0;

  // recv data from the server and send to the client
  do
  {
    n = recv(clientfd, buffer, MAXBUF, 0);
    if (n <= 0)
    {
      break;
    }

    n = send(connfd, buffer, n, 0);

    // check the postion of header

    if (length == -2)
    { // the first one
      size_t i = 0;
      while (i <= n - 4 && strncmp(buffer + i, "\r\n\r\n", 4) != 0)
      {
        i++;
      }

      if (i <= n - 4)
      {
        response_header = (char *)Malloc(i + 5);
        strncpy(response_header, buffer, i + 4);
        response_header[i + 4] = '\0';
        i += 4;
        // find filed Content-Length
        length = extract_length(response_header);

        if (length > 0 && length <= MAX_OBJECT_SIZE)
        {
          response_item = Malloc(length);
          memcpy(response_item, buffer + i, n - i);
          copied_length = n - i;
        }
        else
        {
          length = -1;
        }
      }
    }
    else if (length > 0)
    {
      memcpy(response_item + copied_length, buffer, n);
      copied_length += n;
    }

  } while (n > 0);

  // insert url to caches
  if (length > 0 && copied_length == length)
  {
    cache_URL(url, response_header, response_item, length, list);
    printf("insert url: %p %s \n", response_header, url);
    Free(response_header);
  }
  else
  {
    Free(response_header);
    Free(response_item);
  }

  Free(header);
  Free(port);
  Free(host);
  Free(subdir);
  Free(url);
  return;
}

/**
 * recv http request from the user
 * return the http header
 * */
char *recv_request(int connfd)
{
  char *buffer = (char *)Malloc(MAXLINE + 1);
  int n = recv(connfd, buffer, MAXLINE - 1, 0);
  if (n <= 0)
  {
    close(connfd);
    return NULL;
  }
  buffer[n - 1] = '\0';
  printf("%s\n", buffer);

  return buffer;
}

// find subdir or host name in the GET Line
int find_subdir(const char *line, char *subdir, char *host, char *port)
{
  size_t i = 0;
  size_t j, k;
  size_t n = strlen(line);
  while (i < strlen(line) && line[i] == ' ')
  {
    i++;
  }

  j = i;
  if (strncmp(line + i, "http", 4) == 0 || strncmp(line + i, "HTTP", 4) == 0) // contain host
  {
    while (j < n && strncmp(line + j, "://", 3) != 0)
      j++;
    if (j == n)
    {
      return -1; // invalid value
    }
    k = 0;
    j += 3;
    while (j < n && line[j] != '/' && line[j] != ' ' && line[j] != ':')
    {
      host[k++] = line[j++];
    }
    host[k] = '\0';
    if (j >= n)
    {
      return -1; // invalid value
    }
    if (line[j] == ':')
    {
      j++;
      k = 0;
      while (j < n && line[j] != '/' && line[j] != ' ')
      {
        port[k++] = line[j++];
      }
      port[k] = '\0';
    }
    else
    {
      port[0] = '\0';
    }
  }
  k = 0;
  while (j < n && line[j] != ' ')
  {
    subdir[k++] = line[j++];
  }

  if (j == n)
  {
    return -1; // invalid value
  }
  subdir[k] = '\0';
  if (k == 0)
  {
    subdir[0] = '/';
    subdir[1] = '\0';
  }
  return 1;
}

// retrive the request from the remote server.
char *get_request(char *request, char *header, char *port, char *subdir)
{

  char *host = (char *)Malloc(MAX_URL);

  // get HOST or url
  char *temp;
  temp = strtok(request, "\r\n");

  while (temp != NULL)
  {
    printf("%s \n", temp);

    if (strncmp(temp, "GET ", 4) == 0)
    {
      find_subdir(temp + 4, subdir, host, port);
      sprintf(header, "GET %s HTTP/1.1\r\nHost: %s\r\n", subdir, host);
      temp = strtok(NULL, "\r\n");
      continue;
    }

    if (strncmp(temp, "Host: ", 6) == 0)
    {
      strcpy(host, temp + 6);
      size_t i = 0;
      while (i < strlen(host) && host[i] != ':')
      {
        i++;
      }

      if (i < strlen(host) && host[i] == ':')
      {
        host[i] = '\0';
      }

      sprintf(header, "GET %s HTTP/1.1\r\nHost: %s\r\n", subdir, host);
      temp = strtok(NULL, "\r\n");
      continue;
    }
    //get rid of unkown
    if (strncmp(temp, "If-Modified-Since", strlen("If-Modified-Since")) == 0 || strncmp(temp, "If-None-Match", strlen("If-None-Match")) == 0 || strncmp(temp, "User-Agent:", strlen("User-Agent:")) == 0 || strncmp(temp, "Connection:", strlen("Connection:")) == 0 || strncmp(temp, "Proxy-Connection:", strlen("Proxy-Connection:")) == 0)
    {
      temp = strtok(NULL, "\r\n");
      continue;
    }
    temp = strtok(NULL, "\r\n");
  }

  //append rest of the headers
  strcat(header, "Connection: close\r\n");
  strcat(header, user_agent_hdr);
  strcat(header, "Proxy-Connection: close\r\n\r\n");
  printf("http header:\n %s \n", header);
  Free(request);
  return host;
}
