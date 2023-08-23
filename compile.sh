#!/bin/sh

gcc -o parse main.c xml_parser.c murmur3.c emu2_parser.c -Wall
