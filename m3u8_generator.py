# -*- coding: utf-8 -*-
from threading import Timer

class M3u8_Generator:
    def __init__(self,EXTINF):
        self.EXTINF = EXTINF

    def generate(self,output_list,media_sequence):
        m3u8file = open('RTLSDR.m3u','w')
        m3u8file.write('#EXTM3U\n'+'#EXT-X-TARGETDURATION:'+str(self.EXTINF)+'\n'\
                       '#EXT-X-VERSION:3\n'+'#EXT-X-MEDIA-SEQUENCE:'+str(media_sequence)+'\n')
        for list_item in output_list:
            m3u8file.write('#EXTINF:'+str(self.EXTINF)+','+'\n'+list_item+'\n')
        m3u8file.close()
