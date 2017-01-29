# -*- coding: utf-8 -*-

class m3u8_generator:
    def generate(self,output_list,EXTINF,media_sequence):
        m3u8file = open(r'audio\RTLSDR.m3u','w')
        m3u8file.write(r'#EXTM3U\n'+r'#EXT-X-TARGETDURATION:10\n'\
                       r'#EXT-X-VERSION:3'+r'#EXT-X-MEDIA-SEQUENCE:'+media_sequence+r'\n')
        for list_item in output_list:
            m3u8file.write('#EXTINF:'+EXTINF+','+r'\n'+list_item+r'\n')
        m3u8file.close()