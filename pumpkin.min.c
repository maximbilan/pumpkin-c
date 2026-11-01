#include<math.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<sys/ioctl.h>
int W=80,H=24,P,p,i,j,k,f,c,R,G,B;
float K=1.342,T,C,S,M,N,m,l,X,Y,Z,x,y,z,t,d,u,v,e,n,q,a,b,g,h,s,o,I,L;
struct winsize w;
float D(float x,float y,float z){
float r=sqrt(x*x+z*z),b=sqrt(r*r+y*y*K*K)-1+.1*(1-sqrt(fabs(sin(5*atan2(z,x)))))*fmin(r,1)+(y>0)*.12*exp(-9*r*r),
u=x-.3*(y-.6)*(y-.6),s=fmax(sqrt(u*u+z*z)-.1+.05*(y-.6),fabs(y-.74)-.24);
P=s<b;return fmin(b,s)*.6;}
int O(float x,float y){
float a=fabs(x),s=x/.6,k=1-s*s,t=-.1-.15*k,b=-.14-.31*k;
return(y>.04&&y<.4&&fabs(a-.35)<(.4-y)*.62)||(y>-.06&&y<.06&&a<(.06-y)*.55)||
(k>0&&y<t&&y>b&&!(fabs(a-.22)<.06&&y>t-.08)&&!(a<.06&&y<b+.07));}
void Q(int s){write(1,"\33[m\33[?25h\33[2J\33[H",19);_exit(0);}
int main(){
signal(2,Q);ioctl(1,TIOCGWINSZ,&w);if(w.ws_col)W=w.ws_col,H=w.ws_row;
m=fmin(W,2.2*H)*1.45;printf("\33[2J\33[?25l");
for(;;f++){
T=f/60.;C=T*.8;C-=.6*sin(C);S=sin(C);C=cos(C);l=.85+.1*sin(T*9)+.07*sin(T*23+1);M=-.7*C-.5*S;N=.7*S-.5*C;
printf("\33[H");
for(j=0;j<H;j++){
for(i=0;i<W;i++){
u=(i-W/2.)/m;v=(H/2.-j)*2/m-.07;X=C*u+S;Y=v;Z=C-S*u;n=sqrt(X*X+Y*Y+Z*Z);X/=n;Y/=n;Z/=n;
for(t=0,d=1,k=0;k<90&&d>.002&&t<7;k++)x=-3.6*S+t*X,y=.25+t*Y,z=-3.6*C+t*Z,t+=d=D(x,y,z);
c=-1;p=P;
if(d<.01){
if(!p&&z<0&&O(x,y)){
e=Y*K;q=y*K;a=X*X+e*e+Z*Z;b=x*X+q*e+z*Z;t=(-b+sqrt(fmax(0,b*b-a*(x*x+q*q+z*z-.8))))/a;
u=x+t*X;v=y+t*Y;e=z+t*Z;g=u*u+(v+.42)*(v+.42)+e*e;
h=(u*u+v*K*K*(v+.42)+e*e)/sqrt(g*(u*u+v*v*K*K*K*K+e*e));
s=fmin(fmax(-x*X-(.42+y)*Y-z*Z,0),t);I=.55+.45*fmax(h,0)*.3/g;
u=x+s*X;v=y+s*Y+.42;e=z+s*Z;I=fmin(l*(I+exp(-(u*u+v*v*.4+e*e)*40)),1);
c=9+2.99*I;R=255;G=165+90*I;B=50+170*I*I;
}else{
e=.01;b=D(x,y,z);u=D(x+e,y,z)-b;v=D(x,y+e,z)-b;q=D(x,y,z+e)-b;n=sqrt(u*u+v*v+q*q);u/=n;v/=n;q/=n;
a=fmax(0,u*M+v*.5+q*N);g=M-X;h=.5-Y;s=N-Z;n=sqrt(g*g+h*h+s*s);s=pow(fmax(0,(u*g+v*h+q*s)/n),30);
o=fmin(fmax(D(x+u*.1,y+v*.1,z+q*.1)/.06,0),1);L=fmin(.05+.75*a*o+.35*s,1);c=L*9.99;
if(p)R=110*(.3+L),G=140*(.3+L),B=40*(.3+L);else R=235*(.2+.8*L),G=95*(.15+.85*L)+50*s,B=10+60*s;
}}
if(c<0)putchar(32);else printf("\33[38;2;%d;%d;%dm%c",R,G,B,".,-~:;=!*#$@"[c]);
}
if(j<H-1)putchar(10);
}
fflush(stdout);usleep(16000);
}}
