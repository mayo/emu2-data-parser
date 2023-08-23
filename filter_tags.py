#!/usr/bin/env python
#cat sample.txt| python filter_tags.py

import sys

buffer = ""
state = 'find_tag'
tags = set([])

for x in sys.stdin.read():
    # primitive strip of control characters
    if ord(x) < 32:
        continue

    if state == 'find_tag':
        if x == '<':
            state = 'collect'
            continue

    elif state == 'collect':
        if x == ' ':
            continue

        if x == '<':
            buffer = ""
            continue

        if x == '>':
            if len(buffer) > 0:
                if buffer not in tags:
                    tags.add(buffer)
                    print(buffer)
            state = 'find_tag'
            buffer = ""
            continue

        if x == '/':
            continue

        buffer += x
        continue
