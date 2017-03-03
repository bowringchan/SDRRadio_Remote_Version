/*
	gr-mcs_judgement - A GNU Radio modulation scheme jugement tool
	Copyright (C) 2017 Bowring Chan <bowringchan@gmail.com>. All Rights Reserved.
	Copyright (C) 2015 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
	Copyright (C) 2012  Nicholas Tomlinson

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <stdint.h>

#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/fft/fft_vcc.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include "mcs_judgement.hpp"

class TopBlock : public gr::top_block
{
public:
	TopBlock(unsigned int vector_length,double centre_freq_1,
		     double bandwidth0, double cutoff_freq,double transition_width,unsigned int avg_size):
		gr::top_block("Top Block"),
		window(GetWindow(vector_length)),
		source(osmosdr::source::make()),
		lps(gr::filter::firdes::low_pass::make(1,bandwidth0,cutoff_freq,transition_width,WIN_BLACKMAN))
		stv(gr::blocks::stream_to_vector::make(sizeof(float) * 2, vector_length)),
		fft(gr::fft::fft_vcc::make(vector_length, true, window, false, 1)),
		ctmp(gr::blocks::complex_to_magphase::make(vector_length)),
		iir(gr::filter::single_pole_iir_filter_ff::make(1.0, vector_length)),
		sink(make_mcs_judgement(source, vector_length, centre_freq_1,
		     bandwidth0,avg_size))
			 {
				 /* Set up the OsmoSDR Source */
				  source->set_sample_rate(sample_rate);
				  source->set_freq_corr(0.0);
				  source->set_center_freq(m_centre_freq_1)
				  source -> set_gain_mode(false);
				  source -> set_gain(30);
				  source -> set_if_gain(30);
				  source -> set_bb_gain(30);
				  
				  
				/* Set up the connections */
				  connect(source, 0, lps, 0);
				  connect(lps, 0,stv, 0)
				  connect(stv, 0, fft, 0);
				  connect(fft, 0, ctmp, 0);
				  connect(ctmp, 0, iir, 0);
				  connect(iir, 0, sink, 0);
				  connect(stv, 0, sink, 1);
				  connect(ctmp, 1, sink, 2);
				 
			 }
private:
	/* http://en.wikipedia.org/w/index.php?title=Window_function&oldid=508445914 */
	std::vector<float> GetWindow(size_t n)
	{
		std::vector<float> w;
		w.resize(n);
		//Blackman windows
		double a = 0.16;
		double a0 = (1.0 - a)/2.0;
		double a1 = 0.5;
		double a2 = a/2.0;

		for (unsigned int i = 0; i < n; ++i)
			w[i] = a0 - a1 * ::cos((2.0 * 3.14159 * static_cast<double>(i))/static_cast<double>(n - 1)) + a2 * ::cos((4.0 * 3.14159 * static_cast<double>(i))/static_cast<double>(n - 1));
		return w;
	}

	double GetWindowPower()
	{
		double total = 0.0;
		BOOST_FOREACH (double d, window)
			total += d * d;
		return total;
	}	
	

	std::vector<float> window;
	gr::filter::firdes::low_pass lps;
	osmosdr::source::sptr source;
	gr::blocks::stream_to_vector::sptr stv;
	gr::fft::fft_vcc::sptr fft;
	gr::blocks::complex_to_magphase::sptr ctmp;
	gr::filter::single_pole_iir_filter_ff::sptr iir;
	mcs_judgement_sptr sink;
}