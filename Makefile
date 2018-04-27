# Makefile for alarm
#

# WiringPi lib needed for functionality (wiringpi.con).  May be disabled to test compile on platform without wiringPi
USE_WIRING_PI=yes

# install libcurl4-nss-dev for curl lib

INCLUDE= stdafx.h Config.h AlarmManager.h Serial.h sendEmail.h SockClient.h logMsg.h

DEFINES= 
LDFLAGS= -Wall -lcurl

ifdef USE_WIRING_PI
  DEFINES+= -D_HAVE_WIRING_PI
  LDFLAGS+= -lwiringPi
endif

CFLAGS= -Wall -Wno-write-strings -O2 $(DEFINES)
OBJS= main.o Config.o AlarmManager.o Serial.o sendEmail.o SockClient.o logMsg.o

all: alarm

alarm: $(OBJS) $(INCLUDE)
	g++ -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp $(INCLUDE)
	g++ -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o *~ alarm

# install must be done as root
install: alarm
	cp alarm /usr/local/bin/
	cp alarm_config /etc
	chmod 755 /usr/local/bin/alarm
	chmod 600 /etc/alarm_config
