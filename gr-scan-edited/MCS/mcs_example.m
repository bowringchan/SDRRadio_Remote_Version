close all;
fc=20000; %载波频率
fs=40000; %采样速率
k=2;
%code_size=15*round(k*fs/fc);             %信息码元长度
t0=5.5;                               %信号长度，即输入信号的时长，单位秒
Ns=256;                              %采样点个数
%fd=125;                              %符号速率
ts=1/fs;                               %采样周期
%M=64                                %码元个数
%ti=1/fd;                               %码元间隔
N=ti/ts;
t=[0:ts:t0];						%时间轴
fileID=fopen('D:\modustring5.bnt');	%打开文件
info=fread(fileID);					%获得信号

for n=1:1:50	%总共50帧
    m=n*Ns;		%第#帧的最后一个采样点
    x=(n-1)*Ns;	%第#帧的第一个采样点之前的点
for i=x+1:m;                                  %提取信号段，依次迭代，获得每个采样点的信息
    y0(i)=info(i);
end
Y=fft(y0);                                    %调制信号的傅立叶变换
y1=hilbert(y0);                                %实信号的解析式 
z=abs(y0);                                    %实信号的瞬时幅度
phase=angle(y1);                              %实信号的瞬时相位

add=0;                                       %求Rmax
for i=x+1:m;
   add=add+z(i);	%信号绝对幅度的和
end

ma=add/Ns;                                 %瞬时(绝对)幅度的平均值
y2=z./ma  ;                                 %幅度比,即为文献中的an(i)
y3=y2-1;                                    %归一化瞬时幅度
y4=max(abs(y3));
y2n=y3./y4;                                  % 即为文献中的acn(i)
s=fft(y2n);
R=abs(s);
Rmax=max((R)/Ns).^2;                  %零中心归一化瞬时幅度的谱密度的最大值

%{
Xcn=0;
Ycn=0;
for i=x+1:m;
    Xcn=Xcn+y2n(i).*y2n(i);
    Ycn=Ycn+abs(y2n(i));
end
Xcnav=Xcn/Ns;
Ycnav=(Ycn/Ns).*(Ycn/Ns);
deltaaa=sqrt(Xcnav-Ycnav);               %零中心归一化瞬时幅度绝对值得标准偏差
%}

if  phase(2+x)-phase(1+x)>pi;             %修正相位序列
    Ck(1+x)=-2*pi;
elseif  phase(1+x)-phase(2+x)>pi;
    Ck(1+x)=2*pi;
else Ck(1+x)=0;
end

for i=x+2:m-1;
  if   phase(i+1)-phase(i)>pi; 
              Ck(i)=Ck(i-1)-2*pi;
      elseif  phase(i)-phase(i+1)>pi
              Ck(i)=Ck(i-1)+2*pi;
      else
              Ck(i)=Ck(i-1);
   end
end
if  -phase(m)>pi;
    Ck(m)=Ck(m-1)-2*pi;
elseif phase(m)>pi;
    Ck(m)=Ck(m-1)+2*pi;
else Ck(m)=Ck(m-1);
end
phase1=phase+Ck                       %去相位卷叠后的相位序列

phasenl=phase1-2*pi*fc*i/fs;              %非线性相位 


    at=1;                             %判决门限电平
%求取零中心非弱信号段瞬时相位非线性分量绝对值的标准偏差
%和零中心非弱信号段瞬时相位非线性分量的标准偏差
    a=0;%非线性相位之平方的和
	b=0;%非线性相位之绝对值的和
    d=0;%非线性相位的和
    c=0; %计数器，计算有多少个采样点，幅度的绝对值是大于判决门限的
for i=x+1:m;
     if y2(i)>at
         c=c+1;
         phasesquare(i)=phasenl(i).*phasenl(i); %非线性相位的平方
         a=a+phasesquare(i); %非线性相位之平方的和
         phaseabs(i)=abs(phasenl(i)); %非线性相位的绝对值
         b=b+phaseabs(i); %非线性相位之绝对值的和
         d=d+phasenl(i)    %非线性相位的和
     end
end
a1=a/c; %非线性相位之平方的和 取平均
b1=(b/c).*(b/c);	%非线性相位之绝对值的和 求平均再求平方
d1=(d/c).*(d/c);	%非线性相位的和 求平均再求平方
deltaap=sqrt(a1-b1);    %零中心非弱信号段瞬时相位非线性分量绝对值的标准偏差
deltadp=sqrt(a1-d1);    %零中心非弱信号段瞬时相位非线性分量的标准偏差


%{
freqN(i)=phase1(i)-phase1(i-1);	%去相位卷叠后的相位序列 的 相位差
for i=x+1:m;
    if i>at;
         c=c+1;
         freqNsquare(i)=freqN(i).*freqN(i);
         a=a+freqNsquare(i);
         b=b+freqN(i);
        end
end
a1=a/c;
b1=(b/c)^2;
deltaaf=sqrt(a1-b1);           %零中心归一化非弱信号段瞬时频率绝对值得标准偏差
%}

%end
if (Rmax>80)
    if(deltaaa>0.2)
       type=menu('输入信号是','4PSK信号');
    else
        type=menu('输入信号是','2ASK信号'); 
    end
else
    if (deltaaa<0.23)
       type=menu('输入信号是','4ASK信号'); 
   else
       if(deltaap>50)
          type=menu('输入信号是','4FSK信号'); 
      else
          if(Rmax>80)
              type=menu('输入信号是','2PSK信号'); 
          else
              type=menu('输入信号是','2FSK信号'); 
          end
      end
  end
end
