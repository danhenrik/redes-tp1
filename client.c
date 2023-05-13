#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SZ 1024
#define MAX_MSG_SZ 500

void usage(int argc, char **argv)
{
  printf("usage: %s <server IP> <server Port>\n", argv[0]);
  printf("example: %s 127.0.0.1 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

void send_message(int sock, char *msg)
{
  // send message
  int count = send(sock, msg, strlen(msg), 0);
  if (count != strlen(msg))
    log_error("on send");
  printf("[conn] sent %d bytes\n", count);

  // waits for the ACK response
  memset(msg, 0, MAX_MSG_SZ);

  count = recv(sock, msg, MAX_MSG_SZ, 0);

  printf("[conn] received %d bytes\n", count);
}

int main(int argc, char **argv)
{
  if (argc == 0)
    usage(argc, argv);

  struct sockaddr_storage storage;
  if (addrparse(argv[1], argv[2], &storage) != 0)
    usage(argc, argv);

  // create socket and connect to server(by default we're using IPV4)
  int sock;
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    log_error("on socket creation");
  struct sockaddr *addr = (struct sockaddr *)(&storage);

  if (connect(sock, addr, sizeof(storage)) != 0)
    log_error("on connect");

  char addrstr[BUF_SZ];
  addrtostr(addr, addrstr, BUF_SZ);

  printf("[conn] successfully connected to %s\n", addrstr);

  char select_command[] = "select file ";
  char send_command[] = "send file";
  char exit_command[] = "exit";

  FILE *fp = NULL;

  // user menu
  char input[BUF_SZ];
  int header_sz = MAX_MSG_SZ;
  char header[header_sz];
  memset(header, 0, MAX_MSG_SZ);
  char endFlag[] = "\\end";
  int endFlag_sz = strlen(endFlag) -1;
  while (1)
  {
    memset(input, 0, BUF_SZ);
    printf("Enter your command\n");
    fgets(input, BUF_SZ - 1, stdin);
    input[strcspn(input, "\n")] = 0;
    // Select File CMD
    if (strncmp(select_command, input, strlen(select_command)) == 0)
    {
      char *filename = strrchr(input, ' ');

      filename = filename + 1;
      // # Validations
      if (!filename)
      {
        printf("no file selected\n");
        continue;
      }

      // Validate if file extensions is acceptable
      int formats_supported = 6;
      char allowedExts[][6] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
      int invalid = 1;
      char *ext = strrchr(input, '.');
      if (ext != NULL)
      {
        for (int i = 0; i < formats_supported; i++)
        {
          if (strcmp(ext, allowedExts[i]) == 0)
          {
            invalid = 0;
            break;
          }
        }
      }
      if (invalid == 1)
      {
        printf("%s not valid!\n", filename);
        continue;
      }

      fp = fopen(filename, "r");

      if (fp == NULL) // file not found
      {
        printf("%s does not exist\n", filename);
        continue;
      }

      sprintf(header, "%s ", filename);
      header_sz = strlen(header);
      printf("%s selected\n", filename);
    }
    // Send File CMD
    else if (strncmp(send_command, input, strlen(send_command)) == 0)
    {
      if (fp == NULL)
      {
        printf("no file selected!\n");
        continue;
      }
      while (1)
      {
        int payload_sz = MAX_MSG_SZ - header_sz - endFlag_sz - 1;
        char payload[payload_sz];
        char msg[MAX_MSG_SZ];

        int read_sz = fread(payload, sizeof(payload[0]), payload_sz, fp);

        sprintf(msg, "%s %s%s", header, payload, endFlag);
        send_message(sock, msg);

        if (read_sz < payload_sz)
        {
          break;
        }
      }
      fclose(fp);
    }
    else if (strncmp(exit_command, input, strlen(exit_command)) == 0)
    {
      // not sure if just close connection or send a message requiring so
      printf("Exit\n");
      close(sock);
      break;
    }
  }
  exit(EXIT_SUCCESS);
}
