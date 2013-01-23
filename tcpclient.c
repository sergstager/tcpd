#include "tcpclient.h"

//////////////////////////////////
//
//  начало
//
int main(int argc, char *argv[]) {
  char buf[2048];
  int fl_status = 0;

  // инициализируем переменные
  int fl_filename = 0;
  memset(&filename, 0, sizeof(filename));
  int fl_address = 0;
  memset(&remote_address, 0, sizeof(remote_address));
  int fl_fqdn = 0;
  memset(&remote_fqdn, 0, sizeof(remote_fqdn));
  remote_port = 0;

  // парсим опции командной строки
  char opt;
  while ((opt = getopt(argc, argv, "hi:a:p:f:")) != -1) {
    switch (opt) {
      case 'i':
        if (optarg) strncpy(remote_address, optarg, sizeof(remote_address)-1);
        fl_address = 1;
        break;
      case 'a':
        if (optarg) strncpy(remote_fqdn, optarg, sizeof(remote_fqdn)-1);
        fl_fqdn = 1;
        break;
      case 'p':
        if (optarg) strncpy(buf, optarg, sizeof(buf)-1);
        remote_port = atoi(buf);
        break;
      case 'f':
        if (optarg) strncpy(filename, optarg, sizeof(filename)-1);
        fl_filename = 1;
        break;
      default:
        printUsage(argv);
        quit(0);
        break;
     }
   }

  // проверяем опции
  if (0 == remote_port) quit(ER_CONFIG_NOPORT);
  if (0 == fl_filename) quit(ER_CONFIG_FILENAME);
  if ((0 == fl_address)&&(0 == fl_fqdn)) quit(ER_CONFIG_NOADDRESS);
  if ((1 == fl_address)&&(1 == fl_fqdn)) quit(ER_CONFIG_TOOMANYADDRESSES);

// в бесконечном цикле пытаемся отправить файл
  do {
    fl_status = 0;
    // открываем файл
    if (! (file_id = fopen(filename, "r")) ) {
      message(ER_CANT_OPENFILE);
      continue;
     }

    // открываем сокет
    sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock_id) {
      message(ER_CANT_CREATE_SOCKET);
      continue;
     }

    // по ип или
    if (1 == fl_address) {
      struct sockaddr_in addr;
      addr.sin_port = htons(remote_port);
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(remote_address);
      if (connect(sock_id, (struct sockaddr *)&addr, sizeof(addr))) {
        message(ER_CANT_CONNECT);
        continue;
       }
     }
    // по имени
    if (1 == fl_fqdn) {
      int status;
      char sport[16];
      struct addrinfo hints;
      struct addrinfo *report;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = AI_PASSIVE;
      snprintf(sport, sizeof(sport), "%d", remote_port);
      if (0 != (status = getaddrinfo(remote_fqdn, sport, &hints, &report)) ) {
        message(ER_CANT_CONNECT);
        continue;
       }
      if (connect(sock_id, report->ai_addr, report->ai_addrlen)) {
        message(ER_CANT_CONNECT);
        continue;
       }
      freeaddrinfo(report);
     }

    // шлём файл
    fl_status = 1;
    while (!feof(file_id)||(0 == fl_status)) {
      int cntr = fread(&buf, 1, sizeof(buf), file_id);
      if ( -1 == send(sock_id, &buf, cntr, 0)) {
        message(ER_ERROR_ON_SEND);
        fl_status = 0;
        continue;
       }
     }
    if (0 == fl_status) continue;

    // закрываем сокет и файл
    shutdown(sock_id, SHUT_RDWR);
    close(sock_id);
    fclose(file_id);
    fl_status = 1;
  } while ( 0 == fl_status);

  // выход
  return 0;
}
