# RPIalarm
This repository is the companion to https://github.com/TomVickers/Arduino2keypad

The code creates an app that runs as a service on a Raspberry PI to perform the functions of a home alarm system.  

My project has been the replacement of a Honeywell Vista-20 alarm board with an open source solution.  My current alarm setup features a Raspberry PI 3 running the code in this repository, serially coupled with a Arduino Mega.  The Arduino acts as a bi-directional translator between the RPI serial port (8N1@115200) and the Honeywell alarm keypads which speak a weird sort-of-serial 8E2@4800 (see the other repository for details).  The system is installed and working well.  I may upload some videos of keypad operations soon.

The alarm includes the features you expect: 
 - bi-directional comms with the commercially available Honeywell 6160 keypads
 - sense loops monitored via GPIO pins
 - arm-away, arm-stay, disarm with entry delay, etc
 - notification of open zones, alarm status, etc

Since the code runs on a RPI, it was easy to include features not available on a non-monitored system
 - Text message notifications of alarm events
 - Multiple pin codes of variable length
 - Interface support for virtual keypad 
 
The Makefile explains most of what is needed to build the code.  It currently depends on having the WiringPi lib (wiringpi.com) and the libcurl4-nss-dev package installed.  Many items can be configured in the alarm_config file without requiring a recompile.

You will see some places (like alarm_config) where you need to plug in your own values for pin codes, etc.  The text message notification works by utilizing a gmail account to send email messages to whatever email address you specify.  I specify an email address connected to my phone sms account.

-A quick note on home/computer security: Obviously, if someone can remotely access your RPi, they can compromise the security of your alarm system.  Think carefully before allowing access through your router to any ports on your RPI.  I am not including details about how I remotely connect to my RPi to avoid compromising the security of my setup.

Update: You will notice some changes having to do with sense loop noise issues.  With a few months of operation, I discovered that certain electrical events (A/C fan startup, one particular fluorescent light turning on) could cause noise on a sense loop that my RPi interpreted as a sense loop momentarily opening and then closing.  I fixed the problem in software by ignoring momentary changes in the sense loop state that were much shorter than could be created by opening & closing a door or window.  I also added a 'Armed-Bypass' mode of operation that is similar to Armed-Stay except that sense loops that are marked as BYPASS in the config file will not cause an alarm.  This might be useful for a door to the garage, etc.

Let me know if you have any questions, comments, or you found this project useful in your home.
