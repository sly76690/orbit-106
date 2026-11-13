#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <map>

using namespace juce;
#include "Arpeggiator.h"
#include "PresetStorage.h"
#include "EnsembleChorus.h"
#include "Reverbs.h"
#include "CharacterDelay.h"
#include "NoteHold.h"
class Orbit;
struct Sound : SynthesiserSound {
 bool appliesToNote(int) override { return true; }
 bool appliesToChannel(int) override { return true; }
};
#include "Voice.h"
#include "VoiceAllocator.h"
class Orbit : public AudioProcessor, private AudioProcessorValueTreeState::Listener {
public:
 AudioProcessorValueTreeState state;
 MidiKeyboardState keyboard;
 Orbit() : AudioProcessor(BusesProperties().withOutput("Stereo",AudioChannelSet::stereo(),true)), state(*this,nullptr,"OrbitState",parameters()), synth(state,true), classic(state) {
  for(int i=0;i<8;++i){auto* voice=new Voice(state,previousJupiter,modulation);voice->setNoiseSeed(1001+i);voice->setVintage(i);synth.addVoice(voice);} synth.addSound(new Sound()); synth.setNoteStealingEnabled(true);
  for(int i=0;i<6;++i){auto* voice=new Voice(state,previousClassic,modulation);voice->setNoiseSeed(2001+i);classic.addVoice(voice);} classic.addSound(new Sound());classic.setNoteStealingEnabled(true);
  for(auto* param:getParameters()) if(auto* identified=dynamic_cast<AudioProcessorParameterWithID*>(param)) state.addParameterListener(identified->paramID,this);
  state.state.setProperty("stateVersion",3,nullptr);
 }
 ~Orbit() override {for(auto* param:getParameters()) if(auto* identified=dynamic_cast<AudioProcessorParameterWithID*>(param)) state.removeParameterListener(identified->paramID,this);}
 String userPresetName() const {return state.state.getProperty("userPresetName").toString();}
 // User presets live in the user's library, so they survive moving or rebuilding the project.
 static File presetRoot(){return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile("SF Instruments").getChildFile("Orbit 106");}
 StringArray userPresetNames() const {return PresetStorage(presetRoot()).names();}
 bool userPresetExists(const String& name) const {return PresetStorage(presetRoot()).exists(name);}
 Result saveUserPreset(const String& name,bool replace=false){auto saved=state.copyState();saved.setProperty("program",currentProgram.load(),nullptr);saved.setProperty("modified",false,nullptr);saved.setProperty("userPresetName",name,nullptr);auto xml=saved.createXml();auto result=PresetStorage(presetRoot()).save(name,*xml,replace);if(result.wasOk()){state.state.setProperty("userPresetName",name,nullptr);modified.store(false);}return result;}
 Result loadUserPreset(const String& name){auto xml=PresetStorage(presetRoot()).load(name);if(!xml)return Result::fail("Preset absent, invalide ou inaccessible dans Presets/User.");MemoryBlock data;copyXmlToBinary(*xml,data);setStateInformation(data.getData(),int(data.getSize()));state.state.setProperty("userPresetName",name,nullptr);modified.store(false);return Result::ok();}
 bool isPatchModified() const {return modified.load();}
  // Host pages of eight, in the order of the front panel.
  static constexpr const char* hostPages[]={
   "wave","width","pwm","sub","noise","osc1Shape","osc1Octave","osc1Level",                    // OSC I
   "oscLayer","osc2Width","osc2Tune","detune","osc2Shape","osc2Octave","oscSync","crossMod",     // OSC II
   "cutoff","resonance","env","keytrack","lfoFilter","hpf","slope","filterAType",               // FILTER A
   "attack","decay","sustain","release","velocity","vcaMode","portamento","legato",            // AMP
   "fattack","fdecay","fsustain","frelease","envLink","envInvert","envPitch","pwmSource",       // FILTER ENVELOPE
   "rate","lfoDelay","lfoShape","lfoPitch","wheelDepth","bendRange","bendFilter","unisonSpread",// LFO, BEND
   "filterBType","cutoffB","resonanceB","envB","slopeB","filterRoute","filterBlend","voiceDrive",// FILTER B, ROUTING
   "mode","voiceMode","age","voiceWidth","hold","chorusMode","chorus","gain",                  // VOICES, CHORUS, OUTPUT
   "arpOn","arpSync","arpPattern","arpDivision","arpOctaves","arpTempo","arpGate","fxDrive",    // ARPEGGIO
   "delayTime","delaySync","delayFeedback","delayMod","delayMix","reverbType","reverbSize","reverbDamp", // DELAY, REVERB
   "reverbColor","reverbMix"};
  // Short names for the host and controller displays (Push shows about a dozen characters).
  static String label(const String& id,const String& longName){
   static const std::map<String,String> names{
    {"wave","Osc1 SawPls"},{"width","Osc1 Width"},{"pwm","Osc1 PWM"},{"sub","Sub Level"},{"noise","Noise"},{"osc1Shape","Osc1 Wave"},{"osc1Octave","Osc1 Octave"},{"osc1Level","Osc1 Level"},
    {"oscLayer","Osc2 Level"},{"osc2Width","Osc2 Width"},{"osc2Tune","Osc2 Tune"},{"detune","Osc2 Fine"},{"osc2Shape","Osc2 Wave"},{"osc2Octave","Osc2 Octave"},{"oscSync","Osc2 Sync"},{"crossMod","X-Mod"},
    {"cutoff","A Freq"},{"resonance","A Reso"},{"env","A Env"},{"keytrack","Filter Kybd"},{"lfoFilter","Filter LFO"},{"hpf","HPF"},{"slope","A Slope"},{"filterAType","A Model"},
    {"attack","Amp Attack"},{"decay","Amp Decay"},{"sustain","Amp Sustain"},{"release","Amp Release"},{"velocity","Velocity"},{"vcaMode","VCA Mode"},{"portamento","Portamento"},{"legato","Legato"},
    {"fattack","Flt Attack"},{"fdecay","Flt Decay"},{"fsustain","Flt Sustain"},{"frelease","Flt Release"},{"envLink","Flt Env Src"},{"envInvert","A Env Pol"},{"envPitch","Env>Pitch"},{"pwmSource","PWM Source"},
    {"rate","LFO Rate"},{"lfoDelay","LFO Delay"},{"lfoShape","LFO Wave"},{"lfoPitch","LFO Pitch"},{"wheelDepth","Wheel Vib"},{"bendRange","Bend Range"},{"bendFilter","Bend>Filter"},{"unisonSpread","Uni Spread"},
    {"filterBType","B Model"},{"cutoffB","B Freq"},{"resonanceB","B Reso"},{"envB","B Env"},{"slopeB","B Slope"},{"filterRoute","Routing"},{"filterBlend","A/B Mix"},{"voiceDrive","Drive Pre"},
    {"mode","Voices"},{"voiceMode","Assign"},{"age","Age"},{"voiceWidth","Width"},{"hold","Hold"},{"chorusMode","Chorus"},{"chorus","Chorus Mix"},{"gain","Volume"},
    {"arpOn","Arp On"},{"arpSync","Arp Sync"},{"arpPattern","Arp Mode"},{"arpDivision","Arp Step"},{"arpOctaves","Arp Range"},{"arpTempo","Arp Tempo"},{"arpGate","Arp Gate"},{"fxDrive","Drive Post"},
    {"delayTime","Dly Time"},{"delaySync","Dly Sync"},{"delayFeedback","Dly Fdbk"},{"delayMod","Dly Mod"},{"delayMix","Dly Level"},{"reverbType","Rev Type"},{"reverbSize","Rev Size"},{"reverbDamp","Rev Damp"},
    {"reverbColor","Rev Color"},{"reverbMix","Rev Level"}};
   const auto found=names.find(id);return found!=names.end()?found->second:longName;
  }
 static AudioProcessorValueTreeState::ParameterLayout parameters() {
   // Parameters are made in any order, then handed to the host page by page (see hostPages): a controller that shows
   // eight at a time, such as Push, walks through the instrument section by section. Ids never change, so presets,
   // sessions and automation are unaffected; only the order and the short names the host shows are set here.
   std::vector<std::unique_ptr<RangedAudioParameter>> made;
   auto keep=[&made](auto parameter){made.push_back(std::move(parameter));};
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"mode",1},label("mode","Voices"),StringArray{"6 voices","8 voices"},0));
  // Values are shown on the scale printed around the control: the eleven marks read 0 to 10 (or -5 to +5 for a
  // control centred on zero), so mark 3 always displays 3.0. Controls with a real unit add it in brackets,
  // for example "4.0 (120 BPM)" or "6.3 (1800 Hz)".
  auto add=[&](const char* id,const char* name,float lo,float hi,float def,float skew=1.f) {
   const NormalisableRange<float> range(lo,hi,0,skew);const bool centred=lo<0.f,whole=hi>=100.f,fine=lo>0.f&&lo<.06f;
   String unit=String(name).fromFirstOccurrenceOf("(",false,false).upToFirstOccurrenceOf(")",false,false);
   const String key(id);if(key=="arpTempo")unit="BPM";if(key=="fattack"||key=="fdecay"||key=="frelease")unit="s";if(key=="osc2Tune")unit="st";
   if(unit=="semitones")unit="st";if(unit=="octaves")unit="oct";if(unit=="cents")unit="ct";
   auto text=[range,centred,whole,fine,unit](float v,int){
    const float mark=range.convertTo0to1(range.snapToLegalValue(v))*10.f-(centred?5.f:0.f);
    String shown=(centred&&mark>.04f?"+":"")+String(mark,1);
    if(unit.isNotEmpty())shown+=" ("+(whole?String(roundToInt(v)):String(v,fine&&std::abs(v)<1.f?2:1))+" "+unit+")";
    return shown;
   };
   auto parse=[range,centred](const String& typed){return range.convertFrom0to1(jlimit(0.f,1.f,(typed.getFloatValue()+(centred?5.f:0.f))/10.f));};
   keep(std::make_unique<AudioParameterFloat>(ParameterID{id,1},label(id,name),range,def,AudioParameterFloatAttributes().withStringFromValueFunction(text).withValueFromStringFunction(parse)));
  };
  add("wave","Saw / Pulse",0,1,0.35f);add("width","Pulse width",0.08f,0.92f,0.5f);add("sub","Sub level",0,1,0.7f);add("detune","Detune (cents)",0,30,8);
  add("cutoff","Cutoff (Hz)",25,18000,1800,0.3f);add("resonance","Resonance",0,0.95f,0.2f);add("env","Filter envelope",0,1,0.35f);
  add("attack","Attack (s)",0.002f,5,0.02f,0.35f);add("decay","Decay (s)",0.01f,5,0.6f,0.4f);add("sustain","Sustain",0,1,0.7f);add("release","Release (s)",0.01f,8,0.8f,0.4f);
  add("rate","LFO (Hz)",0.05f,12,0.6f,0.4f);add("pwm","PWM depth",0,1,0.3f);add("chorus","Chorus",0,1,0.35f);add("gain","Output",0,1,0.7f);
  add("fattack","Filter attack",0.002f,5,0.02f,0.35f);add("fdecay","Filter decay",0.01f,5,0.6f,0.4f);add("fsustain","Filter sustain",0,1,0.3f);add("frelease","Filter release",0.01f,8,0.8f,0.4f);
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"hpf",1},label("hpf","High-pass"),StringArray{"0 BASS BOOST","1 FLAT","2","3"},1));add("keytrack","Filter key track",0,1,0.5f);add("noise","Noise",0,1,0);add("velocity","Velocity",0,1,0);
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"slope",1},label("slope","Filter A slope"),StringArray{"12 dB","24 dB"},1));
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"chorusMode",1},label("chorusMode","Chorus mode"),StringArray{"OFF","I","II","I+II"},1));
  add("portamento","Portamento (s)",0,2,0,0.4f);
  keep(std::make_unique<AudioParameterBool>(ParameterID{"arpOn",1},label("arpOn","Arpeggiator"),false));
  keep(std::make_unique<AudioParameterBool>(ParameterID{"arpSync",1},label("arpSync","Host tempo"),false));
  add("arpTempo","Arp tempo",40,240,120);add("arpGate","Arp gate",0.1f,0.95f,0.65f);
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"arpDivision",1},label("arpDivision","Arp division"),StringArray{"1/4","1/8","1/16"},1));
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"arpPattern",1},label("arpPattern","Arp pattern"),StringArray{"UP","DOWN","UP/DOWN","RANDOM"},0));
  keep(std::make_unique<AudioParameterChoice>(ParameterID{"arpOctaves",1},label("arpOctaves","Arp octaves"),StringArray{"1 OCT","2 OCT","3 OCT"},0));
  add("fxDrive","Drive",0,1,0);add("delayTime","Delay time (s)",0.03f,1.5f,0.3f);add("delayFeedback","Delay feedback",0,1.1f,0.3f);add("delayMod","Delay modulation",0,1,0.3f);add("delayMix","Delay mix",0,0.65f,0);
  add("reverbSize","Reverb size",0,1,0.5f);add("reverbDamp","Reverb damping",0,1,0.5f);add("reverbMix","Reverb mix",0,0.65f,0);add("reverbColor","Reverb color",0,1,0.4f);
  add("oscLayer","Oscillator II layer",0,1,0);
  keep(std::make_unique<AudioParameterBool>(ParameterID{"envLink",1},label("envLink","Link filter to amp envelope"),true));
  auto choice=[&](const char* id,const char* name,StringArray values,int initial=0){keep(std::make_unique<AudioParameterChoice>(ParameterID{id,1},label(id,name),values,initial));};
  choice("osc1Shape","Oscillator I shape",{"Legacy mix","Saw","Pulse","Triangle","Sine"});
  choice("osc2Shape","Oscillator II shape",{"Legacy mix","Saw","Pulse","Triangle","Sine"});
  add("osc1Level","Oscillator I level",0,1,1);add("osc2Width","Oscillator II pulse width",.08f,.92f,.5f);
  choice("osc1Octave","Oscillator I octave",{"-2","-1","0","+1","+2"},2);
  choice("osc2Octave","Oscillator II octave",{"-2","-1","0","+1","+2"},2);
  add("osc2Tune","Oscillator II semitones",-12,12,0);
  choice("lfoShape","LFO waveform",{"Triangle","Sine","Saw","Square"});
  add("lfoDelay","LFO delay (s)",0,3,0,.5f);add("lfoPitch","LFO pitch (semitones)",0,12,0);add("lfoFilter","LFO filter (octaves)",0,4,0);
  add("envPitch","Envelope pitch (semitones)",-24,24,0);add("wheelDepth","Mod wheel vibrato (semitones)",0,12,1);
  choice("reverbType","Reverb type",{"VINT","PLATE","SPACE"});
  choice("delaySync","Delay sync",{"FREE","1/2","1/4 D","1/4","1/4 T","1/8 D","1/8","1/8 T","1/16 D","1/16","1/16 T"});
  keep(std::make_unique<AudioParameterBool>(ParameterID{"hold",1},label("hold","Hold"),false));keep(std::make_unique<AudioParameterBool>(ParameterID{"oscSync",1},label("oscSync","Oscillator II sync"),false));add("crossMod","Cross modulation II > I",0,1,0);
  choice("vcaMode","VCA mode",{"Envelope","Gate"});choice("envInvert","Filter A envelope polarity",{"Normal","Inverted"});choice("pwmSource","PWM source",{"LFO","Manual","Envelope"});
  add("bendFilter","Pitch bend to filter (octaves)",0,3,0);
  choice("voiceMode","Voice mode",{"Poly","Mono","Unison"});
  keep(std::make_unique<AudioParameterBool>(ParameterID{"legato",1},label("legato","Legato envelopes"),false));
  add("voiceDrive","Voice drive",0,1,.3f);add("age","Age",0,1,.3f);add("voiceWidth","Voice width",0,1,.5f);
  add("unisonSpread","Unison detune (cents)",0,40,12);add("bendRange","Pitch bend (semitones)",0,24,2);
  choice("filterRoute","Filter routing",{"A only","B only","A > B","B > A","Parallel"});
  choice("filterAType","Filter A model",{"Ladder","State variable"});
  choice("filterBType","Filter B model",{"Ladder","State variable"},1);
  add("cutoffB","Filter B cutoff (Hz)",25,18000,1800,.3f);add("resonanceB","Filter B resonance",0,.95f,.2f);
  choice("slopeB","Filter B slope",{"12 dB","24 dB"},1);
  add("filterBlend","Parallel A / B balance",0,1,.5f);add("envB","Filter B envelope",-1,1,.35f);
   AudioProcessorValueTreeState::ParameterLayout l;
   for(const char* id:hostPages)for(auto& parameter:made)if(parameter&&parameter->paramID==id)l.add(std::move(parameter));
   for(auto& parameter:made)if(parameter)l.add(std::move(parameter)); // anything not placed on a page goes last, never lost
  return l;
 }
 const String getName() const override {return "Orbit 106";}
 bool acceptsMidi() const override {return true;} bool producesMidi() const override {return false;}
 double getTailLengthSeconds() const override {return 30;}
 struct Patch {const char* name; int engine; float cutoff, resonance, attack, decay, sustain, release, wave, sub, detune, chorus, amount;};
 static const std::array<Patch,24>& patches() {
  static const std::array<Patch,24> bank{{
   {"INIT / Classic",0,1800,.2f,.02f,.6f,.7f,.8f,.35f,.7f,8,.35f,.35f},
   {"PAD / Velvet",0,1200,.15f,1.2f,1.4f,.8f,2.8f,.3f,.5f,8,.65f,.25f},
   {"BASS / Midnight",0,350,.3f,.004f,.28f,.25f,.18f,.1f,.9f,8,.12f,.6f},
   {"PLUCK / Glass",0,900,.5f,.003f,.32f,.05f,.4f,.65f,.2f,8,.45f,.7f},
   {"BRASS / Solar",1,1300,.2f,.05f,.5f,.65f,.35f,.1f,0,12,.2f,.5f},
   {"STRINGS / Aurora",1,2100,.12f,.7f,1.1f,.8f,2.2f,.2f,0,18,.5f,.2f},
   {"KEYS / Neon",1,1700,.25f,.006f,.7f,.3f,.65f,.55f,0,7,.35f,.45f},
   {"PAD / Slow Horizon",1,650,.35f,2.1f,2.0f,.8f,4.0f,.4f,0,22,.55f,.6f},
   {"BASS / Round Sub",0,220,.12f,.003f,.35f,.55f,.15f,.85f,1,3,0,.35f},
   {"BASS / Rubber Funk",0,420,.55f,.003f,.2f,.12f,.12f,.45f,.8f,5,.08f,.72f},
   {"BASS / Hybrid Drive",0,650,.2f,.005f,.35f,.4f,.2f,.12f,.75f,9,.05f,.45f},
   {"BASS / Pulse Runner",1,510,.32f,.003f,.18f,.25f,.13f,.9f,.6f,5,0,.5f},
   {"BASS / Deep Velvet",0,180,.18f,.012f,.65f,.8f,.3f,.25f,1,4,.15f,.18f},
   {"BRASS / Big Leap",1,2500,.08f,.008f,.65f,.72f,.22f,0,0,7,0,.36f},
   {"BRASS / Big Leap Wide",1,2100,.1f,.01f,.75f,.75f,.3f,0,0,13,.18f,.4f},
   {"BRASS / Golden Horns",1,1400,.18f,.07f,.55f,.7f,.4f,.1f,0,10,.12f,.5f},
   {"BRASS / Hybrid Stabs",0,1700,.24f,.006f,.22f,.2f,.18f,.1f,.25f,11,.1f,.6f},
   {"BRASS / Soft Ensemble",1,900,.13f,.18f,.8f,.7f,.7f,.2f,0,15,.3f,.35f},
   {"PAD / Silk Cloud",0,800,.15f,1.8f,2.2f,.85f,3.5f,.35f,.35f,12,.6f,.3f},
   {"PAD / Hybrid Aurora",0,1200,.18f,1.2f,1.7f,.75f,3.8f,.15f,.3f,21,.55f,.3f},
   {"PAD / Star Choir",1,1500,.3f,.9f,1.4f,.7f,2.8f,.7f,0,16,.4f,.25f},
   {"PAD / Midnight Cinema",1,450,.28f,2.6f,2.5f,.8f,5,.25f,.15f,24,.5f,.5f},
   {"PAD / Glass Atmosphere",0,2400,.4f,.8f,1.8f,.55f,3,.9f,.15f,10,.65f,.2f},
   {"PAD / Warm Horizon",1,700,.1f,1.5f,2,.8f,4,.05f,.2f,18,.4f,.3f}
  }};return bank;
 }
 int getNumPrograms() override {return int(patches().size());} int getCurrentProgram() override {return currentProgram.load();}
 void setCurrentProgram(int index) override {
  if(!isPositiveAndBelow(index,getNumPrograms())) return;
  const auto& p=patches()[size_t(index)];
  // A patch change leaves the performance settings alone: output level, arpeggiator, hold, bend range.
  static const StringArray performance{"gain","arpOn","arpSync","arpTempo","arpGate","arpDivision","arpPattern","arpOctaves","hold","bendRange"};
  for(auto* param:getParameters()){if(auto* identified=dynamic_cast<AudioProcessorParameterWithID*>(param))if(performance.contains(identified->paramID))continue;param->setValueNotifyingHost(param->getDefaultValue());}
  auto set=[&](const char* id,float value){auto* param=state.getParameter(id);param->setValueNotifyingHost(param->convertTo0to1(value));};
  set("mode",float(p.engine));set("cutoff",p.cutoff);set("resonance",p.resonance);set("attack",p.attack);set("decay",p.decay);set("sustain",p.sustain);set("release",p.release);set("wave",p.wave);set("sub",p.sub);set("detune",p.detune);set("chorus",p.chorus);set("env",p.amount);
  set("fattack",p.attack*.6f);set("fdecay",p.decay);set("fsustain",p.sustain*.6f);set("frelease",p.release);
  set("oscLayer",p.engine==0?0.f:1.f);
  set("envLink",p.engine==0?1.f:0.f);
  if(index==10 || index==16 || index==19){set("oscLayer",.75f);set("envLink",0);set("slope",0);}
  if(index==10)set("fxDrive",.25f);
  if(index>=18){set("reverbMix",.18f);set("reverbSize",.7f);}
  if(index==13 || index==14){set("fattack",.012f);set("fdecay",.48f);set("fsustain",.55f);set("frelease",.22f);set("slope",0);set("keytrack",.35f);set("pwm",0);set("reverbMix",.09f);}
  currentProgram.store(index);state.state.removeProperty("userPresetName",nullptr);modified.store(false);
 }
 const String getProgramName(int index) override {return isPositiveAndBelow(index,getNumPrograms())?patches()[size_t(index)].name:String();}
 void changeProgramName(int,const String&) override {}
 void prepareToPlay(double sr,int block) override {
  arp.prepare(sr);hold.prepare();previousClassic=previousJupiter=0;modulation.lfo.assign(size_t(jmax(block,8192)),0.f);lfoPhase=0;
  sampleRate=sr;echo.prepare(sr);reverb.prepare(sr);
  synth.setCurrentPlaybackSampleRate(sr); for(int i=0;i<synth.getNumVoices();++i) static_cast<Voice*>(synth.getVoice(i))->prepare(sr);
  classic.setCurrentPlaybackSampleRate(sr); for(int i=0;i<classic.getNumVoices();++i) static_cast<Voice*>(classic.getVoice(i))->prepare(sr);
  ignoreUnused(block);chorus.prepare(sr);
  gain.reset(sr,0.02);gain.setCurrentAndTargetValue(state.getRawParameterValue("gain")->load());
 }
 void releaseResources() override {synth.allNotesOff(0,false);classic.allNotesOff(0,false);chorus.reset();keyboard.reset();arp.reset();hold.reset();echo.reset();reverb.reset();previousClassic=previousJupiter=0;}
 bool isBusesLayoutSupported(const BusesLayout& b) const override {return b.getMainOutputChannelSet()==AudioChannelSet::stereo();}
 void processBlock(AudioBuffer<float>& buffer,MidiBuffer& midi) override {
  ScopedNoDenormals noDenormals;buffer.clear();keyboard.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
  auto value=[this](const char* key){return state.getRawParameterValue(key)->load();};
  hold.process(midi,value("hold")>.5f);
  double bpm=value("arpTempo"),samplesToGrid=-1;const int division=int(value("arpDivision"));
  if(value("arpSync")>.5f) if(auto* playHead=getPlayHead()) if(auto position=playHead->getPosition()) if(auto hostBpm=position->getBpm()) {
   bpm=*hostBpm;
   // While the host plays, line the steps up with its beat grid (quarter, eighth or sixteenth notes).
   if(position->getIsPlaying()&&bpm>1) if(auto beats=position->getPpqPosition()) {
    const double steps[]={1.,.5,.25},step=steps[jlimit(0,2,division)],halfSample=.5*bpm/(60.*sampleRate);
    double into=std::fmod(*beats,step);if(into<0)into+=step;
    samplesToGrid=(into<halfSample||step-into<halfSample)?0.:(step-into)*60./bpm*sampleRate;
   }
  }
  arp.process(midi,buffer.getNumSamples(),value("arpOn")>.5f,bpm,division,int(value("arpPattern")),int(value("arpOctaves"))+1,value("arpGate"),samplesToGrid);
  int mode=state.getRawParameterValue("mode")->load()>0.5f?1:0;
  synth.updateMode();classic.updateMode();
  if(mode!=lastMode) {synth.allNotesOff(0,false);classic.allNotesOff(0,false);lastMode=mode;}
  renderLfo(buffer.getNumSamples());
  (mode==0?classic:synth).renderNextBlock(buffer,midi,0,buffer.getNumSamples());
  chorus.process(buffer,int(value("chorusMode")),value("chorus"));
  // Drive, then the echo.
  if(const float drive=value("fxDrive");drive>0) for(int c=0;c<2;++c){auto* samples=buffer.getWritePointer(c);for(int i=0;i<buffer.getNumSamples();++i)samples[i]=std::tanh(samples[i]*(1+drive*9))/(1+drive*2);}
  // Delay time: the TIME knob when free, otherwise a note value at the host tempo (the arpeggio TEMPO knob when there is no host).
  float delaySeconds=value("delayTime");
  if(const int syncStep=int(value("delaySync"));syncStep>0) {
   double tempo=value("arpTempo");
   if(auto* playHead=getPlayHead()) if(auto position=playHead->getPosition()) if(auto hostBpm=position->getBpm()) tempo=*hostBpm;
   static const double beats[]={0,2.,1.5,1.,2./3.,.75,.5,1./3.,.375,.25,1./6.}; // D = dotted, T = triplet
   double seconds=beats[jlimit(1,10,syncStep)]*60./jmax(20.,tempo);
   while(seconds>1.5)seconds*=.5; // longer than the delay line: fall back to half the value
   delaySeconds=float(seconds);
  }
  echo.process(buffer,delaySeconds,value("delayFeedback"),value("delayMix"),value("delayMod"));
  reverb.process(buffer,int(value("reverbType")),value("reverbSize"),value("reverbDamp"),value("reverbColor"),value("reverbMix"));
  gain.setTargetValue(state.getRawParameterValue("gain")->load());
  for(int i=0;i<buffer.getNumSamples();++i) {float g=gain.getNextValue();for(int c=0;c<buffer.getNumChannels();++c) buffer.setSample(c,i,std::tanh(buffer.getSample(c,i)*g));}
 }
 bool hasEditor() const override {return true;} AudioProcessorEditor* createEditor() override;
 void getStateInformation(MemoryBlock& dest) override {auto saved=state.copyState();saved.setProperty("program",currentProgram.load(),nullptr);saved.setProperty("modified",modified.load(),nullptr);auto xml=saved.createXml();copyXmlToBinary(*xml,dest);}
 void setStateInformation(const void* data,int size) override {auto xml=getXmlFromBinary(data,size);if(xml && xml->hasTagName(state.state.getType())) {auto restored=ValueTree::fromXml(*xml);
   // Sessions and presets saved before state version 2 stored chorus mode as 0 = I, 1 = II.
   if(int(restored.getProperty("stateVersion",1))<2){for(auto child:restored)if(child.getProperty("id").toString()=="chorusMode")child.setProperty("value",float(child.getProperty("value"))+1.f,nullptr);}
   // Before state version 3 the high-pass was a continuous cutoff in Hz; it is now a four-position switch.
   if(int(restored.getProperty("stateVersion",1))<3){for(auto child:restored)if(child.getProperty("id").toString()=="hpf"){const float hz=float(child.getProperty("value"));child.setProperty("value",hz<100.f?1.f:(hz<480.f?2.f:3.f),nullptr);}}
   restored.setProperty("stateVersion",3,nullptr);
   currentProgram.store(jlimit(0,getNumPrograms()-1,int(restored.getProperty("program",0))));for(auto* base:getParameters())if(auto* parameter=dynamic_cast<RangedAudioParameter*>(base)) {
   bool found=false;for(auto child:restored)if(child.getProperty("id").toString()==parameter->paramID){found=true;break;}
   if(!found){ValueTree child("PARAM");child.setProperty("id",parameter->paramID,nullptr);child.setProperty("value",parameter->convertFrom0to1(parameter->getDefaultValue()),nullptr);restored.addChild(child,-1,nullptr);}
  }state.replaceState(restored);modified.store(bool(restored.getProperty("modified",false)));}}
private:
 void parameterChanged(const String&,float) override {modified.store(true);}
 // The single LFO shared by all voices: free-running, one value per sample of the current block.
 void renderLfo(int count) {
  if(int(modulation.lfo.size())<count) modulation.lfo.resize(size_t(count));
  const int shape=int(state.getRawParameterValue("lfoShape")->load());const double step=state.getRawParameterValue("rate")->load()/sampleRate;
  for(int i=0;i<count;++i) {
   const float t=float(lfoPhase);float v=t<.25f?4*t:(t<.75f?2-4*t:4*t-4);
   if(shape==1)v=std::sin(t*MathConstants<float>::twoPi);else if(shape==2)v=2*t-1;else if(shape==3)v=t<.5f?1.f:-1.f;
   modulation.lfo[size_t(i)]=v;lfoPhase+=step;if(lfoPhase>=1)lfoPhase-=1;
  }
 }
 ModulationBus modulation;double lfoPhase=0;
 std::atomic<bool> modified{false};
 std::atomic<int> currentProgram{0};
 GlideMemory previousClassic,previousJupiter;double sampleRate=44100;Arpeggiator arp;CharacterDelay echo;OrbitReverb reverb;NoteHold hold;
 VoiceAllocator synth,classic;EnsembleChorus chorus;SmoothedValue<float> gain;int lastMode=-1;
};
#include "Editor.h"
AudioProcessorEditor* Orbit::createEditor(){return new Editor(*this);}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Orbit();}
