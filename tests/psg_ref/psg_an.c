/* SPDX-License-Identifier: GPL-3.0-only */
/* an FILE.f32 T0 T1 F0 [NH [CH]]  : stereo f32 @44100. CH 0 L 1 R 2 mid (default mid).
   prints: rms dBFS(32768), peak, dc, measured f0 (FFT, parabolic), harmonics: abs dBFS and rel dB, ; "L=.. R=.." rms for both */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define N 65536
static double re[N], im[N];
static void fft(void){ int i,j=0,k; for(i=1;i<N;i++){int b=N>>1; for(;j&b;b>>=1) j^=b; j^=b; if(i<j){double t=re[i];re[i]=re[j];re[j]=t;t=im[i];im[i]=im[j];im[j]=t;}}
 for(int len=2;len<=N;len<<=1){double a=-2*M_PI/len; double wr=cos(a),wi=sin(a); for(i=0;i<N;i+=len){double cr=1,ci=0; for(k=0;k<len/2;k++){int u=i+k,v=i+k+len/2; double xr=re[v]*cr-im[v]*ci, xi=re[v]*ci+im[v]*cr; re[v]=re[u]-xr; im[v]=im[u]-xi; re[u]+=xr; im[u]+=xi; double t=cr*wr-ci*wi; ci=cr*wi+ci*wr; cr=t;}}}}
int main(int argc,char**argv){
 FILE*f=fopen(argv[1],"rb"); if(!f) return 1; fseek(f,0,SEEK_END); long sz=ftell(f)/8; fseek(f,0,SEEK_SET); float*d=malloc(sz*8); fread(d,8,sz,f); fclose(f);
 double t0=atof(argv[2]), t1=atof(argv[3]), f0=atof(argv[4]); int nh=argc>5?atoi(argv[5]):8, ch=argc>6?atoi(argv[6]):2;
 long s0=(long)(t0*44100), s1=(long)(t1*44100); if(s1>sz) s1=sz; long n=s1-s0; if(n>N) n=N;
 double sl=0,sr=0,sm=0,pk=0,mean=0; static double x[N];
 for(long i=0;i<n;i++){double l=d[2*(s0+i)],r=d[2*(s0+i)+1]; sl+=l*l; sr+=r*r; double v=ch==0?l:ch==1?r:(l+r)/2; x[i]=v; mean+=v; if(fabs(v)>pk)pk=fabs(v);}
 mean/=n; double ss=0; for(long i=0;i<n;i++) ss+=(x[i]-mean)*(x[i]-mean);
 double rms=sqrt(ss/n);
 printf("rms %.1f dBFS  peak %.0f  dc %.1f  | L %.1f R %.1f dBFS(rms incl dc)\n",20*log10(rms/32768+1e-12),pk,mean,20*log10(sqrt(sl/n)/32768+1e-12),20*log10(sqrt(sr/n)/32768+1e-12));
 if(f0<=0) return 0;
 double wsum=0; for(long i=0;i<N;i++){double w=i<n?0.5-0.5*cos(2*M_PI*i/(n-1)):0; re[i]=i<n?(x[i]-mean)*w:0; im[i]=0; wsum+=w;}
 fft(); double fs=44100.0, bw=fs/N; static double mag[N/2];
 for(int i=0;i<N/2;i++) mag[i]=2*sqrt(re[i]*re[i]+im[i]*im[i])/wsum;
 /* fundamental */
 int lo=(int)(f0*0.9/bw), hi=(int)(f0*1.1/bw), b=lo; for(int i=lo;i<=hi;i++) if(mag[i]>mag[b]) b=i;
 double a=log(mag[b-1]+1e-12),c=log(mag[b]+1e-12),e=log(mag[b+1]+1e-12), dl=0.5*(a-e)/(a-2*c+e); double fm=(b+dl)*bw;
 printf("f0 expected %.3f measured %.3f (%+.2f cents)\n",f0,fm,1200*log2(fm/f0));
 double A1=0; printf("harm abs dBFS:");
 double H[64]; for(int k=1;k<=nh;k++){ double fk=k*fm; if(fk>fs/2-200){H[k]=0;printf(" -----");continue;} int l2=(int)((fk*0.985)/bw), h2=(int)((fk*1.015)/bw)+1, bb=l2; for(int i=l2;i<=h2;i++) if(mag[i]>mag[bb]) bb=i; H[k]=mag[bb]; if(k==1)A1=H[k]; printf(" %6.1f",20*log10(H[k]/32768+1e-12)); }
 printf("\nharm rel dB:   "); for(int k=1;k<=nh;k++) printf(" %6.1f",H[k]>0?20*log10(H[k]/A1+1e-12):-999.0);
 printf("\n");
 return 0;}
