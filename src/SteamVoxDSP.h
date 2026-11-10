#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace steamvox {
constexpr double pi=3.14159265358979323846;
inline double clamp(double x,double a,double b){return std::clamp(x,a,b);}
inline double smooth(double x){x=clamp(x,0,1);return x*x*(3-2*x);}
inline double gain(double d){return std::pow(10.,d/20.);}
inline double db(double x){return 20*std::log10(std::max(1e-12,x));}
struct Pole {
 double z=0,c=0;
 void set(double hz,double sr){c=std::exp(-2*pi*clamp(hz,1,sr*.47)/sr);}
 double tick(double x){z+=(1-c)*(x-z);return z;}
};
struct Biquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void set(double B0,double B1,double B2,double A0,double A1,double A2){b0=B0/A0;b1=B1/A0;b2=B2/A0;a1=A1/A0;a2=A2/A0;}
 double tick(double x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
 void hp(double hz,double sr){double w=2*pi*clamp(hz,12,sr*.45)/sr,c=cos(w),a=sin(w)/1.41421356;set((1+c)/2,-1-c,(1+c)/2,1+a,-2*c,1-a);}
 void bell(double hz,double sr,double q,double g){double w=2*pi*clamp(hz,12,sr*.45)/sr,c=cos(w),A=std::pow(10.,g/40),a=sin(w)/(2*q);set(1+a*A,-2*c,1-a*A,1+a/A,-2*c,1-a/A);}
 void shelf(double hz,double sr,double g){double w=2*pi*clamp(hz,12,sr*.45)/sr,c=cos(w),A=std::pow(10.,g/40),t=std::sqrt(2*A)*sin(w);set(A*((A+1)+(A-1)*c+t),-2*A*((A-1)+(A+1)*c),A*((A+1)+(A-1)*c-t),(A+1)-(A-1)*c+t,2*((A-1)-(A+1)*c),(A+1)-(A-1)*c-t);}
};
struct Compressor {
 double env=0,g=1;
 double tick(double x,double sr,double threshold,double ratio,double attackMs,double releaseMs){
  double a=exp(-1/(sr*.001*attackMs)),r=exp(-1/(sr*.001*releaseMs)),v=std::abs(x),c=v>env?a:r;
  env=c*env+(1-c)*v;double over=db(env)-threshold,knee=6,red=0;
  if(over>knee/2)red=over*(1-1/ratio);
  else if(over>-knee/2)red=(1-1/ratio)*(over+knee/2)*(over+knee/2)/(2*knee);
  double target=gain(-red),gc=target<g?a:r;g=gc*g+(1-gc)*target;return x*g;
 }
};
struct Deesser {
 Pole split,detect;double g=1;
 void prepare(double sr){split.set(std::min(6800.,sr*.38),sr);detect.set(100,sr);}
 double tick(double x,double sr,double strength){
  double hi=x-split.tick(x),e=detect.tick(std::abs(hi)),want=1-.78*strength*smooth((e-.008)*35);
  double c=exp(-1/(sr*.009));g=c*g+(1-c)*want;return x-hi*(1-g);
 }
};
struct Eq {
 Biquad h,a,b,p;
 void set(double sr,double phase,double body,double presence){
  h.hp(50+105*phase,sr);a.shelf(12000,sr,10*phase);b.bell(220,sr,.7,body);p.bell(1500,sr,.8,presence);
 }
 double tick(double x){return p.tick(b.tick(a.tick(h.tick(x))));}
};
struct Channel {
 Compressor fet,opto,parallel,peak,colourComp;
 Deesser deess,finalDeess;
 Pole bodyDetect,bodySplit;
 std::array<Eq,4> eq;
 unsigned eqUpdate=0;
 void prepare(double sr){deess.prepare(sr);finalDeess.prepare(sr);bodyDetect.set(120,sr);bodySplit.set(260,sr);}
};
struct Controls {double boiler=0,steam=0,blastpipe=0,mix=1,inputDB=0,outputDB=0;bool oversampling=false;};
class SteamVoxDSP {
 double sr=48000,coefficient=0;
 Controls target{},current{};std::array<Channel,2> channels{};
 static double blend(double a,double b,double t){return a+(b-a)*smooth(t);}
 static double body(double s){return s<.225?0:s<.25?.5*smooth((s-.225)*40):s<.5?.5+4.5*smooth((s-.25)*4):s<.75?5-2*smooth((s-.5)*4):3;}
 double equalize(double x,Channel& c,double s){
  int q=std::min(3,int(4*s));double phase=clamp(4*s-q,0,1),v[4];
  for(int i=0;i<4;++i){
   // Previous path is held at its full-quarter setting for the transition.
   double p=i==q?phase:(i<q?1.:0.);
   if ((c.eqUpdate & 15u)==0u) c.eq[i].set(sr,p,body(s),4*smooth((s-.5)*4));
   v[i]=c.eq[i].tick(x);
  }
  // Fade into the next quarter BEFORE the boundary, guaranteeing continuity
  // as the shelf and HPF reset for the following processing journey.
  ++c.eqUpdate;
  if(q<3 && phase>.86)return blend(v[q],v[q+1],(phase-.86)/.14);
  return v[q];
 }
 double processChannel(double x,Channel& c,const Controls& k){
  double tube=smooth(4*k.blastpipe),iron=smooth(4*(k.blastpipe-.20)),bite=smooth(4*(k.blastpipe-.45)),rich=smooth(4*(k.blastpipe-.70));
  double drive=1+2.2*tube;
  double t=tanh(x*drive+.075*tube*x*x)/tanh(drive);
  double y=blend(x,t,.64*tube);
  double d=1+.9*iron;y=blend(y,tanh(y*d)/tanh(d),.35*iron);
  d=1+1.5*bite;y=blend(y,tanh(y*d)/tanh(d),.3*bite);
  y=blend(y,tanh(5*y)/tanh(5.),.34*rich);
  y=blend(y,c.colourComp.tick(y,sr,-25,2.5,18,200),.15*rich);
  double fet=smooth(4*(k.boiler-.20)),ratio=4+4*smooth(4*(k.boiler-.45));
  y=blend(y,c.fet.tick(y,sr,-17,ratio,4.5,65),fet);
  y=blend(y,c.opto.tick(y,sr,-21,3.2,18,230),smooth(4*k.boiler));
  y=blend(y,c.parallel.tick(y,sr,-26,9,2.5,95),.3*smooth(4*(k.boiler-.70)));
  y=c.deess.tick(y,sr,.8*k.boiler);
  double low=c.bodySplit.tick(y),env=c.bodyDetect.tick(std::abs(low));
  y-=low*.16*smooth(4*(k.boiler-.48))*smooth((env-.08)*9);
  y=equalize(y,c,k.steam);
  double excitation=smooth(10*(k.steam-.90)),distortion=tanh(2*y)*.5;
  y=blend(y,y+.18*(distortion-y),excitation);
  y=c.finalDeess.tick(y,sr,.35+.35*k.steam);
  double peak=tanh(y*1.1)/1.1;
  return blend(y,peak,.33*smooth(4*(k.boiler-.45)));
 }
public:
 void prepare(double sampleRate){sr=std::max(22050.,sampleRate);channels={};for(auto& c:channels)c.prepare(sr);coefficient=exp(-1/(sr*.004));current=target;}
 void reset(){prepare(sr);}
 void set(const Controls& c){target=c;target.boiler=clamp(c.boiler,0,1);target.steam=clamp(c.steam,0,1);target.blastpipe=clamp(c.blastpipe,0,1);target.mix=clamp(c.mix,0,1);target.inputDB=clamp(c.inputDB,-24,24);target.outputDB=clamp(c.outputDB,-24,24);}
 Controls get() const {return target;}
 std::array<float,2> process(float left,float right){
  auto follow=[&](double& a,double b){a=coefficient*a+(1-coefficient)*b;};
  follow(current.boiler,target.boiler);follow(current.steam,target.steam);follow(current.blastpipe,target.blastpipe);follow(current.mix,target.mix);follow(current.inputDB,target.inputDB);follow(current.outputDB,target.outputDB);
  double in=gain(current.inputDB),out=gain(current.outputDB),samples[]={left,right};std::array<float,2> result{};
  for(int i=0;i<2;++i){double dry=std::isfinite(samples[i])?clamp(samples[i]*in,-32,32):0;double wet=processChannel(dry,channels[i],current);double z=(dry*(1-current.mix)+wet*current.mix)*out;result[i]=float(std::isfinite(z)?clamp(z,-32,32):0);}
  return result;
 }
};
}
