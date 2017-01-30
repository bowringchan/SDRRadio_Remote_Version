import dsp
import fifo_tool
import encoder
import threading
from time import sleep


def main():
    fifo_filename = '/home/bowring/Downloads/filesink.raw'
    tb = dsp.SignalReceiver(fifo_filename)
    tb.connect_FM_blocks_filesink()
    fifo_tool_i = fifo_tool.Fifo_Generator()
    fifo_desc = fifo_tool_i.mkfifo_file(fifo_filename,644)
    batch_encoder_i = encoder.Batch_Encoder(fifo_desc)
    encode_mkm3u8_thread = threading.Thread(target = batch_encoder_i.encode_mkm3u8)
    encode_mkm3u8_thread.start()

    try:
        while True:
            sleep(1)
    except KeyboardInterrupt:
        batch_encoder_i.thread_running = False
        fifo_tool_i.delfifo_file()
        #TODO: del_fifo_file when exit

if __name__ == '__main__':
    main()
