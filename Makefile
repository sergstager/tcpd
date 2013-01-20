all:	tcpdaemon

tcpdaemon:	tcpdaemon.c tcpdaemon.h mainloop.c
	gcc tcpdaemon.c -o tcpdaemon -lconfig
	strip tcpdaemon

clean:
	rm -f tcpdaemon /var/run/tcpdaemon.pid /usr/local/tmp/*

kill:
	kill `cat /var/run/tcpdaemon.pid`
#	killall tcpdaemon
	tail -n 12 /var/log/syslog

test:	all
	./tcpdaemon -n -c ./tcpdaemon.cfg

t:	all
	for i in `seq 1 10`  ; do cat /vmlinuz|nc 127.0.0.1 999 -q 1 ; echo -n . ; done
#	time cat /vmlinuz|nc 127.0.0.1 999 -q 1 -vv
