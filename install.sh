#!/bin/sh
# install.sh - Installation script for the MR16

sudo apt-get update
sudo apt-get upgrade

# Python Base
sudo apt-get install python-dev build-essential python-pip

# Arduino
sudo apt-get install arduino arduino-mk

# MongoDB
sudo apt-get install mongodb
sudo pip install pymongo

# CMQ
sudo apt-get install zmq python-serial python-zmq

# OBD
sudo apt-get install python-cherrypy3

# HUD
sudo apt-get install python-tkinter

