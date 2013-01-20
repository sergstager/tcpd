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
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <libconfig.h>

//  переменные конфига
char daemon_config[1024];
char daemon_pid[1024];
char daemon_workdir[1000]; // к нему в конец будет добавляться шаблон временного файла
#define TMP_TEMPLATE	"/tdmnXXXXXX"
char daemon_username[256];
char daemon_address[16];
char daemon_exec[1024];
int config_status, daemon_port;

// флаг демонистости:
//   1 - работать как демон
//   0 - работать в фореграунде
int daemonize = 1;

// сокет демона
int skListener;

// длина очереди
#define MAX_CONNECTION	5

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
#define ER_CANT_READ_CONFIG_EXEC	8	// не читается exec из конфига
#define ER_CANT_SETUID		19	// не меняется uid
#define ER_CANT_CREATE_SOCKET	20	// не создаётся сокет
#define ER_CANT_BIND		21	// не биндится сокет
#define ER_CANT_LISTEN		22	// не прослушивается
#define ER_CANT_ACCEPT		23	// не цепляется новое соединение

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
  if(err) syslog(LOG_WARNING, "quit on %d", err);
  if(skListener) {
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
      syslog(LOG_WARNING, "recieved %s signal", strsignal(sig));
      break;
    case SIGINT:
      syslog(LOG_WARNING, "recieved %s signal", strsignal(sig));
      break;
    case SIGQUIT:
    case SIGTERM:
      syslog(LOG_WARNING, "recieved %s signal", strsignal(sig));
      quit(0);
      break;
    case SIGCHLD:
      waitpid(-1,0,WNOHANG);
      break;
    default:
      syslog(LOG_WARNING, "recieved %s signal", strsignal(sig));
      break;
   }
}

////////////////////////////////
//
//  парсинг конфига
//
int parseConfig(char *path) {
  struct config_t cfg;
  config_setting_t *setting;
  const char *str;
  int i1;
  long l1;

  config_init(&cfg);
  if (!config_read_file(&cfg, path)) {
    config_destroy(&cfg);
    return ER_CANT_READ_CONFIG;
   }

  if (!config_lookup_string(&cfg, "pid", &str)) return ER_CANT_READ_CONFIG_PID;
  else strncpy(daemon_pid, str, sizeof(daemon_pid)-1);

  if (!config_lookup_string(&cfg, "workdir", &str)) return ER_CANT_READ_CONFIG_WORKDIR;
  else strncpy(daemon_workdir, str, sizeof(daemon_workdir)-1);

  if (!config_lookup_string(&cfg, "username", &str)) return ER_CANT_READ_CONFIG_USERNAME;
  else strncpy(daemon_username, str, sizeof(daemon_username)-1);

  if (!config_lookup_string(&cfg, "address", &str)) return ER_CANT_READ_CONFIG_ADDRESS;
  else strncpy(daemon_address, str, sizeof(daemon_address)-1);

  if (!config_lookup_string(&cfg, "exec", &str)) return ER_CANT_READ_CONFIG_EXEC;
  else strncpy(daemon_exec, str, sizeof(daemon_exec)-1);

  if (!config_lookup_int(&cfg, "port", &l1)) return ER_CANT_READ_CONFIG_PORT;
  else {
    i1 = l1;
    daemon_port = i1;
   }
   
  return 0;
}

#endif // _TCPDAEMON_H_
