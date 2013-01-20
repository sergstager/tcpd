/////////////////////////////
//
//  основной цикл, где создаётся сокет и запускаются форки на обрабатывающие сокеты
//
void mainLoop() {
  struct sockaddr_in addr, naddr;
  int ns, sock_id;

  ns = sizeof(naddr);
  // создаём сокет
  skListener = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == skListener) {
    quit(ER_CANT_CREATE_SOCKET);
   }

  //  привязываем сокет
  int yes = 1;
  if ( setsockopt(skListener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
    quit(ER_CANT_BIND);
   }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(daemon_port);
  addr.sin_addr.s_addr = inet_addr(daemon_address); //INADDR_ANY;
  if (-1 == bind(skListener, (struct sockaddr *)&addr, sizeof(addr))) {
    quit(ER_CANT_BIND);
   }

  // здесь меняем id (тк порты ниже 1024 не рут создать не может)
  // сначала gid, потомучто потом уже не будет прав
  struct group *gr = getgrnam(daemon_groupname);
  if (!gr) quit(ER_CANT_SETGID);
  if (setregid(gr->gr_gid, gr->gr_gid)) quit(ER_CANT_SETGID);
  struct passwd *pswd = getpwnam(daemon_username);
  if (!pswd) quit(ER_CANT_SETUID);
  if (setreuid(pswd->pw_uid, pswd->pw_uid)) quit(ER_CANT_SETUID);

  // создаём очередь
  if (-1 == listen(skListener, MAX_CONNECTION)) {
    quit(ER_CANT_LISTEN);
   }

  for (;;) {
    pid_t pid, sid;
    char buf[2048]; // буфер
    char tmpfn[1024]; // имя временного файла
    int cntr;
    FILE *fd;

    // получаем соединение
    sock_id = accept(skListener, (struct sockaddr *)&naddr, &ns);
    if (-1 == sock_id) {
      quit(ER_CANT_ACCEPT);
     }
//    syslog(LOG_WARNING, "OK ip: %s", (char *)inet_ntoa(naddr.sin_addr));

    // форкаемся
    pid = fork();
    if (pid <0) quit(ER_CANT_FORK); // ошибка при форке
    if (0 == pid) {
      // это мы в child'е
      if((sid = setsid()) < 0) quit(ER_CANT_SETSID);
      if (daemonize) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
       }
      // открываем темп файл
      strcpy(tmpfn, daemon_workdir);
      strcat(tmpfn, TMP_TEMPLATE);
      mktemp(tmpfn);
      if (fd = fopen(tmpfn, "w")) {
        // в цикле читаем и пишем в файл
        for (;;) {
          cntr = recv(sock_id, buf, sizeof(buf)-1, 0);
          if (0 != cntr) {
            if (-1 == fwrite(buf, 1, cntr, fd)) {
              syslog(LOG_WARNING, "fail write \"%s\" with %s", fd, buf, strerror(errno));
             }
           }
          if (0 == cntr) break;
          if (-1 == cntr) {
            syslog(LOG_WARNING, "fail socket %d with %s", sock_id, strerror(errno));
            break;
           }
         }
        // закрываем файл
        fclose(fd);
       }
      else {
        syslog(LOG_WARNING, "fail file %d with %s", fd, strerror(errno));
       }
      // закрываем сокет
      shutdown(sock_id, SHUT_RDWR);
      close(sock_id);
      sock_id = -1;
      syslog(LOG_WARNING, "file %s loaded", tmpfn);
      // новый процесс
//      printf("%s\n", tmpfn);
      exit(0);
     }
    else {
      // а это в parent'е
      printf(".");
      fflush(NULL);
     }

   }

  return;
}
