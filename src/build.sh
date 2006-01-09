#gcc smbparser.c -DTEST -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -lglib-2.0 -o smbparser
#gcc smbparser.c smbparser-dbus-server.c  -Wall -DDBUS_API_SUBJECT_TO_CHANGE -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I /usr/include/dbus-1.0 -lgli\b-2.0 -ldbus-glib-1 -o server
gcc smbparser-dbus-client.c  -Wall -DTEST -DDBUS_API_SUBJECT_TO_CHANGE -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I /usr/include/dbus-1.0 -lgli\b-2.0 -ldbus-glib-1  `pkg-config --cflags --libs libglade-2.0`  -o client 
#gcc test.c  -Wall -DTEST -DDBUS_API_SUBJECT_TO_CHANGE -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I /usr/include/dbus-1.0 -lgli\b-2.0 -ldbus-glib-1 -o test
#cc monitor.c -o monitor `pkg-config --libs --cflags gamin`
