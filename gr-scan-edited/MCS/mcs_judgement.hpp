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

#include <ctime>
#include <set>
#include <utility>
#include <cmath>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include <gnuradio/block.h>
#include <gnuradio/io_signature.h>
#include <osmosdr/source.h>
#include <complex.h>
#include <fftw3.h>


class mcs_judgement : public gr::block
{
public:
	mcs_judgement(osmosdr::source::sptr source, unsigned int vector_length, double centre_freq_1,
		double bandwidth0,unsigned int avg_size):
		gr::block("mcs_judgement",
			gr::io_signature::make(3, 3, sizeof (float) * vector_length),
			gr::io_signature::make(0, 0, 0)),
			m_source(source),
			m_vector_length(vector_length),//size of the FFT
			m_buffer_fft(new float[vector_length]), //buffer into which we accumulate the total for averaging
			m_buffer_ifft(new float[vector_length]), //buffer into which we accumulate the total for averaging
			m_buffer_phase(new float[vector_length]),//buffer into which we accumulate the total for averaging
			m_centre_freq_1(centre_freq_1), //center frequency; "fc"
			m_bandwidth0(bandwidth0), //samples per second 采样率 "fs"
			m_avg_size(avg_size),
			m_count(0)
			{

			}
	virtual ~mcs_judgement(){
		delete []m_buffer_fft; //delete the buffer
		delete []m_buffer_ifft; //delete the buffer
		delete []m_buffer_phase;//delete the buffer
	}

private:
	virtual int general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
	{
		for (int i = 0; i < ninput_items[0]; ++i)
			ProcessVector(static_cast<const float *>(input_items[0]) + i * m_vector_length,
			static_cast<const float *>(input_items[1]) + i * m_vector_length,
			static_cast<const float *>(input_items[2]) + i * m_vector_length);

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
		
		std::cout << "Step into here: m_count" << std::endl;
		if (m_avg_size != m_count){
			return; //we haven't yet averaged over the number we intended to
		}
		double freqs[m_vector_length]; //for convenience
		float bands0[m_vector_length]; //bands in order of frequency
		float ifft0[m_vector_length]; // ifft after averageing
		float phase0[m_vector_length]; //phase after averageing
		Rearrange(bands0, freqs, m_centre_freq_1, m_bandwidth0); //organise the buffer into a convenient order (saves to bands0)
		//Rearrange包含了平均操作

		
		avg_ifft_phase(ifft0, phase0);
		int mcs_type = 0; 
		//mcs_type ==1 FM
		//mcs_type ==2 AM;
		std::cout << "Step into here: StartJudgement" << std::endl;
		StartJudgement(&mcs_type,freqs,bands0,ifft0,phase0);
		std::cout<< "\n\n\nmcs_type: is "<<mcs_type << "\nmcs_type ==1 FM\nmcs_type ==2 AM\n";
		exit(0);
	}
	
	
	/*用于判定信号的调制方式
	* 杨小牛院士《软件无线电原理与应用》P202
	* freqs是每个采样点对应的频率 
	* bands0是从负频率到正频率的整个频谱，中间点为中心频率。注意，频谱对称吗？不对称,使用的是complex to mag
	* ifft0是采样点(浮点数，假设做了complex to mag, squarted)
	*/
	void StartJudgement(int* mcs_type,double *freqs, float *bands0,float* ifft0, float* phase0){
		float z[m_vector_length];//实信号的瞬时幅度
		float ma = 0;//瞬时(绝对)幅度的平均值
		float y2[m_vector_length];//幅度比,即为文献中的an(i)
		float y3[m_vector_length];//归一化瞬时幅度
		double y2n[m_vector_length];//即为文献中的acn(i)
		double s[m_vector_length];
		float y4 = 0;//y3的绝对值的最大值
		double R[m_vector_length];
		double Rmax = 0; //零中心归一化瞬时幅度的谱密度的最大值
		float Ck[m_vector_length];

		const float pi = 3.1415926; // 相位的单位
		const int m = m_vector_length -1;
		float phase1[m_vector_length];
		float phasen1[m_vector_length];
		float phaseabs[m_vector_length];
		float phasesquare[m_vector_length];
		  
		const float at=1; //判决门限电平
		float a =0; //非线性相位之平方的和
		float b = 0; //非线性相位之绝对值的和
		float d = 0; //非线性相位的和
		float c = 0; //计数器，计算有多少个采样点，幅度的绝对值是大于判决门限的
		
		
		const float deltaapt = pi/6.5; 
		const float deltadpt = pi/6;
		float a1; //非线性相位之平方的和 取平均
		float b1;	//非线性相位之绝对值的和 求平均再求平方
		float d1;	//非线性相位的和 求平均再求平方
		float deltaap;    //零中心非弱信号段瞬时相位非线性分量绝对值的标准偏差
		float deltadp;		//零中心非弱信号段瞬时相位非线性分量的标准偏差
		
		const double pt = 0.5;
		double pl = 0; //负频率频谱幅度的平方，再求和
		double pu = 0; //正频率频谱幅度的平方，再求和
		double pp = 0;
		
		
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
		  std::cout << "Step into here: fftw_plan_r2r_1d" << std::endl;
		  //s=fft(y2n);做fft变换
		fftw_plan my_plan;
		my_plan = fftw_plan_r2r_1d(m_vector_length, y2n, s, FFTW_R2HC, 0);
		fftw_execute(my_plan);
		
		fftw_destroy_plan(my_plan);
		  std::cout << "Step out here: fftw_plan_r2r_1d" << std::endl;
		//s数组的格式:r0, r1, r2, ..., rn/2, i(n+1)/2-1, ..., i2, i1; r是实部,i是虚部
		for(unsigned int i=0; i< m_vector_length/2; ++i){
			R[i] = std::abs(s[i]*s[i]+s[m_vector_length-i]*s[m_vector_length-i]); //此处和matlab代码不同，没有开根号
			 if(R[i] > Rmax){
				Rmax = R[i];
			}
		}
		Rmax = Rmax * Rmax; //零中心归一化瞬时幅度的谱密度的最大值
		  
		if(phase0[1] - phase0[0] > pi){
				   Ck[0]=-2*pi;
			}else if(phase0[0] - phase0[1] > pi){
				   Ck[0]=2*pi;
			}else{
				 Ck[0]=0;
			}
		
		for(unsigned int i=1; i< m_vector_length-1; ++i){
			if(phase0[i+1] - phase0[i] > pi){
				Ck[i] = Ck[i-1] - 2*pi;
			}else if(phase0[i]-phase0[i+1]>pi){
				Ck[i]=Ck[i-1]+2*pi;
			}else{
				Ck[i]=Ck[i-1];
			}
		}
		  
		  if(-phase0[m] > pi){
			  Ck[m]=Ck[m-1]-2*pi;
		  }else if(phase0[m] > pi){
			  Ck[m]=Ck[m-1]+2*pi;
		  }else{
			  Ck[m]=Ck[m-1];
		  }
		  
		  for(unsigned int i=1; i< m_vector_length-1; ++i){
			  
			  //phase1[i] = phase0[i] + Ck[i]; //去相位卷叠后的相位序列
			  //phasen1[i] = phase1[i] +2*pi*m_centre_freq_1*i/m_bandwidth0; //phasen1 = phase1-2*pi*fc*i/fs;
			  phasen1[i] = phase0[i];
		  }
		  
		  for(unsigned int i =0; i< m_vector_length; ++i){
			  if(y2[i] > at){
				  c=c+1;
				  phasesquare[i] = phasen1[i]*phasen1[i]; //非线性相位的平方
				  a=a+phasesquare[i]; //非线性相位之平方的和
				  phaseabs[i]=std::abs(phasen1[i]); //非线性相位的绝对值
				  b=b+phaseabs[i]; //非线性相位之绝对值的和
				  d=d+phasen1[i];    //非线性相位的和
			  }
			  
		  }
		  a1 = a/c; //非线性相位之平方的和 取平均
		  b1 = (b/c)*(b/c);	//非线性相位之绝对值的和 求平均再求平方
		  d1=(d/c)*(d/c);	//非线性相位的和 求平均再求平方
		  deltaap = std::sqrt(a1-b1);    //零中心非弱信号段瞬时相位非线性分量绝对值的标准偏差
		  deltadp=std::sqrt(a1-d1);    //零中心非弱信号段瞬时相位非线性分量的标准偏差
		  
		  
		  for(unsigned int i =0; i< m_vector_length/2; ++i){
			  pl = bands0[i] * bands0[i];
		  }
		  for(unsigned int i =m_vector_length/2; i< m_vector_length; ++i){
			  pu = bands0[i] * bands0[i];
		  }

		  pp = (pl - pu)/(pl + pu);
		  //判定过程啦，假设现在只需要判定AM,FM,则只需要对deltadp做判定
		  std::cout << "Step into here: if(deltadp > deltadpt)" << std::endl;
		  if(deltadp > deltadpt){
			  *mcs_type = 1; //FM
		  }else{
			  *mcs_type = 2; //AM
		  }
		  std::cout << "\n\ndeltadp is: " << deltadp << std::endl;
		  std::cout << "\n\na1 is: " << a1 << std::endl;
		  std::cout << "\n\nd1 is: " << d1 << std::endl;
		  std::cout << "\n\nphasen1 0 1 2 3 4 is: " << phasen1[0] << std::endl << phasen1[1] << std::endl << phasen1[2] << std::endl <<phasen1[3] <<std::endl<< phasen1[4]<< std::endl;
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
	
	osmosdr::source::sptr m_source;
	unsigned int m_vector_length;
	float *m_buffer_fft;
	float *m_buffer_ifft;
	float *m_buffer_phase;
	double m_centre_freq_1;
	double m_bandwidth0;
	unsigned int m_avg_size;
	unsigned int m_count;
	
};

/* Shared pointer thing gnuradio is fond of */
typedef boost::shared_ptr<mcs_judgement> mcs_judgement_sptr;
mcs_judgement_sptr make_mcs_judgement(osmosdr::source::sptr source, unsigned int vector_length, double centre_freq_1,
		     double bandwidth0,unsigned int avg_size)
{
	return boost::shared_ptr<mcs_judgement>(new mcs_judgement(source, vector_length, centre_freq_1,
		     bandwidth0,avg_size));
}
