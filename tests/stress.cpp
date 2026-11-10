#include "../src/SteamVoxDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
int main(){
 steamvox::SteamVoxDSP d;std::mt19937 rng(43);std::uniform_real_distribution<float> noise(-.2f,.2f);
 for(double rate:{44100.,48000.,96000.,192000.}){
 d.prepare(rate);
 for(int mode=0;mode<5;++mode){
  steamvox::Controls c;
  for(int i=0;i<45000;++i){
   if(i%37==0){c.boiler=(i%101)/100.;c.blastpipe=((i*7)%101)/100.;c.steam=((i*13)%101)/100.;c.mix=((i*17)%101)/100.;d.set(c);}
   float s=(mode==0?0.f:mode==1?.2f*std::sin(i*2*steamvox::pi*211/rate):mode==2?noise(rng):mode==3?(i%400==0?1.f:0.f):.95f);
   auto r=d.process(s,-s);assert(std::isfinite(r[0])&&std::isfinite(r[1]));assert(std::abs(r[0])<=32&&std::abs(r[1])<=32);
  }
 }
 }
 for(double boundary:{.25,.5,.75}){
  d.prepare(48000);steamvox::Controls c;c.steam=boundary-.001;c.mix=1;c.boiler=.45;c.blastpipe=.5;d.set(c);
  for(int i=0;i<3000;++i)d.process(.1f,.1f);
  double previous=d.process(.1f,.1f)[0],maxDelta=0;
  for(int i=0;i<3000;++i){c.steam=boundary-.001+.002*i/2999.;d.set(c);double now=d.process(.1f,.1f)[0];maxDelta=std::max(maxDelta,std::abs(now-previous));previous=now;}
  assert(maxDelta<.05);
 }
 std::cout<<"SteamVox DSP stress: PASS\n";
}
