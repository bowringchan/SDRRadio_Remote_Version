#coding=utf-8
import subprocess
from time import sleep
import os
import signal
import m3u8_generator
import fifo_tool

class Batch_Encoder:
    def __init__(self, fifo_tool_i):
        self.fifo_tool_i = fifo_tool_i
        self.output_list = []
        self.output_counter = 1
        self.EXTINF = 1
        self.media_sequence = 1
        self.thread_running = True

    def encode_ffmpeg_fifo(self, encoder_ready):
        #subprocess.call("ffmpeg -loop 1 -i image.jpg -f u8 -ar 48000 -channels 1 -i audio/filesink.raw -c:v libx264 -tune stillimage -pix_fmt yuv420p -ac 2 -c:a aac -f hls -hls_time 3 -hls_list_size 5 -hls_segment_filename 'audio%03d.ts' RTLSDR.m3u8",shell = True)
        ffmpeg_p = subprocess.Popen('exec '+"ffmpeg -f u8 -ar 48000 -channels 1 -i ../audio/filesink.raw -c:a aac -f hls -hls_flags omit_endlist -hls_time "+str(self.EXTINF)+" -hls_list_size 3 -hls_segment_filename 'audio%03d.ts' RTLSDR.m3u8", shell=True,cwd = os.getcwd()+'/static')
        encoder_ready[0] = 1
        while True:
            if self.thread_running == True:
                sleep(1)
            else:
                break
        ffmpeg_p.kill()
        print 'ffmpeg exited\n',ffmpeg_p.pid

    def encode(self):
        # 1s 8bit linear PCM raw audio data take 48KB
        #print 'Debug point: encode start reading'
        buf = self.fifo_desc.read(48000 * self.EXTINF)
        output = open('audio/audio' + str(self.output_counter) + '.raw', 'w')
        output.write(buf)
        output.close()
        # TODO Convert And Append mp3 filename to output_list
        # lame -r -s 48 --bitwidth 8 --unsigned --quiet
		#ffmpeg -f u8 -ar 48000 -i audio1.raw -f mpegts output.mp3 -v 0
        #subprocess.call(
        #    "lame -r -s 48 --bitwidth 8 --unsigned --quiet -m m audio/audio" + str(self.output_counter) + ".raw"+
        #     " audio/audio" + str(self.output_counter) + ".mp3",shell = True)
        subprocess.call("ffmpeg -f u8 -ar 48000 -channels 1 -i audio/audio"+str(self.output_counter)+".raw -f mpegts -mpegts_copyts 1 -output_ts_offset "+ str((self.output_counter-1) * self.EXTINF)+" -b:a 128k audio/audio"+str(self.output_counter)+".mp3 -v 0",shell = True)
        self.output_list.append('audio/audio' + str(self.output_counter) + '.mp3')
        #if self.output_list.__len__() > 5:
            #self.output_list.pop(0)
            #self.media_sequence += 1

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

                self.output_counter += 1
            else:
                break
        self.fifo_tool_i.delfifo_file()
        subprocess.call("sudo sh clean.sh",shell = True)
