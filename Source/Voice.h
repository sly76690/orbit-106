#pragma once
// Envelope with curved segments, shaped like the charge and discharge of a capacitor rather than
// straight lines. Attack bends towards a target above full level, so it rises fast then eases in and
// still arrives at full level in the set time. Decay and release fall quickly at first and then
// settle: at the set time they are within about 3 % of where they are heading.
// Same calls as juce::ADSR, which it replaces.
class Contour {
public:
 struct Parameters { float attack, decay, sustain, release; };
 void setSampleRate(double sr) { sampleRate=sr; }
 void setParameters(const Parameters& p) {
  sustain=p.sustain;
  attackRate=rate(p.attack/std::log(overshoot/(overshoot-1.f)));
  decayRate=rate(p.decay/curve); releaseRate=rate(p.release/curve);
 }
 void reset() { level=0; stage=idle; }
 void noteOn() { stage=attack; }
 void noteOff() { if(stage!=idle) stage=release; }
 bool isActive() const { return stage!=idle; }
 float getNextSample() {
  switch(stage) {
   case attack: level+=(overshoot-level)*attackRate; if(level>=1.f){level=1.f;stage=decay;} break;
   case decay: level+=(sustain-level)*decayRate; break;                 // also holds, and follows, the sustain level
   case release: level-=level*releaseRate; if(level<silence){level=0;stage=idle;} break;
   case idle: break;
  }
  return level;
 }
private:
 enum Stage { idle, attack, decay, release };
 static constexpr float overshoot=1.3f, curve=3.5f, silence=.001f;
 float rate(float timeConstant) const { return 1.f-std::exp(-1.f/(jmax(.0001f,timeConstant)*float(sampleRate))); }
 double sampleRate=44100; float level=0,sustain=1,attackRate=1,decayRate=1,releaseRate=1; Stage stage=idle;
};

// One LFO for the whole instrument, rendered once per audio block by the processor so that
// every voice reads the same value at the same sample.
struct ModulationBus { std::vector<float> lfo; };

// Two independently selectable low-pass topologies; each slot owns its history.
// Cutoff and resonance are applied on the very sample they are given: nothing is smoothed inside
// the filter, so a fast filter envelope keeps its edge.
class FilterSlot {
public:
 void prepare(double sr) { sampleRate=float(sr); for(auto& f:svf){f.prepare({sr,512,1});f.setType(dsp::StateVariableTPTFilterType::lowpass);} reset(); }
 void reset() { ladder.fill(0.f); for(auto& f:svf)f.reset(); }
 void configure(int model,int slope,float resonance) { type=model; stages=slope; feedback=4.f*(.1f+.9f*resonance); for(auto& f:svf)f.setResonance(.7071f+resonance*7.f); }
 float process(float x,float cutoff) {
  if(type==0) return processLadder(x,cutoff);
  svf[0].setCutoffFrequency(cutoff); x=svf[0].processSample(0,x);
  if(stages){svf[1].setCutoffFrequency(cutoff); x=svf[1].processSample(0,x);}
  return std::tanh(x);
 }
private:
 // Four one-pole stages in series with resonance fed back from the last one, solved without a
 // one-sample delay in the loop. The 12 dB setting taps the signal after the second stage.
 float processLadder(float x,float cutoff) {
  const float g=std::tan(MathConstants<float>::pi*jmin(cutoff,sampleRate*.45f)/sampleRate), G=g/(1.f+g), keep=1.f-G, G2=G*G;
  const float pending=keep*(G2*G*ladder[0]+G2*ladder[1]+G*ladder[2]+ladder[3]);
  float y=(x*(1.f+.5f*feedback)-feedback*pending)/(1.f+feedback*G2*G2);
  y=std::tanh(y*drive)/drive;
  float tap=0;
  for(size_t n=0;n<4;++n){const float v=(y-ladder[n])*G; y=v+ladder[n]; ladder[n]=y+v; if(n==1)tap=y;}
  return stages?y:tap;
 }
 static constexpr float drive=1.2f;
 std::array<float,4> ladder{}; std::array<dsp::StateVariableTPTFilter<float>,2> svf; float sampleRate=44100.f,feedback=.4f; int type=0,stages=1;
};
class Voice : public SynthesiserVoice {
public:
 Voice(AudioProcessorValueTreeState& s,double& previous,const ModulationBus& shared) : state(s),previousPitch(previous),bus(shared) {}
 bool canPlaySound(SynthesiserSound* s) override { return dynamic_cast<Sound*>(s)!=nullptr; }
 float p(const char* key) const {return state.getRawParameterValue(key)->load();}
 void envelopes(){env.setParameters({p("attack"),p("decay"),p("sustain"),p("release")});filterEnv.setParameters({p("fattack"),p("fdecay"),p("fsustain"),p("frelease")});gate.setParameters({.002f,.002f,1.f,.006f});}
 void retarget(int note,float velocity,int wheel){double current=pitch.getCurrentValue();hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));pitch.setCurrentAndTargetValue(current);pitch.setTargetValue(hz);previousPitch=hz;vel=velocity;pitchWheelMoved(wheel);}
 void startNote(int note,float velocity,SynthesiserSound*,int wheel) override {
  double from=voiceGain<1&&pitch.getCurrentValue()>0?pitch.getCurrentValue():previousPitch;hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));pitch.setCurrentAndTargetValue(p("portamento")>.001f&&from>0?from:hz);pitch.setTargetValue(hz);previousPitch=hz;vel=velocity;pitchWheelMoved(wheel);
  a.reset();b.reset();highpass.reset();bass=0;env.reset();filterEnv.reset();gate.reset();envelopes();env.noteOn();filterEnv.noteOn();gate.noteOn();lfoAge=0;
 }
 void stopNote(float,bool tail) override {env.noteOff();filterEnv.noteOff();gate.noteOff();if(!tail){env.reset();filterEnv.reset();gate.reset();clearCurrentNote();}}
 void pitchWheelMoved(int value) override {wheelValue=value;}
 void controllerMoved(int cc,int value) override {if(cc==1)wheel=float(value)/127.f;}
 void setUnison(float cents,float amplitude){unisonCents=cents;voiceGain=amplitude;}
 void setNoiseSeed(int seed){random.setSeed(seed);}
 void prepare(double sr){env.setSampleRate(sr);filterEnv.setSampleRate(sr);gate.setSampleRate(sr);highpass.prepare({sr,512,1});highpass.setType(dsp::StateVariableTPTFilterType::highpass);a.prepare(sr);b.prepare(sr);}
 static double blep(double t,double dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
 static float wave(double t,double dt,float width,float mix){double saw=2*t-1-blep(t,dt);double pulse=(t<width?1.:-1.)+blep(t,dt)-blep(std::fmod(t+1-width,1.),dt);return float(saw*(1-mix)+pulse*mix);}
 static float oscillator(int shape,double t,double dt,float width,float mix){switch(shape){case 1:return wave(t,dt,width,0);case 2:return wave(t,dt,width,1);case 3:return float(2./MathConstants<double>::pi*std::asin(std::sin(t*MathConstants<double>::twoPi)));case 4:return float(std::sin(t*MathConstants<double>::twoPi));default:return wave(t,dt,width,mix);}}
 void renderNextBlock(AudioBuffer<float>& out,int start,int count) override {
  if(!isVoiceActive())return;envelopes();const double sr=getSampleRate();
  // High-pass switch: 0 = bass boost, 1 = flat, 2 and 3 = two fixed high-pass settings.
  const int hpfMode=int(p("hpf"));if(hpfMode>=2)highpass.setCutoffFrequency(hpfMode==2?hpfLowHz:hpfHighHz);
  const float bassRate=1.f-std::exp(-MathConstants<float>::twoPi*bassBoostHz/float(sr));
  a.configure(int(p("filterAType")),int(p("slope")),p("resonance"));b.configure(int(p("filterBType")),int(p("slopeB")),p("resonanceB"));
  const int shape1=int(p("osc1Shape")),shape2=int(p("osc2Shape")),route=int(p("filterRoute"));
  const float cutoff=p("cutoff"),cutoffB=p("cutoffB"),amount=p("env"),amountB=p("envB"),width=p("width"),width2=p("osc2Width"),mix=p("wave"),layer=p("oscLayer"),level=p("osc1Level"),subLevel=p("sub"),noise=p("noise"),velocity=p("velocity"),pwmDepth=p("pwm"),blend=p("filterBlend"),pitchDepth=p("lfoPitch"),wheelDepth=wheel*p("wheelDepth"),filterDepth=p("lfoFilter"),envPitch=p("envPitch");
  const bool linked=p("envLink")>.5f;
  // VCA mode GATE: the amplifier simply opens and closes with the key; the ADSR keeps running for a filter that follows it.
  // Filter A envelope can be inverted. PWM can come from the LFO, stay manual (WIDTH only), or follow the filter envelope.
  const bool gated=p("vcaMode")>.5f;const float polarity=p("envInvert")>.5f?-1.f:1.f;const int pwmSource=int(p("pwmSource"));
  // The bender can also move both filter cutoffs, by up to the set number of octaves.
  // SYNC restarts oscillator II each time oscillator I completes a cycle; the pitch envelope then sweeps II only,
  // which is what gives the classic sync lead. X-MOD lets oscillator II bend the frequency of oscillator I.
  const bool sync=p("oscSync")>.5f;const double crossMod=p("crossMod")*1.5;
  const float bendCutoff=std::pow(2.f,float((wheelValue-8192)/8192.)*p("bendFilter"));
  const double ratio1=std::pow(2.,int(p("osc1Octave"))-2),ratio2=std::pow(2.,int(p("osc2Octave"))-2+p("osc2Tune")/12.+p("detune")/1200.);
  const double bend=std::pow(2.,((wheelValue-8192)/8192.)*p("bendRange")/12.+unisonCents/1200.);
  // LFO delay: after a new note the LFO stays out for the first part of the delay time, then fades in.
  // The mod wheel vibrato is not delayed.
  const float lfoDelay=p("lfoDelay")*float(sr),lfoHold=lfoDelay*.6f,lfoRamp=jmax(1.f,lfoDelay*.4f);
  const float tracking=std::pow(float(hz/261.625565),p("keytrack"));
  for(int i=0;i<count;++i){
   float e=env.getNextSample(),fe=filterEnv.getNextSample();if(linked)fe=e;
   const float open=gate.getNextSample(),amp=gated?open:e;
   const float lfoNow=bus.lfo[size_t(start+i)],modulation=lfoNow*(lfoDelay<1.f?1.f:jlimit(0.f,1.f,(lfoAge-lfoHold)/lfoRamp));
   if(lfoAge<1.e7f)lfoAge+=1.f;
   const double base=pitch.getNextValue()*bend/sr*std::pow(2.,(modulation*pitchDepth+lfoNow*wheelDepth+(sync?0.f:fe*envPitch))/12.);
   const double dt=jlimit(.0000001,.45,base*ratio1),dt2=jlimit(.0000001,.45,base*ratio2*(sync?std::pow(2.,fe*envPitch/12.):1.));
   const double dtx=crossMod>0?jlimit(.0000001,.45,dt*(1.+crossMod*lastOsc2)):dt;
   const float pwmMove=pwmSource==0?.18f*pwmDepth*modulation:(pwmSource==2?.4f*pwmDepth*fe:0.f);
   float pwm=jlimit(.08f,.92f,width+pwmMove),pwm2=jlimit(.08f,.92f,(shape2==0?width:width2)+pwmMove);
   const float second=oscillator(shape2,phase2,dt2,pwm2,mix);lastOsc2=second;
   float sample=(level*oscillator(shape1,phase,dtx,pwm,mix)+layer*second)/(1+layer*.82f);
   sample=.7f*sample+.3f*subLevel*wave(sub,dt*.5,.5f,1);sample+=noise*.2f*(random.nextFloat()*2-1);if(hpfMode>=2)sample=highpass.processSample(0,sample);else if(hpfMode==0){bass+=bassRate*(sample-bass);sample+=bassBoostGain*bass;}
   float ca=jlimit(25.f,float(sr*.45),cutoff*tracking*bendCutoff*std::pow(2.f,fe*amount*polarity*6+modulation*filterDepth));
   float cb=jlimit(25.f,float(sr*.45),cutoffB*tracking*bendCutoff*std::pow(2.f,fe*amountB*6+modulation*filterDepth));
   switch(route){case 1:sample=b.process(sample,cb);break;case 2:sample=b.process(a.process(sample,ca),cb);break;case 3:sample=a.process(b.process(sample,cb),ca);break;case 4:sample=(1-blend)*a.process(sample,ca)+blend*b.process(sample,cb);break;default:sample=a.process(sample,ca);}
   sample*=amp*(1-velocity+velocity*vel)*.18f*voiceGain;
   for(int ch=0;ch<out.getNumChannels();++ch)out.addSample(ch,start+i,sample);
   phase+=dtx;
   if(phase>=1.){phase-=std::floor(phase);if(sync)phase2=phase/dtx*dt2-dt2;} // restart II at the exact point inside the sample where I wrapped
   phase2+=dt2;phase2-=std::floor(phase2);sub+=dt*.5;sub-=std::floor(sub);
   if(!(gated?gate.isActive():env.isActive())){clearCurrentNote();break;}
  }
 }
private:
 AudioProcessorValueTreeState& state;double& previousPitch;const ModulationBus& bus;SmoothedValue<double> pitch;Contour env,filterEnv,gate;FilterSlot a,b;dsp::StateVariableTPTFilter<float> highpass;Random random{12345};
 double phase=0,phase2=0,sub=0,hz=440;float vel=0,wheel=0,unisonCents=0,voiceGain=1,lfoAge=0,bass=0,lastOsc2=0;
 // Tuning values for the high-pass switch: starting points to adjust by ear, not hardware measurements.
 static constexpr float hpfLowHz=225.f,hpfHighHz=700.f,bassBoostHz=90.f,bassBoostGain=1.2f;int wheelValue=8192;
};
