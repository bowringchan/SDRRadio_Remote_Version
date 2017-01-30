import dsp
import encoder
import threading
from time import sleep
import fifo_tool
import os
import signal

def dsp_trigger(fifo_filename):
    tb = dsp.SignalReceiver(fifo_filename)
    print 'tb constructed\n'
    tb.connect_FM_blocks_filesink()
    print 'tb connected\n'
    tb.run()
    print 'tb run\n'

def main():
    fifo_filename = 'audio/filesink.raw'
    fifo_tool_i = fifo_tool.Fifo_Generator()
    fifo_tool_i.mkfifo_file(fifo_filename, 0644)
    print 'fifo made\n'
    batch_encoder_i = encoder.Batch_Encoder(fifo_tool_i)
    print 'batch encoder constructed'
    encode_mkm3u8_thread = threading.Thread(target=batch_encoder_i.encode_mkm3u8)
    encode_mkm3u8_thread.start()

    sub_pid = os.fork()
    if sub_pid == 0:
        dsp_trigger(fifo_filename)
    else:
        try:
            while True:
                sleep(1)
        except KeyboardInterrupt:
            batch_encoder_i.thread_running = False
            os.kill(sub_pid, signal.SIGKILL)
            print 'sub_pid '+str(sub_pid)+' killed\n'

if __name__ == '__main__':
    main()
