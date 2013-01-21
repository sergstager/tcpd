all:	tcpdaemon tcpclient

tcpdaemon:	tcpdaemon.c tcpdaemon.h mainloop.c
	gcc tcpdaemon.c -o tcpdaemon -Wall
	strip tcpdaemon

tcpclient:	tcpclient.c tcpclient.h
	gcc tcpclient.c -o tcpclient -Wall
	strip tcpclient

clean:
	rm -f tcpdaemon /var/run/tcpdaemon.pid /usr/local/tmp/*

kill:
	kill `cat /var/run/tcpdaemon.pid`
#	killall tcpdaemon
	tail -n 12 /var/log/syslog

test:	all
#	./tcpclient -h 127.0.0.1 -p 999 linux
	./tcpdaemon -n -c ./tcpdaemon.cfg

t:	all
#	./tcpclient -i 127.0.0.1 -p 999 -f linux &
#	for i in `seq 1 10`  ; do ( cat linux|nc 127.0.0.1 999 -q 1 & ) ; echo -n . ; done
	for i in `seq 1 10`  ; do ( ./tcpclient -i 127.0.0.1 -p 999 -f linux & ) ; echo -n . ; done
	sleep 2.5s
	ps axjf
	tail -f /var/log/syslog
#	time cat /vmlinuz|nc 127.0.0.1 999 -q 1 -vv
