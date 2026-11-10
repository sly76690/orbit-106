#pragma once
// Two independently selectable low-pass topologies; each slot owns its history.
class FilterSlot {
public:
 void prepare(double sr) {ladder.prepare({sr,512,1});for(auto& f:svf){f.prepare({sr,512,1});f.setType(dsp::StateVariableTPTFilterType::lowpass);}reset();}
 void reset(){ladder.reset();for(auto& f:svf)f.reset();}
 void configure(int model,int slope,float resonance){type=model;stages=slope;ladder.setMode(slope?dsp::LadderFilterMode::LPF24:dsp::LadderFilterMode::LPF12);ladder.setResonance(resonance);ladder.setDrive(1.2f);for(auto& f:svf)f.setResonance(.7071f+resonance*7.f);}
 float process(float x,float cutoff){if(type==0){ladder.setCutoffFrequencyHz(cutoff);float* ptr=&x;dsp::AudioBlock<float> one(&ptr,1,1);dsp::ProcessContextReplacing<float> context(one);ladder.process(context);return x;}svf[0].setCutoffFrequency(cutoff);x=svf[0].processSample(0,x);if(stages){svf[1].setCutoffFrequency(cutoff);x=svf[1].processSample(0,x);}return std::tanh(x);}
private:
 dsp::LadderFilter<float> ladder;std::array<dsp::StateVariableTPTFilter<float>,2> svf;int type=0,stages=1;
};
class Voice : public SynthesiserVoice {
public:
 explicit Voice(AudioProcessorValueTreeState& s,double& previous) : state(s),previousPitch(previous) {}
 bool canPlaySound(SynthesiserSound* s) override { return dynamic_cast<Sound*>(s)!=nullptr; }
 float p(const char* key) const {return state.getRawParameterValue(key)->load();}
 void envelopes(){env.setParameters({p("attack"),p("decay"),p("sustain"),p("release")});filterEnv.setParameters({p("fattack"),p("fdecay"),p("fsustain"),p("frelease")});}
 void retarget(int note,float velocity,int wheel){double current=pitch.getCurrentValue();hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));pitch.setCurrentAndTargetValue(current);pitch.setTargetValue(hz);previousPitch=hz;vel=velocity;pitchWheelMoved(wheel);}
 void startNote(int note,float velocity,SynthesiserSound*,int wheel) override {
  double from=voiceGain<1&&pitch.getCurrentValue()>0?pitch.getCurrentValue():previousPitch;hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));pitch.setCurrentAndTargetValue(p("portamento")>.001f&&from>0?from:hz);pitch.setTargetValue(hz);previousPitch=hz;vel=velocity;pitchWheelMoved(wheel);
  a.reset();b.reset();highpass.reset();env.reset();filterEnv.reset();envelopes();env.noteOn();filterEnv.noteOn();
 }
 void stopNote(float,bool tail) override {env.noteOff();filterEnv.noteOff();if(!tail){env.reset();filterEnv.reset();clearCurrentNote();}}
 void pitchWheelMoved(int value) override {wheelValue=value;}
 void controllerMoved(int cc,int value) override {if(cc==1)wheel=float(value)/127.f;}
 void setUnison(float cents,float amplitude){unisonCents=cents;voiceGain=amplitude;}
 void prepare(double sr){env.setSampleRate(sr);filterEnv.setSampleRate(sr);highpass.prepare({sr,512,1});highpass.setType(dsp::StateVariableTPTFilterType::highpass);a.prepare(sr);b.prepare(sr);}
 static double blep(double t,double dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
 static float wave(double t,double dt,float width,float mix){double saw=2*t-1-blep(t,dt);double pulse=(t<width?1.:-1.)+blep(t,dt)-blep(std::fmod(t+1-width,1.),dt);return float(saw*(1-mix)+pulse*mix);}
 static float oscillator(int shape,double t,double dt,float width,float mix){switch(shape){case 1:return wave(t,dt,width,0);case 2:return wave(t,dt,width,1);case 3:return float(2./MathConstants<double>::pi*std::asin(std::sin(t*MathConstants<double>::twoPi)));case 4:return float(std::sin(t*MathConstants<double>::twoPi));default:return wave(t,dt,width,mix);}}
 void renderNextBlock(AudioBuffer<float>& out,int start,int count) override {
  if(!isVoiceActive())return;envelopes();const double sr=getSampleRate();highpass.setCutoffFrequency(p("hpf"));
  a.configure(int(p("filterAType")),int(p("slope")),p("resonance"));b.configure(int(p("filterBType")),int(p("slopeB")),p("resonanceB"));
  const int shape1=int(p("osc1Shape")),shape2=int(p("osc2Shape")),lfoShape=int(p("lfoShape")),route=int(p("filterRoute"));
  const float cutoff=p("cutoff"),cutoffB=p("cutoffB"),amount=p("env"),amountB=p("envB"),width=p("width"),width2=p("osc2Width"),mix=p("wave"),layer=p("oscLayer"),level=p("osc1Level"),subLevel=p("sub"),noise=p("noise"),velocity=p("velocity"),pwmDepth=p("pwm"),blend=p("filterBlend"),pitchDepth=p("lfoPitch")+wheel*p("wheelDepth"),filterDepth=p("lfoFilter"),envPitch=p("envPitch");
  const bool linked=p("envLink")>.5f;
  const double ratio1=std::pow(2.,int(p("osc1Octave"))-2),ratio2=std::pow(2.,int(p("osc2Octave"))-2+p("osc2Tune")/12.+p("detune")/1200.);
  const double bend=std::pow(2.,((wheelValue-8192)/8192.)*p("bendRange")/12.+unisonCents/1200.);
  const double lfoStep=MathConstants<double>::twoPi*p("rate")/sr;const float tracking=std::pow(float(hz/261.625565),p("keytrack"));
  for(int i=0;i<count;++i){
   float e=env.getNextSample(),fe=filterEnv.getNextSample();if(linked)fe=e;
   float modulation=float(2./MathConstants<double>::pi*std::asin(std::sin(lfo)));
   if(lfoShape==1)modulation=float(std::sin(lfo));else if(lfoShape==2)modulation=float(lfo/MathConstants<double>::pi-1);else if(lfoShape==3)modulation=lfo<MathConstants<double>::pi?1.f:-1.f;
   const double base=pitch.getNextValue()*bend/sr*std::pow(2.,(modulation*pitchDepth+fe*envPitch)/12.);
   const double dt=jlimit(.0000001,.45,base*ratio1),dt2=jlimit(.0000001,.45,base*ratio2);
   float pwm=jlimit(.08f,.92f,width+.18f*pwmDepth*modulation),pwm2=jlimit(.08f,.92f,(shape2==0?width:width2)+.18f*pwmDepth*modulation);
   float sample=(level*oscillator(shape1,phase,dt,pwm,mix)+layer*oscillator(shape2,phase2,dt2,pwm2,mix))/(1+layer*.82f);
   sample=.7f*sample+.3f*subLevel*wave(sub,dt*.5,.5f,1);sample+=noise*.2f*(random.nextFloat()*2-1);sample=highpass.processSample(0,sample);
   float ca=jlimit(25.f,float(sr*.45),cutoff*tracking*std::pow(2.f,fe*amount*6+modulation*filterDepth));
   float cb=jlimit(25.f,float(sr*.45),cutoffB*tracking*std::pow(2.f,fe*amountB*6+modulation*filterDepth));
   switch(route){case 1:sample=b.process(sample,cb);break;case 2:sample=b.process(a.process(sample,ca),cb);break;case 3:sample=a.process(b.process(sample,cb),ca);break;case 4:sample=(1-blend)*a.process(sample,ca)+blend*b.process(sample,cb);break;default:sample=a.process(sample,ca);}
   sample*=e*(1-velocity+velocity*vel)*.18f*voiceGain;
   for(int ch=0;ch<out.getNumChannels();++ch)out.addSample(ch,start+i,sample);
   phase+=dt;phase-=std::floor(phase);phase2+=dt2;phase2-=std::floor(phase2);sub+=dt*.5;sub-=std::floor(sub);lfo+=lfoStep;if(lfo>=MathConstants<double>::twoPi)lfo-=MathConstants<double>::twoPi;
   if(!env.isActive()){clearCurrentNote();break;}
  }
 }
private:
 AudioProcessorValueTreeState& state;double& previousPitch;SmoothedValue<double> pitch;ADSR env,filterEnv;FilterSlot a,b;dsp::StateVariableTPTFilter<float> highpass;Random random{12345};
 double phase=0,phase2=0,sub=0,lfo=0,hz=440;float vel=0,wheel=0,unisonCents=0,voiceGain=1;int wheelValue=8192;
};
