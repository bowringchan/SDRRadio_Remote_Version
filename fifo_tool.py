# -*- coding: utf-8 -*-
from os import mkfifo
from os import unlink


class fifo_generator:
    def mkfifo_file(self, filename, numeric_mod):
        self.filename = filename
        self.numeric_mod = numeric_mod
        mkfifo(filename, numeric_mod)
        self.fifo_desc = open(filename, 'w')
        return self.fifo_desc

    def delfifo_file(self):
        self.fifo_desc.close()
        unlink(self.filename)
