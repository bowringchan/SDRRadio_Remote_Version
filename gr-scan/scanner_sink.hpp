/*
	gr-scan - A GNU Radio signal scanner
	Copyright (C) 2017 Bowringchan <bowringchan@gmail.com>. All Rights Reserved.
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
#include <stdio.h>
#include <ctime>
#include <set>
#include <utility>

#include <boost/shared_ptr.hpp>

#include <gnuradio/block.h>
#include <gnuradio/io_signature.h>
#include <osmosdr/source.h>
#include <fstream>


class scanner_sink : public gr::block
{
public:
	scanner_sink(osmosdr::source::sptr source, unsigned int vector_length, double centre_freq_1,
		     double centre_freq_2, double bandwidth0, double bandwidth1, double bandwidth2,
		     double step, unsigned int avg_size, double spread, double threshold, double ptime,
		     const std::string &outcsv, float noise_line, int freq_shift):
		gr::block("scanner_sink",
			  gr::io_signature::make(1, 1, sizeof (float) * vector_length),
			  gr::io_signature::make(0, 0, 0)),
		m_source(source), //We need the source in order to be able to control it
		m_buffer(new float[vector_length]), //buffer into which we accumulate the total for averaging
		m_vector_length(vector_length), //size of the FFT
		m_count(0), //number of FFTs totalled in the buffer
		m_wait_count(0), //number of times we've listenned on this frequency
		m_avg_size(avg_size), //the number of FFTs we should average over
		m_step(step), //the amount by which the frequency shold be incremented
		m_centre_freq_1(centre_freq_1), //start frequency (and then current frequency)
		m_centre_freq_2(centre_freq_2), //end frequency
		m_bandwidth0(bandwidth0), //samples per second
		m_bandwidth1(bandwidth1), //fine window (band)width
		m_bandwidth2(bandwidth2), //coarse window (band)width
		m_threshold(threshold), //threshold in dB for discovery
		m_spread(spread), //minumum distance between radio signals (overlapping scans might produce slightly different frequencies)
		m_time(ptime), //the amount of time to listen on the same frequency for
		m_start_time(time(0)), //the start time of the scan (useful for logging/reporting/monitoring)
		m_outcsv(NULL),
        m_noise_line(noise_line),
        m_freq_shift(freq_shift)
	{
		/* testing start*/
		// max_buffer = new float[m_vector_length];
		// for(unsigned int i=0; i<m_vector_length; i++){
			// max_buffer[i] = -140.0;
		// }
		/* testing end*/
		/* testing start*/
		/* testing end*/
		m_source -> set_gain_mode(false);
		m_source -> set_gain(30);
		m_source -> set_if_gain(30);
		m_source -> set_bb_gain(30);
		ZeroBuffer();
        remove("static/power2display.csv");
		m_outcsv = fopen(outcsv.c_str(), "w");
        if (!m_outcsv) {
            fprintf(stderr, "[-] Error opening output CSV file %s\n", outcsv.c_str());
            exit(1);
        }

        /* Write XML Header */
        fprintf(m_outcsv, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Scanner_Result>\n");
        fflush(m_outcsv);
	}

	virtual ~scanner_sink()
	{
		delete []m_buffer; //delete the buffer
		/* testing start*/
		// delete []max_buffer;
		/* testing end*/
        
	}

private:
	virtual int general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
	{
		for (int i = 0; i < ninput_items[0]; ++i)
			ProcessVector(static_cast<const float *>(input_items[0]) + i * m_vector_length);

		consume_each(ninput_items[0]);
		return 0;
	}

	void ProcessVector(const float *input)
	{
		//Add the FFT to the total
		for (unsigned int i = 0; i < m_vector_length; ++i){
			m_buffer[i] += input[i];
			/* testing start*/
			// if(input[i] > max_buffer[i]){
				// max_buffer[i] = input[i];
			// }
			/* testing end*/
		}
		++m_count; //increment the total

		if (m_avg_size != m_count)
			return; //we haven't yet averaged over the number we intended to


		double freqs[m_vector_length]; //for convenience
		float bands0[m_vector_length]; //bands in order of frequency
		float bands1[m_vector_length]; //fine window bands
		float bands2[m_vector_length]; //coarse window bands

		Rearrange(bands0, freqs, m_centre_freq_1, m_bandwidth0); //organise the buffer into a convenient order (saves to bands0)
		/* testing start*/
		std::ofstream ofstream_power("static/power2display.csv",std::ofstream::out|std::ofstream::app);
		if(ofstream_power.is_open()){
            std::cout<<m_freq_shift<<std::endl;
            if(m_freq_shift<=0.1){
                for(unsigned int i=0;i<m_vector_length;i++){
                    ofstream_power << freqs[i]/1000000<<","<< bands0[i] << std::endl;
                }
            }else{
                for(unsigned int i=0;i<m_vector_length;i++){
                    ofstream_power << (freqs[i]-100000000)/1000000<<","<< bands0[i] << std::endl;
                }
            }
		}
		ofstream_power.close();
		/* testing end*/
		GetBands(bands0, bands1, m_bandwidth1); //apply the fine window (saves to bands1) //滑动平均的结果
		GetBands(bands0, bands2, m_bandwidth2); //apply the coarse window (saves to bands2)//bandwidth2默认为8倍的bandwidth1
		
		/* testing start*/
		// std::ofstream ofstream_max_buffer("max_buffer.txt");
		// if(ofstream_max_buffer.is_open()){
			// for(unsigned int i=0;i<m_vector_length/2;i++){
				// ofstream_max_buffer << max_buffer[i+1000] << std::endl;
			// }
			// for(unsigned int i=m_vector_length/2;i<m_vector_length;i++){
				// ofstream_max_buffer << max_buffer[i-1000] << std::endl;
			// }
		// }
		// ofstream_max_buffer.close();
		/*
		std::ofstream ofstream_bands2("bands2.txt");
		if(ofstream_bands2.is_open()){
			for(unsigned int i=0;i<m_vector_length;i++){
				ofstream_bands2 << bands2[i] << std::endl;
			}
		}
		ofstream_bands2.close();
		exit(0);
		*//* testing end*/
		
		
		
		
		PrintSignals(freqs, bands0, bands1, bands2);
		//准备新的一次扫描
		
		m_count = 0; //next time, we're starting from scratch - so note this
		ZeroBuffer(); //get ready to start again

		++m_wait_count; //we've just done another listen
		//if (m_time / (m_bandwidth0 / static_cast<double>(m_vector_length * m_avg_size)) <= m_wait_count) { //if we should move to the next frequency 希望覆盖到所有的点
		if (1) { 
            for (;;) { //keep moving to the next frequency until we get to one we can listen on (copes with holes in the tunable range)
				if (m_centre_freq_2 <= m_centre_freq_1) { //we reached the end!
					//do something to end the scan
					fprintf(stderr, "[*] Finished scanning\n"); //say we're exiting
				
                    fprintf(m_outcsv, "</Scanner_Result>\n");
                    fflush(m_outcsv);
                    if (m_outcsv)
                        fclose(m_outcsv);
                   
                    exit(0); //TODO: This probably isn't the right thing, but it'll do for now
				
                }

				m_centre_freq_1 += m_step; //calculate the frequency we should change to
				double actual = m_source->set_center_freq(m_centre_freq_1); //change frequency
				if ((m_centre_freq_1 - actual < 10.0) && (actual - m_centre_freq_1 < 10.0)) //success
					break; //so stop changing frequency
			}
			m_wait_count = 0; //new frequency - we've listened 0 times on it
		}
	}
	
	//freqs是每个采样点对应的频率
	//bands1是细滑动平均的结果
	//bands2是粗滑动平均的结果
	void PrintSignals(double *freqs, float * bands0, float *bands1, float *bands2)
	{
		/* Calculate the current time after start */
		unsigned int t = time(NULL) - m_start_time;
		unsigned int hours = t / 3600;
		unsigned int minutes = (t % 3600) / 60;
		unsigned int seconds = t % 60;

		//Print that we finished scanning something
		fprintf(stderr, "%02u:%02u:%02u: Finished scanning %f MHz - %f MHz\n",
			hours, minutes, seconds, (m_centre_freq_1 - m_bandwidth0 / 2.0) / 1000000.0, (m_centre_freq_1 + m_bandwidth0 / 2.0) / 1000000.0);
									//扫描的频段，[下边界,上边界]
		/* Calculate the differences between the fine and coarse window bands */
		float diffs[m_vector_length];//差[数学]
		for (unsigned int i = 0; i < m_vector_length; ++i)
			diffs[i] = bands1[i] - bands2[i];//bands2是粗滑动平均的结果,bands2[0]应该永远比bands1[0]大
        
        std::ofstream ofstream_diff("diff.txt");
		if(ofstream_diff.is_open()){
			for(unsigned int i=0;i<m_vector_length;i++){
				ofstream_diff << diffs[i] << std::endl;
			}
		}
		ofstream_diff.close();
        
		/* Look through to find signals */
		//start with no signal found (note: diffs[0] should always be very negative because of the way the windowing function works)
		bool sig = false;
		unsigned int peak = 0;
		for (unsigned int i = 0; i < m_vector_length; ++i) {
			if (sig) { //we're already in a signal
			//细滑动平均相比于粗滑动平均，更能凸显峰值。两者做差即可得到峰值点。这就是此处寻找信号的机制。
				if (diffs[peak] < diffs[i]) //we found a rough end to the signal
					peak = i;//peak是bands1和bands2之差的峰值点
				
				// if (diffs[i] < m_threshold) { //we're transitionning to the end
					// /* look for the "start" of the signal 从峰值点开始，向左找信号频域的左边界，向右找的信号频域的右边界*/
					
					// unsigned int min = peak; //scan outwards for the minimum
					// while ((diffs[min] > diffs[peak] - 3.0) && (min > 0)){ //while the signal is still more than half power
						// min--;
					// }

					// /* look for the "end" */
					// unsigned int max = peak;
					// while ((diffs[max] > diffs[peak] - 3.0) && (max < m_vector_length - 1))
						// max++;
				   if (diffs[i] < m_threshold){
					   unsigned int min = peak;
					   while ((bands0[min] > m_noise_line) && (min > 0)){
						   min--;
					   }   
					   unsigned int max = peak;
					   while ((bands0[max] > m_noise_line) && (max < m_vector_length - 1))
						 max++;
					sig = false; //we're now in no signal state

					/* Print the signal if it's a genuine hit */ 
					if (TrySignal(freqs[max], freqs[min])) {
						fprintf(stderr, "[+] %02u:%02u:%02u: Found signal: at %f MHz of width %f kHz, peak power %f dB (difference %f dB)\n",
							hours, minutes, seconds, (freqs[max] + freqs[min]) / 2000000.0, (freqs[max] - freqs[min])/1000.0, bands1[peak], diffs[peak]);
						// WriteCSV((freqs[max] + freqs[min]) / 2000000.0,
							 // (freqs[max] - freqs[min])/1000.0,
							 // bands1[peak], diffs[peak]);//(频率最大值+频率最小值)/2= 中心频率
                             WriteXML((freqs[max] + freqs[min])/2,(freqs[max] - freqs[min]),bands1[peak]);
					}
				}
			}
			else if (diffs[i] >= m_threshold) { //we found a signal!
				peak = i;
				sig = true;
			}
		}
	}
	//看看这个信号是不是太靠近中心频率
	bool TrySignal(double max, double min)
	{
		double mid = (min + max) / 2.0; //calculate the midpoint of the signal

		/* check to see if the signal is too close to the centre frequency (a signal often erroniously appears there) */
		if ((mid - m_centre_freq_1 < m_spread) && (m_centre_freq_1 - mid < m_spread))
			return false; //if so, this is not a genuine hit 太靠近啦

		/* check to see if the signal is close to any other (the same signal often appears with a slightly different centre frequency) */
		BOOST_FOREACH (double signal, m_signals){
			if ((mid - signal < m_spread) && (signal - mid < m_spread)) //tpo close
				return false; //if so, this is not a genuine hit
		}//使用BOOST_FOREACH方便地迭代STL容器
        
        /* check if the bandwidth is too narrow(too close to noise line) */
        if(max - min < 1000)
            return false;
		/* Genuine hit!:D */
		m_signals.insert(mid); //add to the set of signals
		return true; //genuine hit
	}
	//bands是FFT后的频谱，采样点中心为0，定义域 [-bandwidth/2.0, bandwidth/2.0)对应[-xxhz,0,+xxhz)的采样点
	//freqs是每个采样点对应的频率
	//centre是0hz点对应的频率
	//bandwidth是采样点个数/秒(基带采样定理)
	void Rearrange(float *bands, double *freqs, double centre, double bandwidth)
	{
		double samplewidth = bandwidth/(double)m_vector_length;
		for (unsigned int i = 0; i < m_vector_length; ++i) {
			/* FFT is arranged starting at 0 Hz at the start, rather than in the middle */
			//原FFT的上下频带对换位置
			if (i < m_vector_length / 2) //lower half of the fft
				bands[i + m_vector_length / 2] = m_buffer[i] / static_cast<float>(m_avg_size);
			else //upper half of the fft
				bands[i - m_vector_length / 2] = m_buffer[i] / static_cast<float>(m_avg_size);

			freqs[i] = centre + i * samplewidth - bandwidth / 2.0; //calculate the frequency of this sample
		}
	}
	//power: 频谱向量的和
	//bands: 想要得到的一小段频谱，加权滑动平均的结果
	//bandwidth: bands对应的带宽
	void GetBands(float *powers, float *bands, unsigned int bandwidth)
	{
		double samplewidth = m_bandwidth0 / static_cast<double>(m_vector_length); //the width in Hz of each sample
		unsigned int bandwidth_samples = bandwidth / samplewidth; //the number of samples in our window(想要得到的一小段频谱上有多少个点)
		for (unsigned int i = 0; i < m_vector_length; ++i) //we're averaging, so start with 0
			bands[i] = 0.0;

		for (unsigned int i = 0; i < m_vector_length; ++i){ //over the entire FFT
			//make the buffer contains the entire window
			//范围: bands [0,bandwidth]到[m_vector_length-bandwidth, m_vector_length],以bandwidth为窗大小滑动
			if ((i >= bandwidth_samples / 2) && (i < m_vector_length + bandwidth_samples / 2 - bandwidth_samples)){
				for (unsigned int j = 0; j < bandwidth_samples; ++j) //iterate over the window for averaging 滑动平均
					bands[i + j - bandwidth_samples / 2] += powers[i] / static_cast<float>(bandwidth_samples); //add this sample to the bands
			}
		}
	}

	void ZeroBuffer()
	{
		/* writes zeros to m_buffer */
		for (unsigned int i = 0; i < m_vector_length; ++i)
			m_buffer[i] = 0.0;
	}

	void WriteCSV(float freq, float width, float peak, float diff)
	{
		time_t timer;
		struct tm *tm_info;
		char buf[26];

		if (!m_outcsv)
			return;

		time(&timer);
		tm_info = localtime(&timer);
		strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm_info);
		fprintf(m_outcsv, "%s,%f,%f,%f,%f\n", buf, freq, width, peak, diff);
		fflush(m_outcsv);
	}
    
    void WriteXML(float freq, float width, float peak){
        if(m_freq_shift<0.1){
            fprintf(m_outcsv, "<program><freq>%.0f</freq><bandwidth>%.0f</bandwidth><peak>%f</peak><mcs>FM</mcs></program>\n", freq, width, peak);
            fflush(m_outcsv);
        }else{
            fprintf(m_outcsv, "<program><freq>%.0f</freq><bandwidth>%.0f</bandwidth><peak>%f</peak><mcs>AM</mcs></program>\n", freq - 100000000, width, peak);
            fflush(m_outcsv);
        } 
    }
	std::set<double> m_signals;
	osmosdr::source::sptr m_source;
	float *m_buffer;
	unsigned int m_vector_length;
	unsigned int m_count;
	unsigned int m_wait_count;
	unsigned int m_avg_size;
	double m_step;
	double m_centre_freq_1;
	double m_centre_freq_2;
	double m_bandwidth0;
	double m_bandwidth1;
	double m_bandwidth2;
	double m_threshold;
	double m_spread;
	double m_time;
	time_t m_start_time;
	FILE *m_outcsv;
	// float *max_buffer;
	float m_noise_line;
    float m_freq_shift;
};

/* Shared pointer thing gnuradio is fond of */
typedef boost::shared_ptr<scanner_sink> scanner_sink_sptr;
scanner_sink_sptr make_scanner_sink(osmosdr::source::sptr source, unsigned int vector_length, double centre_freq_1, double centre_freq_2, double bandwidth0, double bandwidth1, double bandwidth2, double step, unsigned int avg_size, double spread, double threshold, double ptime, const std::string &outcsv,float noise_line,int freq_shift)
{
	return boost::shared_ptr<scanner_sink>(new scanner_sink(source, vector_length, centre_freq_1, centre_freq_2, bandwidth0, bandwidth1, bandwidth2, step, avg_size, spread, threshold, ptime, outcsv, noise_line, freq_shift));
}
