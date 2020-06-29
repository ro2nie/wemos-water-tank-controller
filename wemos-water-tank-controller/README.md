# Wemos water tank controller

This is for the water tank controller that uses a jsn-sr04t ultrasonic sensor.
The board will join the WiFi network specified in connectionDetails.h
Once connected, it will send and receive MQTT messages that will be used to control its behaviour.

In order to flash this onto a Wemos D1 mini, please rename the conectionDetails-example.h to connectionDetails.h
Then modify the credentials for the WiFi network, and the IP address of the MQTT broker.
Also modify the credentials used to authenticate with MQTT broker.