# -*- coding: utf-8 -*-
import subprocess
import m3u8_generator


class Batch_Encoder:
    def __init__(self, fifo_desc):
        self.fifo_desc = fifo_desc
        self.output_list = []
        self.output_counter = 1
        self.EXTINF = 1
        self.media_sequence = 1
        self.thread_running = True

    def encode(self):
        # 1s 8bit linear PCM raw audio data take 192kb = 24kB
        buf = self.fifo_desc.read(24000 * self.EXTINF)
        output = open(r'audio\audio' + str(self.output_counter) + r'.raw', 'w')
        output.write(buf)
        output.close()
        self.output_counter += 1

        # TODO Convert And Append mp3 filename to output_list
        # lame -r -s 48 --bitwidth 8 --unsigned --quiet
        subprocess.call(
            ["lame", "-r", "-s", "48", "--bitwidth 8", "--unsigned", r'audio\audio' + str(self.output_counter) + r'.raw',
             r'audio\audio' + str(self.output_counter) + r'.mp3'])
        self.output_list.append(r'audio\audio' + str(self.output_counter) + r'.mp3')
        if self.output_list.__len__() > 3:
            self.output_list.pop(0)

    # ENTRY
    def encode_mkm3u8(self):
        while True:
            if self.thread_running == True:
                self.encode()
                m3u8_generator_i = m3u8_generator.M3u8_Generator(self.EXTINF)
                if self.output_counter > 2:
                    m3u8_generator_i.generate(self.output_list, self.media_sequence)
                    self.media_sequence += 1
