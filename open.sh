#!/bin/bash

gnome-terminal --tab --title='TTY0'  --command 'make login MOTES=/dev/ttyUSB0' \
                          --tab --title='TTY1'  --command 'make login MOTES=/dev/ttyUSB1' \
                          --tab --title='TTY2'  --command 'make login MOTES=/dev/ttyUSB2' \
                          --tab --title='TTY3'  --command 'make login MOTES=/dev/ttyUSB3'
