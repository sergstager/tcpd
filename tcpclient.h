#ifndef _TCPCLIENT_H_
#define _TCPCLIENT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

char filename[1024];
char remote_address[16];
char remote_fqdn[1024];
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
#define ER_CONFIG_NOPORT		5	// не указан порт
#define ER_CONFIG_FILENAME		6	// не указан файл
#define ER_CONFIG_NOADDRESS		7	// не указан ни один адрес
#define ER_CONFIG_TOOMANYADDRESSES	8	// указано два адреса

char str_errors[9][1024]= {
  "all OK",
  "Can't open file",
  "Can't create socket",
  "Can't connect to remote host",
  "Error on send file", // 5
  "You must specify remote port",
  "You must specify filename",
  "You must specify at least one address",
  "You must specify only one address"
};



///////////////////////////////
//
//  вывод хелпа
//
void printUsage(char *argv[]) {
  printf("Usage: %s [-h] {-i remote_ip_address|-a remote_fqdn_address} -p remote_port -f filename\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tshow this help\n");
  printf("\t-i\tremote ip address\n");
  printf("\t-a\tremote fqdn address\n");
  printf("\t-p\tremote port\n");
  printf("\t-f\tfilename that sends\n\n");
}

///////////////////////////////
//
//  функция сообщения об ошибке
//
void message(int error) {
  if (sock_id) {
    shutdown(sock_id, SHUT_RDWR);
    close(sock_id);
   }
  if (file_id) {
    fclose(file_id);
   }
  if (error) {
    syslog(LOG_WARNING, "error %d: %s", error, str_errors[error]);
    printf("%s\n", str_errors[error]);
//    printf("%s (%d: %s)\n", str_errors[error], errno, strerror(errno));
   }
  sleep(3);
}

///////////////////////////////
//
//  функция завершения демона
//
void quit(int error) {
  message(error);
  exit(error);
}

#endif // _TCPCLIENT_H_
