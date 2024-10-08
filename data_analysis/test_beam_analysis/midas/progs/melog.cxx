/********************************************************************\

  Name:         elog.c
  Created by:   Stefan Ritt

  Contents:     Electronic logbook utility   

  $Id:$

\********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#define closesocket(s) close(s)
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

typedef int INT;

#define MAX_ATTACHMENTS  10
#define NAME_LENGTH     100
#define MAX_N_ATTR       20

int verbose;

/*------------------------------------------------------------------*/

const char *map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(char *s, char *d)
{
   unsigned int t, pad;

   pad = 3 - strlen(s) % 3;
   while (*s) {
      t = (*s++) << 16;
      if (*s)
         t |= (*s++) << 8;
      if (*s)
         t |= (*s++) << 0;

      *(d + 3) = map[t & 63];
      t >>= 6;
      *(d + 2) = map[t & 63];
      t >>= 6;
      *(d + 1) = map[t & 63];
      t >>= 6;
      *(d + 0) = map[t & 63];

      d += 4;
   }
   *d = 0;
   while (pad--)
      *(--d) = '=';
}

/*------------------------------------------------------------------*/

void sgets(char *string, int size)
{
   char *p;

   do {
      p = fgets(string, size, stdin);
   } while (p == NULL);

   if (strlen(p) > 0 && p[strlen(p) - 1] == '\n')
      p[strlen(p) - 1] = 0;
}

/*------------------------------------------------------------------*/

char request[600000], response[10000], content[600000];

INT submit_elog(char *host, int port, char *experiment, char *passwd,
                char *uname, char *upwd,
                char attrib_name[MAX_N_ATTR][NAME_LENGTH],
                char attrib[MAX_N_ATTR][NAME_LENGTH],
                int n_attr,
                char *text,
                char afilename[MAX_ATTACHMENTS][256],
                char *buffer[MAX_ATTACHMENTS], INT buffer_size[MAX_ATTACHMENTS])
/********************************************************************\

  Routine: submit_elog

  Purpose: Submit an ELog entry

  Input:
    char   *host            Host name where ELog server runs
    in     port             ELog server port number
    char   *passwd          Write password
    char   *uname           User name
    char   *upwd            User password
    int    run              Run number
    char   *attrib_name     Attribute names
    char   *attrib          Attribute values
    char   *text            Message text

    char   afilename[]      File names of attachments
    char   *buffer[]        Attachment contents
    INT    buffer_size[]    Size of buffer in bytes

  Function value:
    EL_SUCCESS              Successful completion

\********************************************************************/
{
   int status, sock, i, n, header_length, content_length;
   struct hostent *phe;
   struct sockaddr_in bind_addr;
   char host_name[256], boundary[80], str[80], *p;

#if defined( _MSC_VER )
   {
      WSADATA WSAData;

      /* Start windows sockets */
      if (WSAStartup(MAKEWORD(1, 1), &WSAData) != 0)
         return -1;
   }
#endif

   /* get local host name */
   gethostname(host_name, sizeof(host_name));

   phe = gethostbyname(host_name);
   if (phe == NULL) {
      perror("Cannot retrieve host name");
      return -1;
   }
   phe = gethostbyaddr(phe->h_addr, sizeof(int), AF_INET);
   if (phe == NULL) {
      perror("Cannot retrieve host name");
      return -1;
   }

   /* if domain name is not in host name, hope to get it from phe */
   if (strchr(host_name, '.') == NULL)
      strcpy(host_name, phe->h_name);

   /* create socket */
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("cannot create socket");
      return -1;
   }

   /* compose remote address */
   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sin_family = AF_INET;
   bind_addr.sin_addr.s_addr = 0;
   bind_addr.sin_port = htons((unsigned short) port);

   phe = gethostbyname(host);
   if (phe == NULL) {
      perror("cannot get host name");
      return -1;
   }
   memcpy((char *) &(bind_addr.sin_addr), phe->h_addr, phe->h_length);

   /* connect to server */
   status = connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
   if (status != 0) {
      printf("Cannot connect to host %s, port %d\n", host, port);
      return -1;
   }

   if (verbose)
      printf("Successfully connected to host %s, port %d\n", host, port);

   /* compose content */
   strcpy(boundary, "---------------------------7d0bf1a60904bc");
   strcpy(content, boundary);
   strcat(content, "\r\nContent-Disposition: form-data; name=\"cmd\"\r\n\r\nSubmit\r\n");

   if (uname[0])
      snprintf(content + strlen(content), sizeof(content) - strlen(content),
              "%s\r\nContent-Disposition: form-data; name=\"unm\"\r\n\r\n%s\r\n",
              boundary, uname);

   if (upwd[0]) {
      base64_encode(upwd, str);
      snprintf(content + strlen(content), sizeof(content) - strlen(content),
              "%s\r\nContent-Disposition: form-data; name=\"upwd\"\r\n\r\n%s\r\n",
              boundary, str);
   }

   if (experiment[0])
      snprintf(content + strlen(content), sizeof(content) - strlen(content),
              "%s\r\nContent-Disposition: form-data; name=\"exp\"\r\n\r\n%s\r\n",
              boundary, experiment);

   for (i = 0; i < n_attr; i++)
      snprintf(content + strlen(content), sizeof(content) - strlen(content),
              "%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n", boundary,
              attrib_name[i], attrib[i]);

   snprintf(content + strlen(content), sizeof(content) - strlen(content),
           "%s\r\nContent-Disposition: form-data; name=\"Text\"\r\n\r\n%s\r\n%s\r\n",
           boundary, text, boundary);

   content_length = strlen(content);
   p = content + content_length;

   for (i = 0; i < MAX_ATTACHMENTS; i++)
      if (afilename[i][0]) {
         snprintf(p, sizeof(content) - strlen(content),
                 "Content-Disposition: form-data; name=\"attfile%d\"; filename=\"%s\"\r\n\r\n",
                 i + 1, afilename[i]);

         content_length += strlen(p);
         p += strlen(p);
         memcpy(p, buffer[i], buffer_size[i]);
         p += buffer_size[i];
         strcpy(p, boundary);
         strcat(p, "\r\n");

         content_length += buffer_size[i] + strlen(p);
         p += strlen(p);
      }

   /* compose request */
   snprintf(request, sizeof(request), "POST /EL/ HTTP/1.0\r\n");
   snprintf(request + strlen(request), sizeof(request) - strlen(request),
            "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
   snprintf(request + strlen(request), sizeof(request) - strlen(request), "Host: %s\r\n", host_name);
   snprintf(request + strlen(request), sizeof(request) - strlen(request), "User-Agent: ELOG\r\n");
   snprintf(request + strlen(request), sizeof(request) - strlen(request), "Content-Length: %d\r\n", content_length);

   if (passwd[0]) {
      base64_encode(passwd, str);
      snprintf(request + strlen(request),  sizeof(request) - strlen(request), "Cookie: elog_wpwd=%s\r\n", str);
   }

   strcat(request, "\r\n");

   header_length = strlen(request);
   memcpy(request + header_length, content, content_length);

   /*
      {
      FILE *f;
      f = fopen("elog.log", "w");
      fwrite(request, header_length+content_length, 1, f);
      fclose(f);
      }
    */

   /* send request */
   if (verbose) {
      printf("Request sent to host:\n");
      puts(request);
   }

   send(sock, request, header_length + content_length, 0);

   /* receive response */
   i = recv(sock, response, 10000, 0);
   if (i < 0) {
      perror("Cannot receive response");
      return -1;
   }

   /* discard remainder of response */
   n = i;
   while (i > 0) {
      i = recv(sock, response + n, 10000, 0);
      if (i > 0)
         n += i;
   }
   response[n] = 0;

   closesocket(sock);

   if (verbose) {
      printf("Response received:\n");
      puts(response);
   }

   /* check response status */
   if (strstr(response, "302 Found"))
      printf("Message successfully transmitted\n");
   else if (strstr(response, "Logbook Selection"))
      printf("No logbook specified\n\n");
   else if (strstr(response, "Enter password"))
      printf("Missing or invalid password\n");
   else if (strstr(response, "login"))
      printf("Missing or invalid user name/password\n");
   else
      printf("Error transmitting message\n");

   return 1;
}

/*------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
   char str[1000], text[10000], uname[80], upwd[80];
   char host_name[256], logbook[32], textfile[256], password[80];
   char *buffer[MAX_ATTACHMENTS], attachment[MAX_ATTACHMENTS][256];
   INT att_size[MAX_ATTACHMENTS];
   INT i, n, fh, n_att, n_attr, size, port;
   char attr_name[MAX_N_ATTR][NAME_LENGTH], attrib[MAX_N_ATTR][NAME_LENGTH];

   text[0] = textfile[0] = uname[0] = upwd[0] = 0;
   host_name[0] = logbook[0] = password[0] = n_att = n_attr = 0;
   port = 80;
   for (i = 0; i < MAX_ATTACHMENTS; i++) {
      attachment[i][0] = 0;
      buffer[i] = NULL;
      att_size[i] = 0;
   }

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'v')
         verbose = 1;
      else {
         if (argv[i][0] == '-') {
            if (i + 1 >= argc || argv[i + 1][0] == '-')
               goto usage;
            if (argv[i][1] == 'h')
               strcpy(host_name, argv[++i]);
            else if (argv[i][1] == 'p')
               port = atoi(argv[++i]);
            else if (argv[i][1] == 'l')
               strcpy(logbook, argv[++i]);
            else if (argv[i][1] == 'w')
               strcpy(password, argv[++i]);
            else if (argv[i][1] == 'u') {
               strcpy(uname, argv[++i]);
               strcpy(upwd, argv[++i]);
            } else if (argv[i][1] == 'a') {
               strcpy(str, argv[++i]);
               if (strchr(str, '=')) {
                  strcpy(attrib[n_attr], strchr(str, '=') + 1);
                  *strchr(str, '=') = 0;
                  strcpy(attr_name[n_attr], str);
                  n_attr++;
               } else {
                  printf
                      ("Error: Attributes must be supplied in the form \"-a <attribute>=<value>\".\n");
                  return 0;
               }
            } else if (argv[i][1] == 'f')
               strcpy(attachment[n_att++], argv[++i]);
            else if (argv[i][1] == 'm')
               strcpy(textfile, argv[++i]);
            else {
             usage:
               printf("\nusage: elog -h <hostname> [-p port]\n");
               printf("           [-l logbook/experiment]\n");
               printf("           [-v]                     for verbose output\n");
               printf
                   ("           [-w password]            write password defined on server\n");
               printf("           [-u username password]   user name and password\n");
               printf("           [-f <attachment>]        (up to %d times)\n",
                      MAX_ATTACHMENTS);
               printf("           -a <attribute>=<value>   (up to %d times)\n",
                      MAX_N_ATTR);
               printf("           -m <textfile>] | <text>\n");
               printf("\nArguments with blanks must be enclosed in quotes\n");
               printf("The elog message can either be submitted on the command line\n");
               printf
                   ("or in a file with the -m flag. Multiple attributes and attachments\n");
               printf("can be supplied\n");
               return 0;
            }
         } else
            strcpy(text, argv[i]);
      }
   }

   if (host_name[0] == 0 || n_attr == 0 || (text[0] == 0 && textfile[0] == 0))
      goto usage;

  /*---- open text file ----*/

   if (textfile[0]) {
      fh = open(textfile, O_RDONLY | O_BINARY);
      if (fh < 0) {
         printf("Message file \"%s\" does not exist.\n", textfile);
         return 0;
      }

      size = lseek(fh, 0, SEEK_END);
      lseek(fh, 0, SEEK_SET);

      if (size > (int)sizeof(text) - 1) {
         printf("Message file \"%s\" is too long (%d bytes max).\n", textfile,
                (int)sizeof(text));
         return 0;
      }

      memset(text, 0, sizeof(text));
      i = read(fh, text, size);
      if (i < size) {
         printf("Cannot fully read message file \"%s\".\n", textfile);
         return 0;
      }

      close(fh);
   }

  /*---- open attachment file ----*/

   for (i = 0; i < MAX_ATTACHMENTS; i++) {
      if (!attachment[i][0])
         break;

      fh = open(attachment[i], O_RDONLY | O_BINARY);
      if (fh < 0) {
         printf("Attachment file \"%s\" does not exist.\n", attachment[i]);
         return 0;
      }

      att_size[i] = lseek(fh, 0, SEEK_END);
      lseek(fh, 0, SEEK_SET);

      buffer[i] = (char*)malloc(att_size[i] + 1);
      if (buffer[i] == NULL || att_size[i] > 500 * 1024) {
         printf("Attachment file \"%s\" is too long (500k max).\n", attachment[i]);
         return 0;
      }

      n = read(fh, buffer[i], att_size[i]);
      if (n < att_size[i]) {
         printf("Cannot fully read attachment file \"%s\".\n", attachment[i]);
         return 0;
      }
      buffer[i][n] = 0;

      close(fh);
   }

   /* now submit message */
   submit_elog(host_name, port, logbook, password,
               uname, upwd,
               attr_name, attrib, n_attr, text, attachment, buffer, att_size);

   for (i = 0; i < MAX_ATTACHMENTS; i++)
      if (buffer[i])
         free(buffer[i]);

   return 1;
}
