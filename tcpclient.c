#include "tcpclient.h"

//////////////////////////////////
//
//  начало
//
int main(int argc, char *argv[]) {
  char buf[2048];

  // инициализируем переменные
  memset(&filename, 0, sizeof(filename));
  memset(&remote_address, 0, sizeof(remote_address));
  remote_port = 0;

  // парсим опции командной строки
  char opt;
  while ((opt = getopt(argc, argv, "ha:p:f:")) != -1) {
    switch (opt) {
      case 'a':
        if (optarg) strncpy(remote_address, optarg, sizeof(remote_address)-1);
        break;
      case 'p':
        if (optarg) strncpy(buf, optarg, sizeof(buf)-1);
        remote_port = atoi(buf);
        break;
      case 'f':
        if (optarg) strncpy(filename, optarg, sizeof(filename)-1);
        break;
      default:
        printUsage(argv);
        quit(0);
        break;
     }
   }

  // открываем файл
  if (! (file_id = fopen(filename, "r")) ) quit(ER_CANT_OPENFILE);

  // открываем сокет
  struct sockaddr_in addr;
  sock_id = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sock_id) quit(ER_CANT_CREATE_SOCKET);

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(remote_address);
  addr.sin_port = htons(remote_port);
  if (connect(sock_id, (struct sockaddr *)&addr, sizeof(addr))) quit(ER_CANT_CONNECT);

  // шлём файл
  while (!feof(file_id)) {
    int cntr = fread(&buf, 1, sizeof(buf), file_id);
    if ( -1 == send(sock_id, &buf, cntr, 0)) quit(ER_ERROR_ON_SEND);
   }

  // закрываем сокет и файл
  shutdown(sock_id, SHUT_RDWR);
  close(sock_id);
  fclose(file_id);

  // выход
  return 0;
}
