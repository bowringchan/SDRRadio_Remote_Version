#!/usr/bin/env python2
#coding=utf-8

from gnuradio import analog
from gnuradio import audio
from gnuradio import filter
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio import blocks

import osmosdr



class SignalReceiver(gr.top_block):
    def __init__(self,fifo_filename):
        gr.top_block.__init__(self, "SignalReceiver")

        # Variables for FM and AM
        # FM
        self.samp_rate = 2e6
        self.RTL_SDR_center_freq = 100.7e6

        self.FM_cutoff_freq = 75e3
        self.FM_transition_width = 25e3
        # AM
        # TODO: FROM HERE
        self.AirAM_cutoff_freq = 7e3
        self.AirAM_transition_width = 15e2
        self.AirAM_default_Volume = 50
        self.demodulation_scheme = 'FM'

        # Setting Blocks
        self.audio_sink_0 = audio.sink(48000, '', True)

        self.rtlsdr_source_0 = osmosdr.source(args="numchan=" + str(1) + " " + '')
        self.rtlsdr_source_0.set_sample_rate(self.samp_rate)
        self.rtlsdr_source_0.set_center_freq(self.RTL_SDR_center_freq, 0)
        self.rtlsdr_source_0.set_freq_corr(0, 0)
        self.rtlsdr_source_0.set_dc_offset_mode(0, 0)
        self.rtlsdr_source_0.set_iq_balance_mode(0, 0)
        self.rtlsdr_source_0.set_gain_mode(False, 0)
        self.rtlsdr_source_0.set_gain(30, 0)
        self.rtlsdr_source_0.set_if_gain(30, 0)
        self.rtlsdr_source_0.set_bb_gain(30, 0)
        self.rtlsdr_source_0.set_antenna('', 0)
        self.rtlsdr_source_0.set_bandwidth(0, 0)
        # FM
        self.rational_resampler_FM_0 = filter.rational_resampler_fff(
            interpolation=48,
            decimation=50,
            taps=None,
            fractional_bw=None,
        )

        self.low_pass_filter_FM_0 = filter.fir_filter_ccf(4, firdes.low_pass(
            1, self.samp_rate, self.FM_cutoff_freq, self.FM_transition_width, firdes.WIN_BLACKMAN, 6.76))
        self.analog_wfm_rcv_0 = analog.wfm_rcv(
            quad_rate=500000,
            audio_decimation=10,
        )

        # AM

        #FileSink
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((128,))
        self.blocks_float_to_uchar_0 = blocks.float_to_uchar()
        print 'Debug point: dsp file sink start\n'
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_char * 1, fifo_filename, False)
        print 'Debug point: dsp file sink pass\n'
        self.blocks_file_sink_0.set_unbuffered(False)
        self.blocks_add_const_vxx_0 = blocks.add_const_vff((127,))

    def connect_FM_blocks(self):
        self.connect((self.rtlsdr_source_0, 0), (self.low_pass_filter_FM_0, 0))
        self.connect((self.low_pass_filter_FM_0, 0), (self.analog_wfm_rcv_0, 0))
        self.connect((self.analog_wfm_rcv_0, 0), (self.rational_resampler_FM_0, 0))
        self.connect((self.rational_resampler_FM_0, 0), (self.audio_sink_0, 0))

    def connect_FM_blocks_filesink(self):
        self.connect((self.rtlsdr_source_0, 0), (self.low_pass_filter_FM_0, 0))
        self.connect((self.low_pass_filter_FM_0, 0), (self.analog_wfm_rcv_0, 0))
        self.connect((self.analog_wfm_rcv_0, 0), (self.rational_resampler_FM_0, 0))
        self.connect((self.rational_resampler_FM_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.blocks_add_const_vxx_0, 0))
        self.connect((self.blocks_add_const_vxx_0, 0), (self.blocks_float_to_uchar_0, 0))
        self.connect((self.blocks_float_to_uchar_0, 0), (self.blocks_file_sink_0, 0))

    def disconnect_FM_blocks(self):
        self.disconnect((self.rtlsdr_source_0, 0), (self.low_pass_filter_FM_0, 0))
        self.disconnect((self.low_pass_filter_FM_0, 0), (self.analog_wfm_rcv_0, 0))
        self.disconnect((self.analog_wfm_rcv_0, 0), (self.rational_resampler_FM_0, 0))
        self.disconnect((self.rational_resampler_FM_0, 0), (self.audio_sink_0, 0))

    def disconnect_FM_blocks_filesink(self):
        self.disconnect((self.rtlsdr_source_0, 0), (self.low_pass_filter_FM_0, 0))
        self.disconnect((self.low_pass_filter_FM_0, 0), (self.analog_wfm_rcv_0, 0))
        self.disconnect((self.analog_wfm_rcv_0, 0), (self.rational_resampler_FM_0, 0))
        self.disconnect((self.rational_resampler_FM_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.disconnect((self.blocks_multiply_const_vxx_0, 0), (self.blocks_add_const_vxx_0, 0))
        self.disconnect((self.blocks_add_const_vxx_0, 0), (self.blocks_float_to_uchar_0, 0))
        self.disconnect((self.blocks_float_to_uchar_0, 0), (self.blocks_file_sink_0, 0))

    def set_FM_freq(self, new_center_freq):
        self.RTL_SDR_center_freq = new_center_freq
        self.rtlsdr_source_0.set_center_freq(new_center_freq)
        print 'set_FM_freq: '+str(new_center_freq)

    def set_cutoff_freq(self, new_cutoff_freq):
        self.FM_cutoff_freq = new_cutoff_freq
        self.FM_transition_width = new_cutoff_freq / 4
        self.low_pass_filter_FM_0.set_taps(firdes.low_pass(
            1, self.samp_rate, self.FM_cutoff_freq, self.FM_transition_width, firdes.WIN_BLACKMAN, 6.76))
        print 'set_cutoff_freq: '+str(new_cutoff_freq)

if __name__ == '__main__':
    tb = SignalReceiver()
    tb.connect_FM_blocks()
    tb.run()
