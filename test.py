#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from gnuradio import analog
from gnuradio import audio
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes

from optparse import OptionParser
import osmosdr
import sys


class SignalReceiver(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "SignalReceiver")

        # Variables for FM and AM
        self.samp_rate = 2e6
        self.RTL_SDR_center_freq = 97.5e6

        self.cutoff_freq = 75e3
        self.transition_width = 25e3
        self.demodulation_scheme = ''

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

        self.rational_resampler_FM_0 = filter.rational_resampler_fff(
            interpolation=48,
            decimation=50,
            taps=None,
            fractional_bw=None,
        )

        self.low_pass_filter_FM_0 = filter.fir_filter_ccf(4, firdes.low_pass(
            1, self.samp_rate, self.cutoff_freq, self.transition_width, firdes.WIN_BLACKMAN, 6.76))
        self.analog_wfm_rcv_0 = analog.wfm_rcv(
            quad_rate=500000,
            audio_decimation=10,
        )

    def connect_FM_blocks(self):
        self.connect((self.rtlsdr_source_0, 0), (self.low_pass_filter_FM_0, 0))
        self.connect((self.low_pass_filter_FM_0, 0), (self.analog_wfm_rcv_0, 0))
        self.connect((self.analog_wfm_rcv_0, 0), (self.rational_resampler_FM_0, 0))
        self.connect((self.rational_resampler_FM_0, 0), (self.audio_sink_0, 0))


def main(options=None):
    tb = SignalReceiver()
    tb.connect_FM_blocks()
    tb.run()


if __name__ == '__main__':
    main()
