# -*- coding: utf-8 -*-
import subprocess
import m3u8_generator
import fifo_tool

class Batch_Encoder:
    def __init__(self, fifo_tool_i):
        self.fifo_tool_i = fifo_tool_i
        self.output_list = []
        self.output_counter = 1
        self.EXTINF = 5
        self.media_sequence = 1
        self.thread_running = True

    def encode(self):
        # 1s 8bit linear PCM raw audio data take 48KB
        #print 'Debug point: encode start reading'
        buf = self.fifo_desc.read(48000 * self.EXTINF)
        output = open('audio/audio' + str(self.output_counter) + '.raw', 'w')
        output.write(buf)
        output.close()


        # TODO Convert And Append mp3 filename to output_list
        # lame -r -s 48 --bitwidth 8 --unsigned --quiet
        subprocess.call(
            "lame -r -s 48 --bitwidth 8 --unsigned --quiet -m m audio/audio" + str(self.output_counter) + ".raw"+
             " audio/audio" + str(self.output_counter) + ".mp3",shell = True)
        self.output_list.append('audio/audio' + str(self.output_counter) + '.mp3')
        if self.output_list.__len__() > 3:
            self.output_list.pop(0)

    # ENTRY
    def encode_mkm3u8(self):
        print 'Debug point:encode Thread run\n'
        self.fifo_desc = self.fifo_tool_i.open_file_for_read()
        print 'Debug point:encode fifo read open\n'

        while True:
            if self.thread_running == True:
                self.encode()
                m3u8_generator_i = m3u8_generator.M3u8_Generator(self.EXTINF)

                m3u8_generator_i.generate(self.output_list, self.media_sequence)
                self.media_sequence += 1

                self.output_counter += 1
            else:
                break
        self.fifo_tool_i.delfifo_file()
        subprocess.call("sh clean.sh",shell = True)
