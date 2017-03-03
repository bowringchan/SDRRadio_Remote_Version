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

#include "topblock.hpp"

int main(int argc, char **argv)
{
	unsigned int vector_length = 2000;//vector_length = sample_rate / FFT_width
	double centre_freq_1 = 100700000;//center_frequency
	double bandwidth0 = 2000000;//sample_rate
	double m_avg_size = 5;//用于平均的frame的个数
	
	TopBlock top_block(vector_length,
			   centre_freq_1,
			   bandwidth0,
			   1/2*bandwidth0,
			   m_avg_size);
			  
	top_block.run();
	return 0; //actually, we never get here because of the rude way in which we end the scan
}
