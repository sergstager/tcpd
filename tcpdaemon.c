#include "tcpdaemon.h"
#include "mainloop.c"

////////////////////////////////////////////
//
//  начало демона
//
int main(int argc, char *argv[]) {
  // инициализация переменных
  daemonize = 1;
  memset(&daemon_config, 0, sizeof(daemon_config));
  memset(&daemon_pid, 0, sizeof(daemon_pid));
  memset(&daemon_username, 0, sizeof(daemon_username));
  memset(&daemon_groupname, 0, sizeof(daemon_groupname));
  memset(&daemon_address, 0, sizeof(daemon_address));
  memset(&daemon_exec, 0, sizeof(daemon_exec));
  memset(&daemon_exec_args, 0, sizeof(daemon_exec_args));
  daemon_port = 999;
  skListener = 0;

  openlog("tcpdaemon", LOG_PID, LOG_DAEMON);

  // обработчики сигналов
  signal(SIGCHLD, signalHandler);
  signal(SIGHUP, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGQUIT, signalHandler);
  signal(SIGINT, signalHandler);

  // парсим опции командной строки
  char opt;
  while ((opt = getopt(argc, argv, "hnc:")) != -1) {
    switch (opt) {
      // опция "n" - не уходить в бэкграунд
      case 'n':
        daemonize = 0;
        break;
      // опция "c" - файл конфига
      case 'c':
        if(optarg) strncpy(daemon_config, optarg, sizeof(daemon_config)-1);
        break;
      // любые другие опции - печатаем usage и выходим
      default:
        printUsage(argv);
        quit(0);
        break;
     }
   }

  // парсим конфиг
  if ( (config_status = parseConfig(daemon_config)) ) quit(config_status);

  // ищем другого демона, уже запущенного
  if (0 != alreadyRunning()) quit(ER_ALREADY_RUNNING);

  // если надо - уходим в демона
  pid_t pid,sid;
  if (daemonize) {
    pid = fork();
    if (pid < 0) quit(ER_CANT_FORK); // ошибка при форке
    if (pid > 0) quit(0); // родитель завершается
    if((sid = setsid()) < 0) quit(ER_CANT_SETSID);
    if(chdir(daemon_workdir) < 0) quit(ER_CANT_CHDIR);
    umask(0);
    // закрываем стандартные потоки
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    syslog(LOG_INFO, "start in daemon mode");
   }
  else {
    syslog(LOG_INFO, "start in foreground mode");
   }

  // сохраняем пид
  pid = getpid();
  FILE *fd;
  if (NULL != (fd = fopen(daemon_pid, "w"))) {
    fprintf(fd, "%d\n", pid);
    fclose(fd);
   }
  else {
    if (daemon_loglevel >= 1) syslog(LOG_DEBUG, "cannot save %s file", daemon_pid);
   }

  // в главный цикл
  mainLoop();
  quit(0);
  return 0;
}
