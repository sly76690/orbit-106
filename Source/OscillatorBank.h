#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Three independent phase accumulators per voice. Both sections feed the same HPF/filter/VCA.
// Called at twice the output sample rate; the voice performs the final downsampling.
class OscillatorBank {
public:
 struct Settings {
  float wave=.35f, width=.5f, sub=.7f, noise=0, level=1;
  float blend=0, sourceMix=.5f, width1=.5f, width2=.5f;
  float fine=0, tune=0, crossMod=0, age=.3f;
  int shape1=0, shape2=0, range1=3, range2=3;
  bool sync=false, lowFrequency=false;
 };
 void setVoiceIndex(int index) {
  static constexpr float errors1[8]={.15f,-.55f,.8f,-.3f,.45f,-1.f,.65f,-.2f};
  static constexpr float errors2[8]={-.35f,.6f,-.1f,.9f,-.75f,.25f,-.5f,-.05f};
  error1=errors1[index&7];error2=errors2[index&7];
  driftRandom.setSeed(index*104729+5);noiseDco.setSeed(1001+index);
  noise1.setSeed(3001+index);noise2.setSeed(5001+index);
 }
 void prepare(double outputRate) {
  sr=outputRate*2.;follow=1.f-std::exp(-1.f/(.005f*float(sr)));
  driftFollow=1.f-std::exp(-1.f/(.6f*float(sr)));
  dco=vco1=vco2=subPhase=0;drift1=drift2=target1=target2=0;driftCount=0;
 }
 void configure(const Settings& next,bool fresh) {
  settings=next;
  range1=std::exp2(double(next.range1-3));
  range2=std::exp2(double(next.range2-3)+next.tune/12.+next.fine/1200.);
  if(fresh){blend=next.blend;sourceMix=next.sourceMix;level=next.level;
   sub=next.sub;noise=next.noise;width=next.width;width1=next.width1;width2=next.width2;waveMix=next.wave;}
 }
 float process(double frequency,float envelopeSemitones,float pwm,float vcoPwm=0) {
  const auto smooth=[this](float& value,float target){value+=follow*(target-value);};
  smooth(blend,settings.blend);smooth(sourceMix,settings.sourceMix);smooth(level,settings.level);
  smooth(sub,settings.sub);smooth(noise,settings.noise);smooth(width,settings.width);
  smooth(width1,settings.width1);smooth(width2,settings.width2);smooth(waveMix,settings.wave);
  if(--driftCount<=0){target1=driftRandom.nextFloat()*2-1;target2=driftRandom.nextFloat()*2-1;driftCount=int(sr*(.4+1.2*driftRandom.nextFloat()));}
  drift1+=driftFollow*(target1-drift1);drift2+=driftFollow*(target2-drift2);
  const double env=std::exp2(double(envelopeSemitones)/12.);
  // AGE is confined to the VCOs. DCO, envelopes, filters and LFO do not change with voice count.
  const double detune1=std::exp2(settings.age*(6.*error1+4.*drift1)/1200.);
  const double detune2=std::exp2(settings.age*(6.*error2+4.*drift2)/1200.);
  const double dd=step(frequency*env),d1=step(frequency*range1*detune1*(settings.sync?1.:env));
  // LOW FREQ is free of keyboard tracking: 2 Hz at 8', with RANGE/TUNE/FINE still available.
  const double d2=step((settings.lowFrequency?2.:frequency*env)*range2*detune2);
  const float second=oscillator(settings.shape2,vco2,d2,juce::jlimit(.08f,.92f,width2+vcoPwm),noise2);
  const double dx=juce::jlimit(1.e-9,.45,d1*(1.+1.5*settings.crossMod*second));
  const float first=oscillator(settings.shape1,vco1,dx,juce::jlimit(.08f,.92f,width1+vcoPwm),noise1);
  const float juno=.7f*level*wave(dco,dd,juce::jlimit(.08f,.92f,width+pwm),waveMix)
                   +.3f*sub*wave(subPhase,dd*.5,.5f,1.f)+noise*.2828f*(noiseDco.nextFloat()*2-1);
  const float jupiter=.7f*((1.f-sourceMix)*first+sourceMix*second);
  advance(dco,dd);advance(subPhase,dd*.5);
  vco1+=dx;
  if(vco1>=1.){vco1-=std::floor(vco1);if(settings.sync)vco2=vco1/dx*d2-d2;}
  advance(vco2,d2);
  // Linear crossfades keep endpoint gains constant and avoid a +3 dB bump for correlated waves.
  return (1.f-blend)*juno+blend*jupiter;
 }
private:
 double step(double hz) const {return juce::jlimit(1.e-9,.225,hz/sr);}
 static void advance(double& phase,double delta){phase+=delta;phase-=std::floor(phase);}
 static double blep(double t,double dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
 static float wave(double t,double dt,float width,float mix){
  const double saw=2*t-1-blep(t,dt),pulse=(t<width?1.:-1.)+blep(t,dt)-blep(std::fmod(t+1-width,1.),dt);
  return float(saw*(1-mix)+pulse*mix);
 }
 static float oscillator(int shape,double phase,double dt,float width,juce::Random& random){
  switch(shape){
   case 1:return wave(phase,dt,width,1);
   case 2:return float(2./juce::MathConstants<double>::pi*std::asin(std::sin(phase*juce::MathConstants<double>::twoPi)));
   case 3:return float(std::sin(phase*juce::MathConstants<double>::twoPi));
   case 4:return wave(phase,dt,.5f,1);
   case 5:return random.nextFloat()*2-1;
   default:return wave(phase,dt,width,0);
  }
 }
 Settings settings;
 double sr=96000,dco=0,subPhase=0,vco1=0,vco2=0,range1=1,range2=1;
 float follow=.004f,driftFollow=0,error1=0,error2=0,drift1=0,drift2=0,target1=0,target2=0;
 float blend=0,sourceMix=.5f,level=1,sub=.7f,noise=0,width=.5f,width1=.5f,width2=.5f,waveMix=.35f;
 int driftCount=0;juce::Random driftRandom{5},noiseDco{1001},noise1{3001},noise2{5001};
};
