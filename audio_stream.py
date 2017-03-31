#coding=utf-8
import dsp
import encoder
import threading
from time import sleep
import fifo_tool
import os
import signal
import multiprocessing

class audio_stream_controller:
    def __init__(self):
        self.thread_running = True
        self.encoder_ready = [0]


    def dsp_trigger(self,fifo_filename,que):
        tb = dsp.SignalReceiver(fifo_filename)
        tb.connect_FM_blocks_filesink()
        print 'tb going to start\n'
        tb.start()
        while True:
            if que.empty() == True:
                sleep(1)
            else:
                commands_dict = que.get()
                for k in commands_dict:
                    if k == "freq":
                        tb.set_FM_freq(commands_dict[k][0])
                        tb.set_cutoff_freq(commands_dict[k][1])
                        print 'Set Freq:'+str(commands_dict[k][0])+'bandwidth:'+str(commands_dict[k][1]*2)

    #command: dict{"k":"v"}
    def set_dsp_command(self,command):
        self.que.put(command)

    def main(self):
        fifo_filename = 'audio/filesink.raw'
        fifo_tool_i = fifo_tool.Fifo_Generator()
        fifo_tool_i.mkfifo_file(fifo_filename, 0644)
        print 'fifo made\n'
        self.batch_encoder_i = encoder.Batch_Encoder(fifo_tool_i)
        print 'batch encoder constructed'
        #encode_mkm3u8_thread = threading.Thread(target=self.batch_encoder_i.encode_mkm3u8)
        encode_mkm3u8_thread = threading.Thread(target=self.batch_encoder_i.encode_ffmpeg_fifo,args=(self.encoder_ready,))
        encode_mkm3u8_thread.start()
        
        # sub_pid = os.fork()
        # if sub_pid != 0:
            # self.dsp_trigger(fifo_filename)
        # else:
            # while True:
                # if self.thread_running == True:
                    # sleep(1)
                # else:
                    # self.batch_encoder_i.thread_running = False
                    # os.kill(sub_pid, signal.SIGKILL)
                    # print 'Gnuradio Top Block :sub_pid '+str(sub_pid)+' killed\n'
        while(self.encoder_ready[0] == 0):
            sleep(1);
        print 'encoder_ready\n'
        self.que = multiprocessing.Queue()
        p = multiprocessing.Process(target=self.dsp_trigger, args=(fifo_filename,self.que))
        p.start()
        while True:
            if self.thread_running == True:
                sleep(1)
            else:
                break
        self.encoder_ready[0] = 0
        self.batch_encoder_i.thread_running = False
        os.kill(p.pid, signal.SIGKILL)

if __name__ == '__main__':
    main()
