/*

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

#include <ctime>
#include <set>
#include <utility>
#include <cmath>

#include <boost/shared_ptr.hpp>

#include <gnuradio/block.h>
#include <gnuradio/io_signature.h>
#include <osmosdr/source.h>


class mcs_judgement : public gr::block
{
public:
	mcs_judgement():
	gr::block("mcs_judgement",
			  gr::io_signature::make(3, 3, sizeof (float) * vector_length),
			  gr::io_signature::make(0, 0, 0)),
			  m_vector_length(vector_length),//size of the FFT
			  m_buffer_fft(new float[vector_length]), //buffer into which we accumulate the total for averaging
			  m_buffer_ifft(new float[vector_length]), //buffer into which we accumulate the total for averaging
			  m_buffer_phase(new float[vector_length]),//buffer into which we accumulate the total for averaging
			  m_centre_freq_1(centre_freq_1), //start frequency (and then current frequency)
			  m_bandwidth0(bandwidth0), //samples per second
			  {
			  }
	virtual ~mcs_judgement(){
		delete []m_buffer_ifft; //delete the buffer
		delete []m_buffer_fft; //delete the buffer
		delete []m_buffer_phase;//delete the buffer
	}

private:
	virtual int general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
	{
		for (int i = 0; i < ninput_items[0]; ++i)
			ProcessVector(static_cast<const float *>(input_items[0]) + i * m_vector_length,
			static_cast<const float *>(input_items[1]) + i * m_vector_length,
			static_cast<const float *>(input_items[0]) + i * m_vector_length);

		consume_each(ninput_items[0]);//0#接口里的是fft处理过的频域点
		consume_each(ninput_items[1]);//1#接口里的是没有经过fft处理的数据的幅度
		consume_each(ninput_items[2]);//1#接口里的是没有经过fft处理的数据相位
		return 0;
	}
	
	void ProcessVector(const float *fft_input, const float* ifft_input,const float* phase_input )
	{
		//Add the FFT to the total
		for (unsigned int i = 0; i < m_vector_length; ++i)
		{
			m_buffer_fft[i] += fft_input[i];
			m_buffer_ifft[i] += ifft_input[i];
			m_buffer_phase[i] += phase_input[i];
		}
		++m_count; //increment the total

		if (m_avg_size != m_count)
			return; //we haven't yet averaged over the number we intended to
		double freqs[m_vector_length]; //for convenience
		float bands0[m_vector_length]; //bands in order of frequency
		float ifft0[m_vector_length]; // ifft after averageing
		float phase0[m_vector_length]; //phase after averageing
		Rearrange(bands0, freqs, m_centre_freq_1, m_bandwidth0); //organise the buffer into a convenient order (saves to bands0)
		//Rearrange包含了平均操作
		avg_ifft_phase(ifft0, phase0);
		StartJudgement(freqs,bands0,ifft0,phase0);
		
	}
	
	/*用于判定信号的调制方式
	* 杨小牛院士《软件无线电原理与应用》P202
	* freqs是每个采样点对应的频率 
	* bands0是从负频率到正频率的整个频谱，中间点为中心频率。注意，频谱对称吗？不对称,使用的是complex to mag
	* ifft0是采样点(浮点数，假设做了complex to mag, squarted)
	*/
	void StartJudgement(double *freqs, float *bands0 float* ifft0, float* phase0){
		  float z[m_vector_length];//实信号的瞬时幅度
		  float ma = 0;//瞬时(绝对)幅度的平均值
		  float y2[m_vector_length];//幅度比,即为文献中的an(i)
		  float y3[m_vector_length];//归一化瞬时幅度
		  float y2n[m_vector_length];//即为文献中的acn(i)
		  float y4 = 0;//y3的绝对值的最大值
		　for(unsigned int i=0; i< m_vector_length; ++i){
			z[i] = std::abs(ifft0[i]);
			ma += (z[i]/m_vector_length);
		  }
		  for(unsigned int i=0; i< m_vector_length; ++i){
			  y2[i] = z[i]/ma;
			  y3[i] = y2[i]-1;
			  float absy3i = std::abs(y3[i]);
			  if(absy3i>y4){
				  y4 = absy3i;
			  }
		  }
		  for(unsigned int i=0; i< m_vector_length; ++i){
			  y2n[i] = y3[i]/y4;
		  }
		  //完成此句　s=fft(y2n);需要做fft变换
	}
		
	void avg_ifft_phase(float* ifft, float* phase){
		for(unsigned int i=0; i< m_vector_length; ++i){
			ifft[i] = m_buffer_ifft[i] / static_cast<float>(m_avg_size);
			phase[i] = m_buffer_phase[i] / static_cast<float>(m_avg_size);
		}
	}
	
	void Rearrange(float *bands, double *freqs, double centre, double bandwidth)
	{
		double samplewidth = bandwidth/(double)m_vector_length;
		for (unsigned int i = 0; i < m_vector_length; ++i) {
			/* FFT is arranged starting at 0 Hz at the start, rather than in the middle */
			//原FFT的上下频带对换位置
			if (i < m_vector_length / 2) //lower half of the fft
				bands[i + m_vector_length / 2] = m_buffer_fft[i] / static_cast<float>(m_avg_size);
			else //upper half of the fft
				bands[i - m_vector_length / 2] = m_buffer_fft[i] / static_cast<float>(m_avg_size);

			freqs[i] = centre + i * samplewidth - bandwidth / 2.0; //calculate the frequency of this sample
		}
	}
	
	unsigned int m_vector_length;
	float *m_buffer_ifft;
	float *m_buffer_fft;
	float *m_buffer_phase;
	double m_bandwidth0;
	double m_centre_freq_1;
}