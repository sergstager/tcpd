all:	tcpdaemon tcpclient

tcpdaemon:	tcpdaemon.c tcpdaemon.h mainloop.c
	gcc tcpdaemon.c -o tcpdaemon -Wall
	strip tcpdaemon

tcpclient:	tcpclient.c tcpclient.h
	gcc tcpclient.c -o tcpclient -Wall
	strip tcpclient

install: tcpdaemon tcpclient
	install -s -o root -g root tcpdaemon /usr/local/bin/tcpdaemon
	install -s -o root -g root tcpclient /usr/local/bin/tcpclient
	install -o root -g root extern_comm /usr/local/bin/extern_comm
	install -o root -g root -m 0644 tcpdaemon.cfg /usr/local/etc/tcpdaemon.cfg

clean:
	rm -f tcpdaemon /var/run/tcpdaemon.pid /usr/local/tmp/*

kill:
	kill `cat /var/run/tcpdaemon.pid`

test:	all
	/usr/local/bin/tcpdaemon -n -c /usr/local/etc/tcpdaemon.cfg

t:	all
	for i in `seq 1 10`  ; do ( ./tcpclient -i 127.0.0.1 -p 999 -f /vmlinuz & ) ; echo -n . ; done
	sleep 1s
	ps axjf
	tail -f /var/log/syslog
