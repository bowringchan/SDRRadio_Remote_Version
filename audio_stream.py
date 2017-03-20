#coding=utf-8
import dsp
import encoder
import threading
from time import sleep
import fifo_tool
import os
import signal

class audio_stream_controller:
    def __init__(self):
        self.thread_running = True

    #该函数在tb子进程执行
    def dsp_trigger(self,fifo_filename):
        self.tb = dsp.SignalReceiver(fifo_filename)
        print 'tb constructed\n'
        self.tb.connect_FM_blocks_filesink()
        print 'tb connected\n'
        #绑定结束函数用于接收SIGTERM，并结束tb
        #signal.signal(signal.SIGTERM, self.sigterm_handler)
        self.tb.start()
        print 'tb start\n'

    #该函数用于结束tb子进程
    def sigterm_handler(_signo, _stack_frame):
        self.tb.stop()
        self.tb.wait()
        sleep(2)

    def main(self):
        fifo_filename = 'audio/filesink.raw'
        fifo_tool_i = fifo_tool.Fifo_Generator()
        fifo_tool_i.mkfifo_file(fifo_filename, 0644)
        print 'fifo made\n'
        self.batch_encoder_i = encoder.Batch_Encoder(fifo_tool_i)
        print 'batch encoder constructed'
        #encode_mkm3u8_thread = threading.Thread(target=self.batch_encoder_i.encode_mkm3u8)
        encode_mkm3u8_thread = threading.Thread(target=self.batch_encoder_i.encode_ffmpeg_fifo)
        encode_mkm3u8_thread.start()

        sub_pid = os.fork()
        if sub_pid != 0:
            self.dsp_trigger(fifo_filename)
        else:
            # try:
                # while True:
                    # sleep(1)
            # except KeyboardInterrupt:
                # self.batch_encoder_i.thread_running = False
                # os.kill(sub_pid, signal.SIGKILL)
                # print 'sub_pid '+str(sub_pid)+' killed\n'

            while True:
                if self.thread_running == True:
                    sleep(1)
                else:
                    self.batch_encoder_i.thread_running = False
                    os.kill(sub_pid, signal.SIGKILL)
                    print 'Gnuradio Top Block :sub_pid '+str(sub_pid)+' killed\n'
                

if __name__ == '__main__':
    main()
