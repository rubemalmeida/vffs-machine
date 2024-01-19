#!/bin/sh

sed -e '59s/.*/const String hostname = SECRET_HOSTNAME/' -e '60s/.*/const char *ssid = SECRET_SSID/' -e '61s/.*/const char *password = SECRET_WIFI_PASSWORD/'
