#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import base64
import hashlib
import time

def solve_pow():
    prefix = input("> ")
    print(time.time(), "solving pow ...");
    solved = b''
    for i in range(1000000000):
        h = hashlib.sha1((prefix + str(i)).encode()).hexdigest();
        if h[:6] == '000000':
            solved = str(i).encode();
            print("solved: ", solved);
            print("base64: ", base64.b64encode(solved));
            break;
    print(time.time(), "done.");

solve_pow();

# vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4 number cindent fileencoding=utf-8 :
