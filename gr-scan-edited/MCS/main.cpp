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


#include <cstdio>
#include <cstdlib>
#include <string>
#include "topblock.hpp"

int main(int argc, char **argv)
{
	unsigned int vector_length = 20000;//vector_length = sample_rate / FFT_width
	double centre_freq_1 = std::atof(argv[1]);//center_frequency
	double bandwidth0 = 2000000;//sample_rate
	double signal_bandwidth = std::atof(argv[2]); // 信号带宽是指中心频率到信号右边界的带宽
	double m_avg_size = std::atoi(argv[3]);;//用于平均的frame的个数
	std::string device_args="";
	if(std::atoi(argv[4]) == 0){
	    
	}else{
	    device_args = argv[4];
	}
	
	TopBlock top_block(vector_length,
			   centre_freq_1,
			   bandwidth0,
			   signal_bandwidth,
			   signal_bandwidth/4,
			   m_avg_size,
			   device_args
			   );
			  
	top_block.run();
	return 0; //actually, we never get here because of the rude way in which we end the scan
}
