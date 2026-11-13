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
struct ModulationBus {
 std::vector<float> lfo,phase; int shape=0; double step=0; // value and phase per sample, waveform, cycles per sample
 bool manual=false; double manualAge=-1; // LFO MAN: samples since LFO TRIG was pressed, -1 while it is up
 static float wave(int shape,float t){if(shape==1)return std::sin(t*MathConstants<float>::twoPi);if(shape==2)return 2*t-1;if(shape==3)return t<.5f?1.f:-1.f;return t<.25f?4*t:(t<.75f?2-4*t:4*t-4);}
};

// Two independently selectable low-pass topologies; each slot owns its history.
// Cutoff and resonance are applied on the very sample they are given: nothing is smoothed inside
// the filter, so a fast filter envelope keeps its edge.
class FilterSlot {
public:
 void prepare(double sr) { sampleRate=float(sr); for(auto& f:svf){f.prepare({sr,512,1});f.setType(dsp::StateVariableTPTFilterType::lowpass);} reset(); }
 void reset() { ladder.fill(0.f); for(auto& f:svf)f.reset(); }
 void setDrive(float amount) { drive=jmax(.2f,amount); } // how hard the ladder is pushed: more = earlier, rounder limiting
 void configure(int model,int slope,float resonance) { type=model; stages=slope; feedback=maxFeedback*jlimit(0.f,1.f,resonance/.95f); // RES fader: a gentle, even rise, matched by measurement to the reference recording (about +2.5 dB at 5, +8 dB at 10)
  // The last few steps of RES tip the ladder into self-oscillation, as on the Juno-106 (resonance "0 to self oscillation").
  if(const float t=jlimit(0.f,1.f,(resonance-.9f)/.05f);t>0.f)feedback+=selfOscillation*t*t*(3.f-2.f*t); for(auto& f:svf)f.setResonance(.7071f+resonance*7.f);
  // Ladder FREQ: without resonance the four poles sit an octave up (cutoffScale) so the ladder is as bright as the
  // state-variable model; as resonance comes in they move down so that the resonant peak sits on FREQ itself
  // (measured: with the poles left an octave up, the peak sang 1.6 to 2 times higher than the fader said).
  const float r=jlimit(0.f,1.f,resonance),k=jmin(1.f,r/.3f),blend=k*k*(3.f-2.f*k),onPeak=1.f+.35f*(1.f-r)*(1.f-r);
  ladderScale=cutoffScale*(1.f-blend)+onPeak*blend; }
 float process(float x,float cutoff) {
  if(type==0) return processLadder(x,cutoff);
  svf[0].setCutoffFrequency(cutoff); x=svf[0].processSample(0,x);
  if(stages){svf[1].setCutoffFrequency(cutoff); x=svf[1].processSample(0,x);}
  return std::tanh(x);
 }
private:
 // Four one-pole stages in series with resonance fed back from the last one, solved without a
 // one-sample delay in the loop. The 12 dB setting taps the signal after the second stage.
 // Voiced to stand next to the state-variable model: the same loudness, a cutoff that opens as far for the same
 // setting (four poles are already 12 dB down at their own frequency, hence cutoffScale), a resonance that comes in
 // early and sings at the top of its range, and the same soft output stage, so a strong peak growls instead of clipping.
 float processLadder(float x,float cutoff) {
  const float g=std::tan(MathConstants<float>::pi*jmin(cutoff*ladderScale,sampleRate*.45f)/sampleRate), G=g/(1.f+g), keep=1.f-G, G2=G*G;
  const float pending=keep*(G2*G*ladder[0]+G2*ladder[1]+G*ladder[2]+ladder[3]);
  // The signal enters at a low level so the saturation inside the loop only bites on the resonant peak (the
  // resonance builds up instead of being flattened); the lows the resonance takes away are given back at the output.
  float y=(x*inputLevel-feedback*pending)/(1.f+feedback*G2*G2);
  y=std::tanh(y*drive)/drive;
  float tap=0;
  for(size_t n=0;n<4;++n){const float v=(y-ladder[n])*G; y=v+ladder[n]; ladder[n]=y+v; if(n==1)tap=y;}
  // Output stage with headroom: it only rounds off real peaks, so a resonant peak and an envelope sweep stand out
  // above the rest of the sound instead of being flattened into it.
  return std::tanh((stages?y:tap)/inputLevel*outputLevel*(1.f+bassReturn*feedback)*outputHeadroom)/outputHeadroom*.85f;
 }
 std::array<float,4> ladder{}; std::array<dsp::StateVariableTPTFilter<float>,2> svf; float sampleRate=44100.f,feedback=.4f,drive=1.2f,ladderScale=cutoffScale; int type=0,stages=1;
 // Ladder voicing, measured against the state-variable model and to adjust by ear.
 static constexpr float inputLevel=.2f,outputLevel=1.8f,bassReturn=.45f,maxFeedback=2.2f,selfOscillation=0.f,resonanceCurve=1.f,cutoffScale=2.f,outputHeadroom=.35f;
};
// What the voices of one instrument share for the glide: the pitch the most recent note is sounding at right now
// (in Hz, part-way through its own glide if it has one), and a count of note starts that tells which note that is.
struct GlideMemory { double pitch=0; uint64 starts=0; GlideMemory& operator=(double hz){pitch=hz;return *this;} };
// Back from twice the sample rate: a 31-tap half-band low-pass (Blackman-windowed sinc), evaluated once per pair.
struct HalfBand {
 static constexpr int taps=31;
 static const std::array<float,taps>& coefficients(){
  static const std::array<float,taps> h=[]{std::array<float,taps> c{};double sum=0;
   for(int n=0;n<taps;++n){const double m=n-(taps-1)/2.,sinc=m==0?.5:std::sin(MathConstants<double>::pi*.5*m)/(MathConstants<double>::pi*m);
    const double w=.42-.5*std::cos(MathConstants<double>::twoPi*n/(taps-1))+.08*std::cos(2*MathConstants<double>::twoPi*n/(taps-1));c[size_t(n)]=float(sinc*w);sum+=c[size_t(n)];}
   for(auto& v:c)v=float(v/sum);return c;}();
  return h;
 }
 void reset(){line.fill(0.f);pos=0;}
 float process(float first,float second){line[size_t(pos)]=first;pos=(pos+1)&63;line[size_t(pos)]=second;pos=(pos+1)&63;
  const auto& h=coefficients();float y=0;for(int n=0;n<taps;++n)y+=h[size_t(n)]*line[size_t((pos-1-n)&63)];return y;}
 std::array<float,64> line{};int pos=0;
};
class Voice : public SynthesiserVoice {
public:
 Voice(AudioProcessorValueTreeState& s,GlideMemory& previous,const ModulationBus& shared) : state(s),previousPitch(previous),bus(shared) {}
 bool canPlaySound(SynthesiserSound* s) override { return dynamic_cast<Sound*>(s)!=nullptr; }
 float p(const char* key) const {return state.getRawParameterValue(key)->load();}
 // Vintage voices (the eight-voice instrument): each voice is a slightly different circuit. AGE sets how different:
 // a fixed error on each oscillator's tuning, on the filter cutoff and on the envelope times, plus a slow drift.
 // The errors are fixed per voice and balanced: over the eight voices each one averages zero, so the instrument as a
 // whole is neither flat nor dull, only uneven from one voice to the next.
 void setVintage(int index){
  static constexpr float tuning1[8]={.15f,-.55f,.8f,-.3f,.45f,-1.f,.65f,-.2f},tuning2[8]={-.35f,.6f,-.1f,.9f,-.75f,.25f,-.5f,-.05f};
  static constexpr float cutoffs[8]={.2f,-.6f,.5f,-.9f,.75f,-.3f,.95f,-.6f},times[8]={-.4f,.7f,-.85f,.3f,.9f,-.6f,.15f,-.2f};
  static constexpr float lfoRates[8]={.5f,-.85f,.2f,.95f,-.35f,.7f,-.6f,-.55f};
  const int i=index&7;vintage=true;tune1=tuning1[i];tune2=tuning2[i];cutOffset=cutoffs[i];timeOffset=times[i];lfoTune=lfoRates[i];wander.setSeed(index*104729+5);}
 // Vintage envelopes run faster up the keyboard and slower down it (15 % per octave around middle C).
 void follow(int note){keyStretch=vintage?jlimit(.55f,1.6f,std::pow(keyFollow,float(note-60)/12.f)):1.f;}
 void setPan(float position){pan=jlimit(-1.f,1.f,position);} // -1 left, 0 centre, 1 right
 void envelopes(){const float stretch=vintage?(1.f+p("age")*envelopeError*timeOffset)*keyStretch:1.f;
  env.setParameters({p("attack")*stretch,p("decay")*stretch,p("sustain"),p("release")*stretch});filterEnv.setParameters({p("fattack")*stretch,p("fdecay")*stretch,p("fsustain"),p("frelease")*stretch});gate.setParameters({.002f,.002f,1.f,.006f});}
 void retarget(int note,float velocity,int wheel){follow(note);held=true;double current=pitch.getCurrentValue();hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));pitch.setCurrentAndTargetValue(current);pitch.setTargetValue(std::log2(hz));serial=++previousPitch.starts;previousPitch.pitch=pitched?std::exp2(current):hz;vel=velocity;pitchWheelMoved(wheel);}
 void startNote(int note,float velocity,SynthesiserSound*,int wheel) override {
  follow(note);punch=vintage?1.f:0.f;
  // The glide runs on the pitch (log2 of the frequency), so an octave takes the same time wherever it is on the keyboard.
  // A new note glides from where the previous one is sounding at this instant, which this voice then keeps up to date
  // while it glides itself: a note played twice in a row, or a fast run, carries the glide on instead of cutting it short.
  double from=voiceGain<1&&pitched?std::exp2(pitch.getCurrentValue()):previousPitch.pitch;hz=MidiMessage::getMidiNoteInHertz(note);pitch.reset(getSampleRate(),p("portamento"));
  const double origin=p("portamento")>.001f&&from>0?from:hz;pitch.setCurrentAndTargetValue(std::log2(origin));pitch.setTargetValue(std::log2(hz));serial=++previousPitch.starts;previousPitch.pitch=origin;pitched=true;
  // A note taken over while it still sounds: what it was putting out fades over a few milliseconds instead of stopping dead.
  tailL+=lastL;tailR+=lastR;lastL=lastR=0;fresh=true;vel=velocity;pitchWheelMoved(wheel);
  a.reset();b.reset();highpass.reset();down.reset();bass=0;env.reset();filterEnv.reset();gate.reset();envelopes();env.noteOn();filterEnv.noteOn();gate.noteOn();lfoAge=0;pressure=0;lfoOffset=0;trigPhase=0;lfoLast=0;held=true;
 }
 void stopNote(float,bool tail) override {held=false;env.noteOff();filterEnv.noteOff();gate.noteOff();if(!tail){env.reset();filterEnv.reset();gate.reset();clearCurrentNote();}}
 void pitchWheelMoved(int value) override {wheelValue=value;}
 void controllerMoved(int cc,int value) override {if(cc==1)wheel=float(value)/127.f;}
 // Pressure: channel aftertouch, or polyphonic aftertouch on this voice's key. It opens the filters (PRESS) and,
 // like the mod wheel, brings in the vibrato.
 void channelPressureChanged(int value) override {pressure=float(value)/127.f;}
 void aftertouchChanged(int value) override {pressure=float(value)/127.f;}
 void setUnison(float cents,float amplitude){unisonCents=cents;voiceGain=amplitude;}
 void setNoiseSeed(int seed){random.setSeed(seed);}
 void prepare(double sr){env.setSampleRate(sr);filterEnv.setSampleRate(sr);gate.setSampleRate(sr);highpass.prepare({2.*sr,1024,1});highpass.setType(dsp::StateVariableTPTFilterType::highpass);a.prepare(2.*sr);b.prepare(2.*sr);down.reset();} // the audio path runs at twice the rate
 static double blep(double t,double dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
 static float wave(double t,double dt,float width,float mix){double saw=2*t-1-blep(t,dt);double pulse=(t<width?1.:-1.)+blep(t,dt)-blep(std::fmod(t+1-width,1.),dt);return float(saw*(1-mix)+pulse*mix);}
 static float oscillator(int shape,double t,double dt,float width,float mix){switch(shape){case 1:return wave(t,dt,width,0);case 2:return wave(t,dt,width,1);case 3:return float(2./MathConstants<double>::pi*std::asin(std::sin(t*MathConstants<double>::twoPi)));case 4:return float(std::sin(t*MathConstants<double>::twoPi));default:return wave(t,dt,width,mix);}}
 void renderNextBlock(AudioBuffer<float>& out,int start,int count) override {
  if(!isVoiceActive()){lastL=lastR=tailL=tailR=0;return;} // silent since the last block: nothing left to fade
  envelopes();const double sr=getSampleRate();
  // High-pass switch: 0 = bass boost, 1 = flat, 2 and 3 = two fixed high-pass settings.
  // HPF, one continuous fader: the bottom quarter boosts the bass (less and less towards the flat zone), a short flat
  // zone, then a high-pass rising from 40 Hz to 1 kHz.
  const float hpfPos=p("hpfFreq");const int hpfMode=hpfPos<.25f?0:(hpfPos<=.3f?1:2);const float boostGain=bassBoostGain*jlimit(0.f,1.f,(.25f-hpfPos)/.25f);
  if(hpfMode==2)highpass.setCutoffFrequency(40.f*std::pow(25.f,(hpfPos-.3f)/.7f));
  const float bassRate=1.f-std::exp(-MathConstants<float>::twoPi*bassBoostHz/float(2.*sr)); // runs at twice the rate (see below)
  a.configure(int(p("filterAType")),int(p("slope")),p("resonance"));b.configure(int(p("filterBType")),int(p("slopeB")),p("resonanceB"));
  const int shape1=int(p("osc1Shape")),shape2=int(p("osc2Shape")),route=int(p("filterRoute"));
  const float cutoff=p("cutoff"),cutoffB=p("cutoffB"),amount=p("env"),amountB=p("envB"),width=p("width"),width2=p("osc2Width"),mix=p("wave"),layer=p("oscLayer"),level=p("osc1Level"),subLevel=p("sub"),noise=p("noise"),velocity=p("velocity"),pwmDepth=p("pwm"),blend=p("filterBlend"),pitchDepth=p("lfoPitch"),vibratoDepth=p("wheelDepth"),filterDepth=p("lfoFilter"),envPitch=p("envPitch");
  const bool linked=p("envLink")>.5f;
  // VCA mode GATE: the amplifier simply opens and closes with the key; the ADSR keeps running for a filter that follows it.
  // Filter A envelope can be inverted. PWM can come from the LFO, stay manual (WIDTH only), or follow the filter envelope.
  // VCA source: 0 the amp envelope, 1 the gate, 2 the filter envelope.
  const int vcaSource=int(p("vcaMode"));const bool gated=vcaSource==1;const float polarity=p("envInvert")>.5f?-1.f:1.f;const int pwmSource=int(p("pwmSource"));
  // The bender can also move both filter cutoffs, by up to the set number of octaves.
  // SYNC restarts oscillator II each time oscillator I completes a cycle; the pitch envelope then sweeps II only,
  // which is what gives the classic sync lead. X-MOD lets oscillator II bend the frequency of oscillator I.
  const bool sync=p("oscSync")>.5f;const double crossMod=p("crossMod")*1.5;
  const float bendCutoff=std::pow(2.f,float((wheelValue-8192)/8192.)*p("bendFilter"));
  const double ratio1=std::pow(2.,int(p("osc1Octave"))-2),ratio2=std::pow(2.,int(p("osc2Octave"))-2+p("osc2Tune")/12.+p("detune")/1200.);
  const double bend=std::pow(2.,((wheelValue-8192)/8192.)*p("bendRange")/12.+unisonCents/1200.+p("masterTune")/1200.); // TUNE: the whole instrument, in cents
  // LFO delay: after a new note the LFO stays out for the first part of the delay time, then fades in.
  // The mod wheel vibrato is not delayed.
  const float lfoDelay=p("lfoDelay")*float(sr),lfoHold=lfoDelay*.6f,lfoRamp=jmax(1.f,lfoDelay*.4f);
  const float manualRate=1.f-std::exp(-1.f/(.015f*float(sr))); // LFO MAN: the LFO comes and goes over about 15 ms, never with a click
  const float tracking=std::pow(float(hz/261.625565),p("keytrack"));
  // VEL > FILTER: a softer note is darker, by up to four octaves; a full-velocity note is unchanged.
  const float velocityOpen=std::pow(2.f,p("velFilter")*4.f*(vel-1.f)),pressFilter=p("atFilter")*3.f,pressLfo=p("atLfo")*2.f,pressVolume=p("atVolume"),pressDrive=p("atDrive"),pressRate=1.f-std::exp(-1.f/(.01f*float(sr)));
  // Eight-voice instrument: each voice runs its own copy of the LFO at a slightly different speed (AGE), starting in
  // step with the others at each note and drifting apart, so a held chord moves instead of wobbling as one block.
  const bool ownLfo=vintage&&p("age")>0.f;const double lfoDrift=bus.step*double(p("age")*lfoRateError*lfoTune);
  // KEY TRIG: this voice's LFO starts its cycle at each note. ENV TRIG: while the key is down, both envelopes start
  // again at each LFO cycle (rhythmic, gated sounds). The mod wheel can also open both filters (WHL>FLT).
  const bool keyTrig=p("lfoKeyTrig")>.5f,envTrig=p("envLfoTrig")>.5f;const double ownStep=bus.step+(ownLfo?lfoDrift:0.);
  const float wheelOpen=std::exp2(p("wheelFilter")*3.f*wheel);
  const float age=vintage?p("age"):0.f,cutSpread=std::pow(2.f,age*cutoffError*cutOffset),driftRate=1.f-std::exp(-1.f/(driftSeconds*float(sr)));
  // PRE drive (eight-voice instrument): each voice is overloaded after its filters and ahead of its amplifier, on a
  // slightly lopsided curve that adds even harmonics too. Placed after the filters, it never takes anything away from
  // their resonance or their envelope sweep. Loud signals keep their level, quiet ones come up: the sound thickens.
  const float grit=vintage?p("voiceDrive"):0.f,push=1.f+3.f*grit,lean=.15f*grit,leanOffset=std::tanh(lean);
  // Quiet signals come up by (1 + PRE) only and peaks go down, so the voice keeps about the same loudness whatever PRE is set to.
  // Blended in over the first quarter of the knob so that turning it up from zero is seamless.
  const float pushScale=(1.f+grit)/push,driveMix=jmin(1.f,grit*4.f);
  const float dcRate=1.f-std::exp(-MathConstants<float>::twoPi*10.f/float(2.*sr)),punchDecay=std::exp(-1.f/(punchSeconds*float(sr)));
  // Faders are read once per block: these follow them over a few milliseconds, so moving one while a note sounds
  // glides instead of stepping. A new note starts on the current settings at once.
  const float follow5ms=1.f-std::exp(-1.f/(.005f*float(sr))),tailDecay=std::exp(-1.f/(.004f*float(sr)));
  if(fresh){cutNow=cutoff;cutBNow=cutoffB;levelNow=level;layerNow=layer;subNow=subLevel;noiseNow=noise;fresh=false;}
  // Equal-power placement; exactly 1 on both sides at the centre.
  const float angle=(pan+1.f)*MathConstants<float>::pi*.25f,gainL=pan==0.f?1.f:1.41421356f*std::cos(angle),gainR=pan==0.f?1.f:1.41421356f*std::sin(angle);
  for(int i=0;i<count;++i){
   float e=env.getNextSample(),fe=filterEnv.getNextSample();if(linked)fe=e;
   const float open=gate.getNextSample(),amp=gated?open:(vcaSource==2?fe:e);
   pressureNow+=pressRate*(pressure-pressureNow);
   float lfoNow=bus.lfo[size_t(start+i)],t=bus.phase[size_t(start+i)];
   if(keyTrig){trigPhase+=ownStep;trigPhase-=std::floor(trigPhase);t=float(trigPhase);lfoNow=ModulationBus::wave(bus.shape,t);}
   else if(ownLfo){lfoOffset+=lfoDrift;lfoOffset-=std::floor(lfoOffset);t=bus.phase[size_t(start+i)]+float(lfoOffset);t-=std::floor(t);lfoNow=ModulationBus::wave(bus.shape,t);}
   if(envTrig&&held&&t<lfoLast-.5f){env.noteOn();filterEnv.noteOn();} // the LFO has just started a new cycle
   lfoLast=t;
   float lfoFade=lfoDelay<1.f?1.f:jlimit(0.f,1.f,(lfoAge-lfoHold)/lfoRamp);
   if(bus.manual){ // LFO MAN: the delay runs from the press of LFO TRIG, not from the note
    const float since=bus.manualAge<0?-1.f:float(bus.manualAge)+float(i);
    const float target=since<0?0.f:(lfoDelay<1.f?1.f:jlimit(0.f,1.f,(since-lfoHold)/lfoRamp));
    manualLevel+=manualRate*(target-manualLevel);lfoFade=manualLevel;}
   const float modulation=lfoNow*lfoFade;
   if(lfoAge<1.e7f)lfoAge+=1.f;
   cutNow+=follow5ms*(cutoff-cutNow);cutBNow+=follow5ms*(cutoffB-cutBNow);levelNow+=follow5ms*(level-levelNow);layerNow+=follow5ms*(layer-layerNow);subNow+=follow5ms*(subLevel-subNow);noiseNow+=follow5ms*(noise-noiseNow);
   const bool gliding=pitch.isSmoothing();const double pitchNow=pitch.getNextValue(),frequency=gliding?std::exp2(pitchNow):hz; // exactly the note once the glide is over
   if(serial==previousPitch.starts)previousPitch.pitch=frequency;
   const double base=frequency*bend/sr*std::pow(2.,(modulation*pitchDepth+lfoNow*vibratoDepth*wheel+(sync?0.f:fe*envPitch))/12.);
   double detune1=1.,detune2=1.;
   if(age>0.f){ // each oscillator wanders slowly around its own small tuning error (cents turned into a ratio; exact enough this close to 1)
    if(--driftCount<=0){driftTarget1=wander.nextFloat()*2-1;driftTarget2=wander.nextFloat()*2-1;driftCount=int(sr*(.4+1.2*wander.nextFloat()));}
    drift1+=driftRate*(driftTarget1-drift1);drift2+=driftRate*(driftTarget2-drift2);
    detune1=1.+age*(tuningError*tune1+driftCents*drift1)*.00057779;detune2=1.+age*(tuningError*tune2+driftCents*drift2)*.00057779;
   }
   const double dt=jlimit(.0000001,.45,base*ratio1*detune1),dt2=jlimit(.0000001,.45,base*ratio2*detune2*(sync?std::pow(2.,fe*envPitch/12.):1.));
   const float pwmMove=pwmSource==0?.18f*pwmDepth*modulation:pwmSource==2?.4f*pwmDepth*fe:pwmSource==3?-.4f*pwmDepth*fe:pwmSource==4?.4f*pwmDepth*e:pwmSource==5?-.4f*pwmDepth*e:0.f; // LFO, MAN, filter env +/-, amp env +/-
   float pwm=jlimit(.08f,.92f,width+pwmMove),pwm2=jlimit(.08f,.92f,(shape2==0?width:width2)+pwmMove);
   punch*=punchDecay; // a few milliseconds of extra brightness and level at the start of each note: the percussive front
   float ca=jlimit(25.f,float(sr*.45),(1.f+punchCutoff*punch)*cutNow*cutSpread*tracking*velocityOpen*wheelOpen*std::exp2(pressFilter*pressureNow)*bendCutoff*std::pow(2.f,fe*amount*polarity*6+modulation*filterDepth+lfoNow*pressLfo*pressureNow));
   float cb=jlimit(25.f,float(sr*.45),cutBNow*cutSpread*tracking*velocityOpen*wheelOpen*std::exp2(pressFilter*pressureNow)*bendCutoff*std::pow(2.f,fe*amountB*6+modulation*filterDepth+lfoNow*pressLfo*pressureNow));
   // Oscillators, mixer, filters and drive run at twice the sample rate and come back down through a half-band
   // filter: the overtones that sync, cross modulation and every saturation throw above the audio band are removed
   // instead of folding back down as metallic, out-of-tune noise.
   // Pressure can also push the drive (from any PRE setting, and in both voice counts), add LFO on the filters, and
   // lift the level by up to 6 dB: the harder the key is pressed, the more the note opens, moves, growls and grows.
   float g=grit,gPush=push,gLean=lean,gOffset=leanOffset,gScale=pushScale,gMix=driveMix;
   if(pressDrive>0.f&&pressureNow>.0005f){g=jmin(1.f,grit+pressDrive*pressureNow);gPush=1.f+3.f*g;gLean=.15f*g;gOffset=std::tanh(gLean);gScale=(1.f+g)/gPush;gMix=jmin(1.f,g*4.f);}
   float twice[2];
   for(int k=0;k<2;++k){
    const double h1=dt*.5,h2=dt2*.5,hx=crossMod>0?jlimit(.0000001,.45,h1*(1.+crossMod*lastOsc2)):h1;
    const float second=oscillator(shape2,phase2,h2,pwm2,mix);lastOsc2=second;
    float sample=(levelNow*oscillator(shape1,phase,hx,pwm,mix)+layerNow*second)/(1+layerNow*.82f);
    sample=.7f*sample+.3f*subNow*wave(sub,h1*.5,.5f,1);sample+=noiseNow*.2828f*(random.nextFloat()*2-1); // noise level kept: half of it is filtered out on the way down
    if(hpfMode>=2)sample=highpass.processSample(0,sample);else if(hpfMode==0){bass+=bassRate*(sample-bass);sample+=boostGain*bass;}
    switch(route){case 1:sample=b.process(sample,cb);break;case 2:sample=b.process(a.process(sample,ca),cb);break;case 3:sample=a.process(b.process(sample,cb),ca);break;case 4:sample=(1-blend)*a.process(sample,ca)+blend*b.process(sample,cb);break;default:sample=a.process(sample,ca);}
    if(g>0.f){sample+=gMix*((std::tanh(gPush*sample+gLean)-gOffset)*gScale-sample);dc+=dcRate*(sample-dc);sample-=dc;} // the lopsided curve leaves a small offset: removed here
    twice[k]=sample;
    phase+=hx;
    if(phase>=1.){phase-=std::floor(phase);if(sync)phase2=phase/hx*h2-h2;} // restart II at the exact point inside the step where I wrapped
    phase2+=h2;phase2-=std::floor(phase2);sub+=h1*.5;sub-=std::floor(sub);
   }
   float sample=down.process(twice[0],twice[1]);
   sample*=amp*(1-velocity+velocity*vel)*.18f*voiceGain*(1.f+punchLevel*punch)*(pressVolume>0.f?std::exp2(pressVolume*pressureNow):1.f);
   lastL=sample*gainL;lastR=sample*gainR;tailL*=tailDecay;tailR*=tailDecay;
   for(int ch=0;ch<out.getNumChannels();++ch)out.addSample(ch,start+i,ch==0?lastL+tailL:(ch==1?lastR+tailR:sample));
   if(!(gated?gate.isActive():(vcaSource==2?filterEnv.isActive():env.isActive()))){clearCurrentNote();break;}
  }
 }
private:
 AudioProcessorValueTreeState& state;GlideMemory& previousPitch;uint64 serial=0;const ModulationBus& bus;SmoothedValue<double> pitch;Contour env,filterEnv,gate;FilterSlot a,b;HalfBand down;dsp::StateVariableTPTFilter<float> highpass;Random random{12345};
 float cutNow=0,cutBNow=0,levelNow=0,layerNow=0,subNow=0,noiseNow=0,lastL=0,lastR=0,tailL=0,tailR=0;bool fresh=true,pitched=false;
 double phase=0,phase2=0,sub=0,hz=440;float vel=0,wheel=0,unisonCents=0,voiceGain=1,lfoAge=0,bass=0,lastOsc2=0;
 bool vintage=false;float tune1=0,tune2=0,cutOffset=0,timeOffset=0,pan=0,drift1=0,drift2=0,driftTarget1=0,driftTarget2=0;int driftCount=0;Random wander{777};
 // Vintage spread at AGE = 10, to adjust by ear: tuning error and drift in cents, cutoff error in octaves, envelope time error, drift smoothing.
 float keyStretch=1,punch=0,dc=0;
 // Vintage envelopes and attack, to adjust by ear: time ratio per octave, length of the punch, its share of cutoff and of level.
 static constexpr float keyFollow=.85f,punchSeconds=.006f,punchCutoff=.4f,punchLevel=.25f;
 float pressure=0,pressureNow=0,lfoTune=0,lfoLast=0,manualLevel=0;double lfoOffset=0,trigPhase=0;bool held=false;
 static constexpr float lfoRateError=.12f; // LFO speed spread between voices at AGE = 10
 static constexpr float tuningError=6.f,driftCents=4.f,cutoffError=.12f,envelopeError=.15f,driftSeconds=.6f;
 // Tuning values for the high-pass switch: starting points to adjust by ear, not hardware measurements.
 static constexpr float bassBoostHz=90.f,bassBoostGain=1.2f;int wheelValue=8192;
};
