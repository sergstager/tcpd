/////////////////////////////
//
//  основной цикл, где создаётся сокет и запускаются форки на обрабатывающие сокеты
//
void mainLoop() {
  struct sockaddr_in addr, naddr;
  int sock_id;
  unsigned int ns;

  ns = sizeof(naddr);
  // создаём сокет
  skListener = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == skListener) quit(ER_CANT_CREATE_SOCKET);

  //  привязываем сокет
  int yes = 1;
  if ( setsockopt(skListener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) quit(ER_CANT_BIND);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(daemon_port);
  addr.sin_addr.s_addr = inet_addr(daemon_address); //INADDR_ANY;
  if (-1 == bind(skListener, (struct sockaddr *)&addr, sizeof(addr))) quit(ER_CANT_BIND);
  if (daemon_loglevel >= 1) filelogsd("bind to %s:%d", daemon_address, daemon_port);

  // здесь меняем id (тк порты ниже 1024 не рут создать не может)
  // сначала gid, потомучто потом уже не будет прав
  struct group *gr = getgrnam(daemon_groupname);
  if (!gr) quit(ER_CANT_SETGID);
  if (setregid(gr->gr_gid, gr->gr_gid)) quit(ER_CANT_SETGID);
  struct passwd *pswd = getpwnam(daemon_username);
  if (!pswd) quit(ER_CANT_SETUID);
  if (setreuid(pswd->pw_uid, pswd->pw_uid)) quit(ER_CANT_SETUID);
  if (daemon_loglevel >= 1) filelogss("reduce privileges to %s:%s", daemon_username, daemon_groupname);

  // создаём очередь
  if (-1 == listen(skListener, MAX_CONNECTION)) quit(ER_CANT_LISTEN);


  for (;;) {
    pid_t pid;
    char buf[2048]; // буфер
    char tmpfn[1024]; // имя временного файла
    int cntr;
    unsigned long int bc;
    FILE *fd;

    // получаем соединение
    sock_id = accept(skListener, (struct sockaddr *)&naddr, &ns);
    if (-1 == sock_id) quit(ER_CANT_ACCEPT);

    // форкаемся
    pid = fork();
    if (pid <0) quit(ER_CANT_FORK); // ошибка при форке
    if (0 == pid) {
      // это мы в child'е
      // открываем темп файл
      strcpy(tmpfn, daemon_workdir);
      strcat(tmpfn, TMP_TEMPLATE);
      mktemp(tmpfn);
      time_t begin_transfer = time(NULL);
      bc = 0;
      if ( (fd = fopen(tmpfn, "w")) ) {
        filelogss("connect from ip: %s, trying to write to file %s", (char *)inet_ntoa(naddr.sin_addr), tmpfn);
        // в цикле читаем и пишем в файл
        for (;;) {
          cntr = recv(sock_id, buf, sizeof(buf)-1, MSG_WAITALL);
          if (0 != cntr) {
            if (-1 == fwrite(buf, 1, cntr, fd)) {
              if (daemon_loglevel >= 1) filelogss("fail write to %s with %s", tmpfn, strerror(errno));
             }
            bc = bc + cntr;
//            printf("%s", buf);
           }
          if (0 == cntr) break;
          if (-1 == cntr) {
            if (daemon_loglevel >= 1) filelogsd("%s (fail socket %d)", strerror(errno), sock_id);
            break;
           }
         }
        // закрываем файл
        fclose(fd);
       }
      else {
        if (daemon_loglevel >= 1) filelogss("fail file %s with %s", tmpfn, strerror(errno));
       }
      // 
      // закрываем сокет
      printf("\n");
      shutdown(sock_id, SHUT_RDWR);
      close(sock_id);
      sock_id = -1;
      time_t end_transfer = time(NULL);
      filelogslulu("file %s (%lu bytes) loaded in %lu seconds", tmpfn, bc, (long int)end_transfer-begin_transfer);
      // новый процесс
      // argv[] нового процесса с путём до команды в [0]
      char *newargv[MAX_ARGV_COUNT];
      newargv[0] = strdup(daemon_exec);

      char delimiters[] = " \t";
      char *tmpbuf;
      int i1 = 1;
      char *s1;

      char tmpfnbuf[1024];
      snprintf(tmpfnbuf, sizeof(tmpfnbuf), daemon_exec_args, tmpfn);
      if (daemon_loglevel >= 1) filelogss("exec %s %s", daemon_exec, tmpfnbuf);
      s1 = strtok_r(daemon_exec_args, delimiters, &tmpbuf);
      while ((NULL != s1) && (i1 < (MAX_ARGV_COUNT-1))) {
        // если встречается "%s", то оно заменяется на имя временного файла
        if (strcmp("%s", s1)) newargv[i1++] = s1;
        else newargv[i1++] = strdup(tmpfn);
        s1 = strtok_r(NULL, delimiters, &tmpbuf);
       }

      // в конце добавляется NULL и запуск
      newargv[i1++] = NULL;
      execvp(daemon_exec, newargv);

      // порождённый процесс внезапно вернулся обратно
      exit(ER_CHILD_FAILED);
     }
    else {
      // а это в parent'е
      if (!daemonize) printf(".");
      fflush(NULL);
     }

   }

  return;
}
