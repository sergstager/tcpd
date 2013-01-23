#ifndef _TCPDAEMON_H_
#define _TCPDAEMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <time.h>

//  переменные конфига
char daemon_config[1024];
char daemon_pid[1024];
char daemon_workdir[1000]; // к нему в конец будет добавляться шаблон временного файла
#define TMP_TEMPLATE	"/tdmnXXXXXX"
char daemon_username[256];
char daemon_groupname[256];
char daemon_address[16];
char daemon_exec[1024];
char daemon_exec_args[1024];
int config_status, daemon_port, daemon_loglevel;

// флаг демонистости:
//   1 - работать как демон
//   0 - работать в фореграунде
int daemonize = 1;

// сокет демона
int skListener;

// длина очереди
#define MAX_CONNECTION	5

// максимальное количество аргументов внешней команды
#define MAX_ARGV_COUNT	10

/////////////////////////////////
//
//  коды ошибок
//
#define ER_CANT_FORK		1	// демон не может форкнуться при запуске или при обработке входящего сокета
#define ER_CANT_SETSID		2	// демон не может создать сеанс
#define ER_CANT_CHDIR		3	// демон не может перейти в workdir
#define ER_CANT_READ_CONFIG	4	// не читается конфиг
#define ER_CANT_READ_CONFIG_PID	5	// не читается pid из конфига
#define ER_CANT_READ_CONFIG_WORKDIR	6	// не читается workdir из конфига
#define ER_CANT_READ_CONFIG_USERNAME	7	// не читается username из конфига
#define ER_CANT_READ_CONFIG_ADDRESS	8	// не читается address из конфига
#define ER_CANT_READ_CONFIG_PORT	9	// не читается port из конфига
#define ER_CANT_READ_CONFIG_EXEC	10	// не читается exec из конфига
#define ER_CANT_READ_CONFIG_EXEC_ARGS	11	// не читаются аргументы exec из конфига
#define ER_CANT_READ_CONFIG_GROUPNAME	12	// не читается groupname из конфига
#define ER_CANT_READ_CONFIG_LOGLEVEL	13	// не читается loglevel из конфига
#define ER_CHILD_FAILED		17	// внешняя команда завершилась с ошибкой
#define ER_CANT_SETGID		18	// не меняется gid
#define ER_CANT_SETUID		19	// не меняется uid
#define ER_CANT_CREATE_SOCKET	20	// не создаётся сокет
#define ER_CANT_BIND		21	// не биндится сокет
#define ER_CANT_LISTEN		22	// не прослушивается
#define ER_CANT_ACCEPT		23	// не цепляется новое соединение

char str_errors[27][1024]= {
  "all OK",
  "Can't fork",
  "Can't setsid",
  "Can't chdir into work dir",
  "Can't read config file",
  "Can't read 'pid' from config file", // 5
  "Can't read 'workdir' from config file",
  "Can't read 'username' from config file",
  "Can't read 'address' from config file",
  "Can't read 'port' from config file",
  "Can't read 'exec' from config file", // 10
  "Can't read 'exec_args' from config file",
  "Can't read 'groupname' from config file",
  "Can't read 'loglevel' from config file",
  "",
  "", // 15
  "",
  "Child process failed",
  "Can't change GID of process",
  "Can't change UID of process",
  "Can't create socket", // 20
  "Can't bind socket",
  "Can't listen socket",
  "Can't accept socket",
  "",
  "", // 25
  ""
};

///////////////////////////////
//
//  вывод хелпа
//
void printUsage(char *argv[]) {
  printf("Usage: %s [-h] [-c config_file] [-n]\n", argv[0]);
  printf("Options:\n");
  printf("\t-h\tshow this help\n");
  printf("\t-n\tnot daemonize (stay on top)\n");
  printf("\t-c config_file\t path to config file\n\n");
}

///////////////////////////////
//
//  функция завершения демона
//
void quit(int err) {
  if (err) {
    if (daemon_loglevel >= 1) syslog(LOG_DEBUG, "error %d: %s", err, str_errors[err]);
    if (daemonize) printf("error %d: %s\n", err, str_errors[err]);
   }
  if (skListener) {
    shutdown(skListener, SHUT_RDWR);
    close(skListener);
   }
  exit(err);
}

///////////////////////////////
//
//  обработчик сигналов
//
void signalHandler(int sig) {
  switch (sig) {
    case SIGHUP:
    case SIGINT:
      syslog(LOG_INFO, "terminated");
      quit(0);
      break;
    case SIGQUIT:
    case SIGTERM:
      syslog(LOG_INFO, "stop");
      quit(0);
      break;
    case SIGCHLD:
      waitpid(-1,0,WNOHANG);
      break;
    default:
//      syslog(LOG_WARNING, "recieved %s signal", strsignal(sig));
      break;
   }
}

///////////////////////////////////
//
//  выкидывает начальные и конечные пробелы из строки
//
char *strtrim(char *s) {
  int i1 = strlen(s);
  while (isspace(s[i1 - 1])) --i1;
  while (*s && isspace(*s)) ++s, --i1;
  return strndup(s, i1);
}

///////////////////////////////////
//
//  получение значения из конфига
//
char *config_getValue(const char *path, const char *key, char *val) {
  FILE *fd;
  char str1[1024],str2[1024],s1[1024],s2[1024];
  int i1 = 0;

  if ( (fd = fopen(path, "r")) ) {
    while (!feof(fd)) {
      if (fgets(str1, 1000, fd)) {
        memset(&str2, 0, sizeof(str2));
        // выкидываем комментарии и перевод строки
        for(i1 = 0; ((str1[i1] != '#') && (str1[i1] != '\n') && (str1[i1] != '\000') && (i1 < sizeof(str1)-1)) ; i1++) str2[i1]=str1[i1];
        str2[i1]='\000';
        memset(&s1, 0, sizeof(s1));
        memset(&s2, 0, sizeof(s2));
        // разбираем на две части
        if (2 == sscanf(str2, "%[ \ta-zA-Z0-9_-/.]=%[ \t\%\"\'a-zA-Z0-9_-/.]", s1, s2)) {
          // если ключ совпал - возвращаем значение
          if (!strcmp(key,strtrim(s1))) return strncpy(val,strtrim(s2),strlen(strtrim(s2)));
          // FIXME течёт память в strtrim
         }
       }
     }
    fclose(fd);
   }
  return NULL;
}

////////////////////////////////
//
//  парсинг конфига
//
int parseConfig(char *path) {
  char port[32];
  memset(&port, 0, sizeof(port));

  if (!config_getValue(path, "pid", daemon_pid)) return ER_CANT_READ_CONFIG_PID;
  if (!config_getValue(path, "workdir", daemon_workdir)) return ER_CANT_READ_CONFIG_WORKDIR;
  if (!config_getValue(path, "username", daemon_username)) return ER_CANT_READ_CONFIG_USERNAME;
  if (!config_getValue(path, "groupname", daemon_groupname)) return ER_CANT_READ_CONFIG_GROUPNAME;
  if (!config_getValue(path, "address", daemon_address)) return ER_CANT_READ_CONFIG_ADDRESS;
  if (!config_getValue(path, "exec", daemon_exec)) return ER_CANT_READ_CONFIG_EXEC;
  if (!config_getValue(path, "exec_args", daemon_exec_args)) return ER_CANT_READ_CONFIG_EXEC_ARGS;
  if (!config_getValue(path, "port", port)) return ER_CANT_READ_CONFIG_PORT;
  daemon_port=atoi(port);
  memset(&port, 0, sizeof(port));
  if (!config_getValue(path, "loglevel", port)) return ER_CANT_READ_CONFIG_LOGLEVEL;
  daemon_loglevel=atoi(port);

  if (daemon_loglevel >= 1) syslog(LOG_DEBUG, "configuration complete");

  return 0;
}

#endif // _TCPDAEMON_H_
