/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <dirent.h>
#include <strings.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void parseHeader(char* header, int sockfd)
{
   // set up space for header line data
   char ret[2048]; 
   int retIndex = 0;
   memset(ret, 0, 2048); 

   int x;
   int startIndex = -1;
   int endIndex = -1;
   int fileIndex = 0;
   char filename[2048];
   memset(filename, 0, 2048);
   
   char extension[10];
   int extend = -1;
   int extIndex = 0;
   memset(extension, 0, 10);

   // get request line
   int endOfRequestLine;
   for (x = 0; x < strlen(header); x++)
   {
      if (header[x] == '\r' && header[x+1] == '\n') 
      {  
         endOfRequestLine = x;
         break;
      }
      if (header[x] == '/' && startIndex == -1) startIndex = x + 1;
      if (header[x] == '.' && extend == -1) extend = x + 1;
      else if (header[x] == '/') endIndex = x - 5; 
   }

   // store file extension
   for (x = extend; x < endIndex; x++)
   { 
      extension[extIndex] = header[x];
      extIndex++;  
   }

   //store file name
   for (x = startIndex; x < endIndex; x++)
   {  
      if (header[x] == '%')
      {
         if (strlen(header) != x + 1 && header[x + 1] == '2')
         {
            if (strlen(header) != x + 2 && header[x + 2] == '0')
            {
                x = x + 2;
                filename[fileIndex] = ' ';  
            }
         }
      }
      else 
      {
         filename[fileIndex] = header[x];
      }
      fileIndex++;
   }   
    
   // get HTTP/#.#
   for (x = endIndex + 1; x < endOfRequestLine; x++)
   {   
      ret[retIndex] = header[x];
      retIndex++;
   } 
   
   // look for the file 
   DIR* dir;
   struct dirent* entry;
   dir = opendir(".");
   char statusCode[50];
   memset(statusCode, 0, 50);
   long int total_size;
   char* final_output;
   while((entry = readdir(dir)) != NULL) //NULL if no more entries
   {
      int s = strcasecmp(entry->d_name, filename);
      if (s == 0)
      {
        strcpy(statusCode, " 200 OK\r\n");
        strcat(ret, statusCode);

        // Found the file, attach message body
        FILE *fp;

        fp = fopen(entry->d_name, "rb");

        fseek(fp, 0, SEEK_END);
        long int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *requested_file;
        requested_file = (char *) malloc(size);

        int j = fread(requested_file, size, 1, fp);
        if (j < 0)
        {
           error("ERROR reading from file");
           exit(1);
        }

        // set up Content-Length header line
        char content_len[200];
        memset(content_len, 0, 200);
        sprintf(content_len, "Content-Length: %ld\r\nX-Content-Type-Options: nosniff\r\n", size);
            
        // set up Content-Type header line
        char content_type[200];
        memset(content_type, 0, 200);
        sprintf(content_type, "Content-Type: ");     
        if (strcasecmp(extension, "txt") == 0)
        {
           strcat(content_type, "text/plain\r\n");
        }
        else if (strcasecmp(extension, "jpeg") == 0 || strcasecmp(extension, "jpg") == 0)
        {
           strcat(content_type, "image/jpeg\r\n");
        }
        else if (strcasecmp(extension, "gif") == 0)
        {
           strcat(content_type, "image/gif\r\n");
        }
        else if (strcasecmp(extension, "htm") == 0 || strcasecmp(extension, "html") == 0 )
        {
           strcat(content_type, "text/html\r\n");
        }
        else 
        {
           strcat(content_type, "application/octet-stream\r\n");
        }
    
        int extralen = strlen(ret) + strlen(content_len) + strlen("\r\n") + strlen(content_type);
        total_size = extralen + size;
        final_output = (char *) malloc(total_size);

        // copy everything to the newly allocated memory
        strcpy(final_output, ret);
        strcat(final_output, content_len);
        strcat(final_output, content_type);
        strcat(final_output, "\r\n");

        // copy the file to the final output string
        // must be copied byte by byte because some image may have '\0' in the middle of the file
        long int x;
        for (x = extralen; x < total_size; x++)
        {
           final_output[x] = requested_file[x - extralen];
        }
      }    
   }
   // if the file is not found
   if (strlen(statusCode) == 0)
   {
      strcpy(statusCode, " 404 Not Found\r\n");
      strcat(ret, statusCode);
      char* notfound = "Content-Length: 309\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML><html><head><meta http-equiv='Content-Type' content='text/html;charset=utf-8'><title>Error response</title></head><body><h1>Error response</h1><p>Error code: 404</p><p>Message: File not found.</p><p>Error code explanation: HTTPStatus.NOT_FOUND - Nothing matches the given URI.</p></body></html>";
      total_size = strlen(ret) + strlen(notfound);
      final_output = (char *) malloc(total_size);
      strcpy(final_output, ret);
      strcat(final_output, notfound);
   }

   // send the HTTP response to the client
   int j = write(sockfd, final_output, total_size);
   if (j < 0)
   {
      error("ERROR writing HTTP response");
      exit(1);
   }
   free(final_output);
   closedir(dir);
}

// allow the client to make multiple requests
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    while(1)
    {
       listen(sockfd, 5);  // 5 simultaneous connection at most

       //accept connections
       newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

       if (newsockfd < 0)
          error("ERROR on accept");

       int n;
       char buffer[2048];

       memset(buffer, 0, 2048);  // reset memory
    
       //read client's message
       n = read(newsockfd, buffer, 2048);
       if (n < 0) 
       {
          error("ERROR reading from socket");
          exit(1);
       }

       printf("%s\n", buffer);

       //send response to browser
       parseHeader(buffer, newsockfd);
       close(newsockfd);
    }
         
    close(sockfd);

    return 0;
}
