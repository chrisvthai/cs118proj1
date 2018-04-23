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
   char ret[2048]; //add more if necessary, up to 800,000,000
   int retIndex = 0;
   memset(ret, 0, 2048); 

   int x;
   int startIndex = -1;
   int endIndex = -1;
   char filename[2048];
   int fileIndex = 0;
   memset(filename, 0, 2048);
   
   char extension[10];
   int extIndex = 0;
   memset(extension, 0, 10);

   //get request line
   int endOfRequestLine;
   for (x = 0; x < strlen(header); x++)
   {
      if (header[x] == '\r' && header[x+1] == '\n') 
      {  
         endOfRequestLine = x;
         break;
      }
      if (header[x] == '/' && startIndex == -1) startIndex = x + 1;
      else if (header[x] == '/') endIndex = x - 5; 
   }

   //fprintf(stdout, "%d\n", startIndex);

   //store file name
   if (startIndex == endIndex) return;
   for (x = startIndex; x < endIndex; x++)
   {  
      //printf("%c\n", header[x]);
      filename[fileIndex] = header[x];
      fileIndex++;
   }   
    
   //get HTTP/1.1
   for (x = endIndex + 1; x < endOfRequestLine; x++)
   {   
      ret[retIndex] = header[x];
      retIndex++;
   } 
   
   //look for the file 
   DIR* dir;
   struct dirent* entry;
   dir = opendir(".");
   char statusCode[50];
   memset(statusCode, 0, 50);
   while((entry = readdir(dir)) != NULL) //NULL if no more entries
   {
      int s = strcasecmp(entry->d_name, filename);
      //printf("%s\n", entry->d_name);
      //printf("%s\n", filename);
      //printf("%d\n", s);
      if (s == 0)
      {
         strcpy(statusCode, " 200 OK\r\n");
	 strcat(ret, statusCode);

	 //Found the file, attach message body
	  FILE *fp;

        fp = fopen(entry->d_name, "rb");

        fseek(fp, 0, SEEK_END);
        long int size = ftell(fp) + 1;
        fseek(fp, 0, SEEK_SET);

        printf("File size is: %d\n", size);

        char *requested_file;
        requested_file = (char *) malloc(size);

        for (int i = 0; i < size; i++)
                fread(requested_file, size, 1, fp);

        printf("%s", requested_file);
         break;
      }    
   }
   if (strlen(statusCode) == 0)
   {
      strcpy(statusCode, " 404 Not Found\r\n");
      strcat(ret, statusCode);
   }
   closedir(dir);
   printf("%s\n", ret);
}

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
