#ifndef _TCPCLIENT_H_
#define _TCPCLIENT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

char filename[1024];
char remote_address[16];
int remote_port;

int sock_id;
FILE *file_id;

//////////////////////////////
//
//  коды ошибок
//
#define ER_CANT_OPENFILE		1	// не открывается файл
#define ER_CANT_CREATE_SOCKET		2	// не создаётся сокет
#define ER_CANT_CONNECT			3	// не конектится
#define ER_ERROR_ON_SEND		4	// ошибка при передаче

char str_errors[5][1024]= {
  "all OK",
  "Cant open file",
  "Cant create socket",
  "Cant connect to remote host",
  "Error on send file"
};



///////////////////////////////
//
//  вывод хелпа
//
void printUsage(char *argv[]) {
  printf("Usage: %s [-h] -a remote_address -p remote_port -f filename\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tshow this help\n");
  printf("\t-a\tremote ip address\n");
  printf("\t-p\tremote port\n");
  printf("\t-f\tfilename that sends\n\n");
}

///////////////////////////////
//
//  функция завершения демона
//
void quit(int error) {
  if (sock_id) {
    shutdown(sock_id, SHUT_RDWR);
    close(sock_id);
   }
  if (file_id) {
    fclose(file_id);
   }
  if (error) {
    syslog(LOG_WARNING, "error %d: %s", error, str_errors[error]);
    printf("%s (%d: %s)\n", str_errors[error], errno, strerror(errno));
   }
  exit(error);
}

#endif // _TCPCLIENT_H_
