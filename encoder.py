# -*- coding: utf-8 -*-
import subprocess


class batch_encoder:
    def __init__(self,fifo_desc):
        self.fifo_desc = fifo_desc
        self.output_list = []
        self.output_counter = 1

    def encode(self):
        # 1s 8bit linear PCM raw audio data take 192kb = 24kB
        buf = self.fifo_desc.read(24000)
        output = open(r'audio\audio'+self.output_counter+r'.raw','w')
        output.write(buf)
        output.close()
        self.output_counter += 1

        #TODO Convert And Append mp3 filename to output_list
        #lame -r -s 48 --bitwidth 8 --unsigned --quiet
        subprocess.call(["lame","-r","-s","48","--bitwidth 8","--unsigned",r'audio\audio'+self.output_counter+r'.raw',r'audio\audio'+self.output_counter+r'.mp3'])
        self.output_list.append(r'audio\audio'+self.output_counter+r'.mp3')