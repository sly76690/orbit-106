#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <cmath>
#include "../Source/Arpeggiator.h"
#include "../Source/PresetStorage.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main() {
 juce::ScopedJuceInitialiser_GUI gui;
 for(int blockSize:{17,256,1024}) {
  Arpeggiator arp;arp.prepare(48000);std::vector<std::pair<int,int>> notes;
  for(int start=0;start<25000;start+=blockSize){juce::MidiBuffer midi;int length=std::min(blockSize,25000-start);
   if(start==0)for(int note:{60,64,67})midi.addEvent(juce::MidiMessage::noteOn(1,note,.8f),0);
   if(start<=24000 && start+length>24000)for(int note:{60,64,67})midi.addEvent(juce::MidiMessage::noteOff(1,note),24000-start);
   arp.process(midi,length,true,120,2,0,1,.5f);
   for(const auto event:midi)if(event.getMessage().isNoteOn())notes.emplace_back(start+event.samplePosition,event.getMessage().getNoteNumber());
  }
  const std::vector<std::pair<int,int>> expected{{0,60},{6000,64},{12000,67},{18000,60}};
  if(notes!=expected){std::cerr<<"Arpeggiator timing mismatch\n";return 11;}
 }
 std::cout<<"PASS arpeggiator sample timing across block sizes and note release\n";

 auto tailEnergy=[](const char* effect){
  std::unique_ptr<juce::AudioProcessor> instance(createPluginFilter());
  auto set=[&](const char* key,float value){for(auto* base:instance->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID==key)param->setValueNotifyingHost(param->convertTo0to1(value));};
  set("chorus",0);set("attack",.002f);set("release",.01f);set("delayTime",.3f);set("reverbSize",.8f);
  if(effect)set(effect,.6f);
  instance->setRateAndBufferSizeDetails(48000,256);instance->prepareToPlay(48000,256);
  double energy=0;juce::AudioBuffer<float> block(2,256);
  for(int n=0;n<180;++n){juce::MidiBuffer midi;if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(n==4)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);instance->processBlock(block,midi);if(n>50)for(int i=0;i<256;++i){float v=block.getSample(0,i);if(!std::isfinite(v))return -1.;energy+=v*v;}}
  return energy;
 };
 auto hybridRender=[](const char* id,float value,int engine){std::unique_ptr<juce::AudioProcessor> instance(createPluginFilter());instance->setCurrentProgram(engine==0?0:4);auto set=[&](const char* key,float v){for(auto* base:instance->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID==key)param->setValueNotifyingHost(param->convertTo0to1(v));};set("chorus",0);set("envLink",0);set("cutoff",500);set(id,value);instance->setRateAndBufferSizeDetails(48000,256);instance->prepareToPlay(48000,256);std::vector<float> audio;juce::AudioBuffer<float> b(2,256);for(int n=0;n<100;++n){juce::MidiBuffer midi;if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);instance->processBlock(b,midi);audio.insert(audio.end(),b.getReadPointer(0),b.getReadPointer(0)+256);}return audio;};
 for(auto test:std::vector<std::pair<const char*,int>>{{"slope",0},{"fattack",0},{"oscLayer",0},{"sub",1}}){auto a=hybridRender(test.first,0,test.second),b=hybridRender(test.first,1,test.second);double difference=0;for(size_t i=0;i<a.size();++i){if(!std::isfinite(a[i])||!std::isfinite(b[i]))return 14;difference+=std::abs(a[i]-b[i]);}if(difference<.001)return 15;}
 std::cout<<"PASS hybrid routing: 106 slope/envelope/layer, Jupiter sub\n";
 auto glideRender=[](float seconds){std::unique_ptr<juce::AudioProcessor> instance(createPluginFilter());for(auto* base:instance->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID=="portamento")param->setValueNotifyingHost(param->convertTo0to1(seconds));instance->setRateAndBufferSizeDetails(48000,256);instance->prepareToPlay(48000,256);std::vector<float> result;juce::AudioBuffer<float> b(2,256);
  for(int n=0;n<40;++n){juce::MidiBuffer midi;if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(n==12){midi.addEvent(juce::MidiMessage::noteOff(1,60),0);midi.addEvent(juce::MidiMessage::noteOn(1,72,.8f),0);}instance->processBlock(b,midi);result.insert(result.end(),b.getReadPointer(0),b.getReadPointer(0)+256);}return result;};
 auto immediate=glideRender(0),gliding=glideRender(.5f);double difference=0;for(size_t i=12*256;i<immediate.size();++i)difference+=std::abs(immediate[i]-gliding[i]);if(difference<.01)return 13;
 std::cout<<"PASS portamento changes note transition audio\n";
 const double dryTail=tailEnergy(nullptr),delayTail=tailEnergy("delayMix"),reverbTail=tailEnergy("reverbMix");
 if(dryTail<0 || delayTail<=dryTail+.00001 || reverbTail<=dryTail+.00001)return 12;
 std::cout<<"PASS audible delay/reverb tails over dry baseline\n";
 // Exercise the actual processor with overlapping notes, two channels, sustain and CC1.
 auto extendedRender=[](std::initializer_list<std::pair<const char*,float>> settings,int blockSize=256,bool wheel=false){
  std::unique_ptr<juce::AudioProcessor> synth(createPluginFilter());
  auto set=[&](const char* id,float v){for(auto* base:synth->getParameters())if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(base))if(p->paramID==id)p->setValueNotifyingHost(p->convertTo0to1(v));};
  set("chorus",0);set("release",.01f);set("attack",.12f);set("oscLayer",.8f);set("cutoff",900);set("cutoffB",350);set("env",0);set("envB",0);
  for(auto pair:settings)set(pair.first,pair.second);
  synth->setRateAndBufferSizeDetails(48000,blockSize);synth->prepareToPlay(48000,blockSize);
  juce::MidiBuffer score;score.addEvent(juce::MidiMessage::controllerEvent(1,1,wheel?127:0),0);
  score.addEvent(juce::MidiMessage::noteOn(1,60,.8f),13);score.addEvent(juce::MidiMessage::noteOn(2,67,.7f),3001);
  score.addEvent(juce::MidiMessage::noteOff(2,67),6003);score.addEvent(juce::MidiMessage::controllerEvent(1,64,127),7000);
  score.addEvent(juce::MidiMessage::noteOff(1,60),8001);score.addEvent(juce::MidiMessage::controllerEvent(1,64,0),10003);
  score.addEvent(juce::MidiMessage::noteOn(1,64,.8f),14011);score.addEvent(juce::MidiMessage::noteOn(1,64,.8f),16017);score.addEvent(juce::MidiMessage::noteOff(1,64),18001);
  std::vector<float> audio;for(int start=0;start<24000;start+=blockSize){int n=std::min(blockSize,24000-start);juce::AudioBuffer<float> block(2,n);juce::MidiBuffer midi;midi.addEvents(score,start,n,-start);synth->processBlock(block,midi);for(int i=0;i<n;++i)audio.push_back(block.getSample(0,i));}return audio;
 };
 auto diff=[](const std::vector<float>& a,const std::vector<float>& b){double sum=0;for(size_t i=0;i<a.size();++i){if(!std::isfinite(a[i])||!std::isfinite(b[i]))return -1.;sum+=std::abs(a[i]-b[i]);}return sum;};
 for(const auto id:{"osc1Shape","osc2Shape","osc1Octave","osc2Octave","osc2Tune","lfoPitch","lfoFilter","envPitch","filterRoute","filterAType"}){
  auto a=extendedRender({{id,0}}),b=extendedRender({{id,1}});if(diff(a,b)<.01){std::cerr<<"Inactive parameter "<<id<<"\n";return 22;}
 }
 if(diff(extendedRender({},256,false),extendedRender({},256,true))<.01)return 23;
 for(int voiceMode:{1,2})for(int legato:{0,1}){
  auto a=extendedRender({{"voiceMode",float(voiceMode)},{"legato",float(legato)}},17),b=extendedRender({{"voiceMode",float(voiceMode)},{"legato",float(legato)}},256);
  if(diff(a,b)>.02 || diff(a,b)<0){std::cerr<<"Voice timing depends on block size\n";return 24;}
  double held=0,tail=0;for(int i=9000;i<10000;++i)held+=std::abs(a[size_t(i)]);for(int i=22000;i<24000;++i)tail+=std::abs(a[size_t(i)]);
  if(held<.01 || tail>.001){std::cerr<<"Sustain or release failed\n";return 25;}
 }
 for(int playMode:{1,2}){
  auto audio=extendedRender({{"voiceMode",float(playMode)},{"legato",1},{"osc1Shape",4},{"oscLayer",0},{"sub",0},{"cutoff",18000},{"attack",.002f},{"sustain",1},{"unisonSpread",0},{"pwm",0}});
  auto frequency=[&](int start,int end){std::vector<int> crossings;for(int i=start+1;i<end;++i)if(audio[size_t(i-1)]<=0&&audio[size_t(i)]>0)crossings.push_back(i);if(crossings.size()<2)return 0.;return 48000.*(crossings.size()-1)/(crossings.back()-crossings.front());};
  if(std::abs(frequency(3600,5900)-391.995)>3 || std::abs(frequency(6500,7900)-261.626)>3){std::cerr<<"Last-note priority/fallback pitch failed\n";return 30;}
 }
 if(diff(extendedRender({{"voiceMode",1},{"legato",0}}),extendedRender({{"voiceMode",1},{"legato",1}}))<.01)return 26;
 for(int route=0;route<5;++route)for(int modelA=0;modelA<2;++modelA)for(int modelB=0;modelB<2;++modelB){auto audio=extendedRender({{"filterRoute",float(route)},{"filterAType",float(modelA)},{"filterBType",float(modelB)},{"resonance",.95f},{"resonanceB",.95f}});for(float v:audio)if(!std::isfinite(v)||std::abs(v)>1.001f)return 27;}
 std::cout<<"PASS independent oscillators, pitch/filter modulation, CC1, mono/unison/legato, sustain, release and all filter routes/models\n";
 std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
 for(double sr:{44100.0,48000.0,96000.0}) {
  p->setRateAndBufferSizeDetails(sr,256);p->prepareToPlay(sr,256);
  for(int mode=0;mode<2;++mode) {
   p->getParameters()[0]->setValueNotifyingHost(float(mode));
   juce::AudioBuffer<float> b(2,256);juce::MidiBuffer m;
   m.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),17);double energy=0;
   for(int block=0;block<100;++block) {
    p->processBlock(b,m);m.clear();
    for(int ch=0;ch<2;++ch) for(int i=0;i<256;++i) {float v=b.getSample(ch,i);if(!std::isfinite(v)||std::abs(v)>1.001f) return 1;energy+=v*v;}
   }
   if(energy<0.001) return 2;
   m.addEvent(juce::MidiMessage::noteOff(1,60),0);
   for(int block=0;block<2000;++block) {p->processBlock(b,m);m.clear();}
   if(b.getMagnitude(0,256)>0.0001f) return 3;
   std::cout<<"PASS audio + release: "<<sr<<" Hz, mode "<<mode<<"\n";
  }
  p->releaseResources();
 }
 p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
 for(auto* parameter:p->getParameters()) parameter->setValueNotifyingHost(1.f);
 for(int mode=0;mode<2;++mode) {
  p->getParameters()[0]->setValueNotifyingHost(float(mode));
  for(int size:{1,17,256}) {
   juce::AudioBuffer<float> b(2,size);juce::MidiBuffer midi;
   for(int note=24;note<108;note+=3) midi.addEvent(juce::MidiMessage::noteOn(1,note,1.f),0);
   for(int n=0;n<40;++n) {
    p->processBlock(b,midi);midi.clear();
    for(int ch=0;ch<2;++ch) for(int i=0;i<size;++i) if(!std::isfinite(b.getSample(ch,i)) || std::abs(b.getSample(ch,i))>1.001f) return 6;
   }
  }
 }
 p->releaseResources();
 std::cout<<"PASS extreme parameters, voice stealing and short blocks\n";
 juce::MemoryBlock state;p->getStateInformation(state);
 p->getParameters()[0]->setValueNotifyingHost(0);p->setStateInformation(state.getData(),int(state.getSize()));
 if(p->getParameters()[0]->getValue()!=1.f) return 4;
 std::cout<<"PASS state restore\n";
 p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
 for(int preset=0;preset<p->getNumPrograms();++preset) {
  p->setCurrentProgram(preset);
  juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;double energy=0;
  midi.addEvent(juce::MidiMessage::noteOn(1,60,0.8f),0);
  for(int n=0;n<200;++n) {
   p->processBlock(b,midi);midi.clear();
   for(int i=0;i<256;++i) {float sample=b.getSample(0,i);if(!std::isfinite(sample)) return 7;energy+=sample*sample;}
  }
  if(energy<.0001 || p->getCurrentProgram()!=preset || p->getProgramName(preset).isEmpty()) return 8;
  juce::MemoryBlock patch;p->getStateInformation(patch);p->setCurrentProgram((preset+1)%p->getNumPrograms());p->setStateInformation(patch.getData(),int(patch.getSize()));
  if(p->getCurrentProgram()!=preset) return 9;
  p->releaseResources();p->prepareToPlay(48000,256);
 }
 std::cout<<"PASS 24 presets: audio and program recall\n";
 const auto testRoot=juce::File::getCurrentWorkingDirectory().getChildFile("preset-tests-"+juce::Uuid().toString());
 PresetStorage storage(testRoot);
 p->setCurrentProgram(19);juce::MemoryBlock original;p->getStateInformation(original);auto xml=juce::AudioProcessor::getXmlFromBinary(original.getData(),int(original.getSize()));if(!xml)return 16;
 if(storage.save("Hybrid Test",*xml).failed())return 17;
 if(storage.save("Hybrid Test",*xml).wasOk()||storage.save("../escape",*xml).wasOk()||storage.load("../escape"))return 18;
 auto loaded=storage.load("Hybrid Test");if(!loaded || loaded->toString()!=xml->toString() || !storage.names().contains("Hybrid Test"))return 19;
 std::vector<float> expectedValues;for(auto* param:p->getParameters())expectedValues.push_back(param->getValue());p->setCurrentProgram(0);juce::MemoryBlock restored;juce::AudioProcessor::copyXmlToBinary(*loaded,restored);p->setStateInformation(restored.getData(),int(restored.getSize()));
 for(int i=0;i<p->getParameters().size();++i)if(std::abs(p->getParameters()[i]->getValue()-expectedValues[size_t(i)])>.00001f)return 20;
 if(!testRoot.deleteRecursively())return 21;
 std::cout<<"PASS disk preset round trip, all parameters, duplicate protection and path rejection\n";

 // Old sessions omit newly appended parameters: restoration must use defaults,
 // not retain edits from the previously loaded sound.
 p->setCurrentProgram(0);juce::MemoryBlock legacy;p->getStateInformation(legacy);auto oldXml=juce::AudioProcessor::getXmlFromBinary(legacy.getData(),int(legacy.getSize()));
 auto tree=juce::ValueTree::fromXml(*oldXml);for(int i=tree.getNumChildren()-1;i>=0;--i)if(tree.getChild(i).getProperty("id").toString()=="filterRoute")tree.removeChild(i,nullptr);
 for(auto* base:p->getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base))if(parameter->paramID=="filterRoute")parameter->setValueNotifyingHost(1);
 auto legacyXml=tree.createXml();juce::AudioProcessor::copyXmlToBinary(*legacyXml,legacy);p->setStateInformation(legacy.getData(),int(legacy.getSize()));
 for(auto* base:p->getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base))if(parameter->paramID=="filterRoute"&&parameter->getValue()!=parameter->getDefaultValue())return 28;
 std::cout<<"PASS legacy session default migration\n";
 p->setCurrentProgram(1);

 {
  auto chorusRender=[](float mode){
   std::unique_ptr<juce::AudioProcessor> synth(createPluginFilter());
   auto set=[&](const char* id,float v){for(auto* base:synth->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID==id)param->setValueNotifyingHost(param->convertTo0to1(v));};
   set("chorus",1);set("chorusMode",mode);set("reverbMix",0);set("delayMix",0);
   synth->setRateAndBufferSizeDetails(48000,256);synth->prepareToPlay(48000,256);
   std::vector<float> sides;juce::AudioBuffer<float> block(2,256);
   for(int n=0;n<400;++n){juce::MidiBuffer midi;if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,57,.8f),0);synth->processBlock(block,midi);for(int i=0;i<256;++i){if(!std::isfinite(block.getSample(0,i))||!std::isfinite(block.getSample(1,i)))return std::vector<float>();sides.push_back(block.getSample(0,i)-block.getSample(1,i));}}
   return sides;
  };
  auto energy=[](const std::vector<float>& v){double sum=0;for(float x:v)sum+=double(x)*x;return sum;};
  const auto offSides=chorusRender(0),oneSides=chorusRender(1),twoSides=chorusRender(2);
  if(offSides.empty()||oneSides.empty()||twoSides.empty()){std::cerr<<"Chorus produced non-finite audio\n";return 40;}
  if(energy(offSides)>1e-9){std::cerr<<"Chorus OFF is not transparent in stereo\n";return 41;}
  if(energy(oneSides)<1e-3||energy(twoSides)<1e-3){std::cerr<<"Chorus I / II do not widen the signal\n";return 42;}
  double modeDifference=0;for(size_t i=0;i<oneSides.size();++i)modeDifference+=std::abs(oneSides[i]-twoSides[i]);
  if(modeDifference<.01){std::cerr<<"Chorus I and II sound the same\n";return 43;}
  // A state saved before the OFF position existed stored I as 0 and II as 1.
  std::unique_ptr<juce::AudioProcessor> old(createPluginFilter());juce::MemoryBlock data;old->getStateInformation(data);
  auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));auto tree=juce::ValueTree::fromXml(*xml);tree.removeProperty("stateVersion",nullptr);
  for(auto child:tree)if(child.getProperty("id").toString()=="chorusMode")child.setProperty("value",1.f,nullptr);
  auto oldXml=tree.createXml();juce::AudioProcessor::copyXmlToBinary(*oldXml,data);old->setStateInformation(data.getData(),int(data.getSize()));
  for(auto* base:old->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID=="chorusMode"&&juce::roundToInt(param->convertFrom0to1(param->getValue()))!=2){std::cerr<<"Old chorus mode II was not migrated\n";return 44;}
  std::cout<<"PASS ensemble chorus: OFF transparent, I / II stereo and distinct, old states migrated\n";
  // LFO delay must change a vibrato patch; the filter envelope must open the ladder within a few milliseconds.
  if(diff(extendedRender({{"lfoPitch",6},{"lfoDelay",0}}),extendedRender({{"lfoPitch",6},{"lfoDelay",2}}))<.01){std::cerr<<"LFO delay has no effect\n";return 45;}
  auto early=[&](float amount){auto audio=extendedRender({{"attack",.002f},{"fattack",.002f},{"envLink",0},{"cutoff",60},{"env",amount},{"filterAType",0},{"oscLayer",0},{"sub",0}});double sum=0;for(int i=13;i<13+480;++i)sum+=std::abs(audio[size_t(i)]);return sum;};
  const double closed=early(0),opened=early(1);
  if(opened<closed*3+.01){std::cerr<<"Ladder filter envelope is too slow: "<<closed<<" -> "<<opened<<"\n";return 46;}
  std::cout<<"PASS shared LFO delay and immediate ladder filter envelope\n";
  // High-pass switch: every position must sound different from the flat one, and low notes must lose level in 2 and 3.
  auto level=[&](float position){auto audio=extendedRender({{"hpf",position},{"osc1Shape",4},{"oscLayer",0},{"sub",0},{"cutoff",18000},{"attack",.002f},{"sustain",1}});double sum=0;for(int i=2000;i<6000;++i)sum+=std::abs(audio[size_t(i)]);return sum;};
  const double boosted=level(0),flat=level(1),thin=level(2),thinner=level(3);
  if(!(boosted>flat*1.02&&flat>thin*1.02&&thin>thinner*1.02)){std::cerr<<"High-pass positions out of order: "<<boosted<<" "<<flat<<" "<<thin<<" "<<thinner<<"\n";return 47;}
  std::cout<<"PASS four-position high-pass: bass boost, flat, two cuts\n";
  // Gate mode, inverted filter envelope and the three PWM sources must each change the sound; chorus I+II must be a third colour.
  if(diff(extendedRender({{"vcaMode",0}}),extendedRender({{"vcaMode",1}}))<.01){std::cerr<<"VCA gate mode has no effect\n";return 48;}
  if(diff(extendedRender({{"env",.5f},{"envInvert",0}}),extendedRender({{"env",.5f},{"envInvert",1}}))<.01){std::cerr<<"Filter envelope polarity has no effect\n";return 49;}
  if(diff(extendedRender({{"pwmSource",0}}),extendedRender({{"pwmSource",1}}))<.01||diff(extendedRender({{"pwmSource",1}}),extendedRender({{"pwmSource",2}}))<.01){std::cerr<<"PWM sources are not distinct\n";return 50;}
  {auto gatedTail=extendedRender({{"vcaMode",1},{"release",8}});double left=0;for(int i=22000;i<24000;++i)left+=std::abs(gatedTail[size_t(i)]);if(left>.001){std::cerr<<"Gate mode does not close with the key\n";return 51;}}
  const auto bothSides=chorusRender(3);double bothDifference=0;for(size_t i=0;i<bothSides.size()&&i<twoSides.size();++i)bothDifference+=std::abs(bothSides[i]-twoSides[i]);
  if(bothSides.empty()||energy(bothSides)<1e-4||bothDifference<.01){std::cerr<<"Chorus I+II is missing or identical to II\n";return 52;}
  std::cout<<"PASS VCA gate, filter envelope polarity, PWM sources and chorus I+II\n";
  // Hard sync and cross modulation must change the sound; a patch change must keep the output level.
  if(diff(extendedRender({{"osc2Tune",7},{"oscSync",0}}),extendedRender({{"osc2Tune",7},{"oscSync",1}}))<.01){std::cerr<<"Oscillator sync has no effect\n";return 53;}
  if(diff(extendedRender({{"crossMod",0}}),extendedRender({{"crossMod",1}}))<.01){std::cerr<<"Cross modulation has no effect\n";return 54;}
  {std::unique_ptr<juce::AudioProcessor> keeper(createPluginFilter());juce::RangedAudioParameter* level=nullptr;for(auto* base:keeper->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID=="gain")level=param;
   if(level==nullptr)return 55;level->setValueNotifyingHost(.25f);keeper->setCurrentProgram(5);if(std::abs(level->getValue()-.25f)>.001f){std::cerr<<"Patch change reset the output level\n";return 55;}}
  // HOLD keeps a released note sounding, and switching it off lets it stop.
  {std::unique_ptr<juce::AudioProcessor> holder(createPluginFilter());auto set=[&](const char* id,float v){for(auto* base:holder->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID==id)param->setValueNotifyingHost(param->convertTo0to1(v));};
   set("chorus",0);set("release",.01f);set("hold",1);holder->setRateAndBufferSizeDetails(48000,256);holder->prepareToPlay(48000,256);juce::AudioBuffer<float> block(2,256);
   auto run=[&](int blocks,int event){double sum=0;for(int n=0;n<blocks;++n){juce::MidiBuffer midi;if(n==0&&event==1)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(n==0&&event==2)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);holder->processBlock(block,midi);if(n>=blocks-10)sum+=block.getMagnitude(0,256);}return sum;};
   run(40,1);const double heldLevel=run(60,2);set("hold",0);const double releasedLevel=run(60,0);
   if(heldLevel<.01||releasedLevel>.0001){std::cerr<<"Hold failed: "<<heldLevel<<" then "<<releasedLevel<<"\n";return 56;}}
  // Both reverbs: a tail after the note, finite and bounded, and the two types must differ.
  {auto reverbTail=[](float type){std::unique_ptr<juce::AudioProcessor> room(createPluginFilter());auto set=[&](const char* id,float v){for(auto* base:room->getParameters())if(auto* param=dynamic_cast<juce::RangedAudioParameter*>(base))if(param->paramID==id)param->setValueNotifyingHost(param->convertTo0to1(v));};
    set("chorus",0);set("release",.01f);set("reverbMix",.6f);set("reverbSize",1);set("reverbType",type);room->setRateAndBufferSizeDetails(48000,256);room->prepareToPlay(48000,256);
    std::vector<float> tail;juce::AudioBuffer<float> block(2,256);for(int n=0;n<600;++n){juce::MidiBuffer midi;if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(n==8)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);room->processBlock(block,midi);if(n>=60)for(int i=0;i<256;++i){const float v=block.getSample(0,i);if(!std::isfinite(v)||std::abs(v)>1.001f)return std::vector<float>();tail.push_back(v);}}return tail;};
   const auto hall=reverbTail(0),plate=reverbTail(1);if(hall.empty()||plate.empty()){std::cerr<<"Reverb output is not finite or bounded\n";return 57;}
   if(energy(hall)<1e-6||energy(plate)<1e-6){std::cerr<<"A reverb has no tail: "<<energy(hall)<<" "<<energy(plate)<<"\n";return 58;}
   double typeDifference=0;for(size_t i=0;i<hall.size();++i)typeDifference+=std::abs(hall[i]-plate[i]);if(typeDifference<.01){std::cerr<<"The two reverbs are identical\n";return 59;}}
  if(diff(extendedRender({{"delayMix",.6f},{"delayTime",.05f},{"delayMod",0}}),extendedRender({{"delayMix",.6f},{"delayTime",.05f},{"delayMod",1}}))<.01){std::cerr<<"Delay modulation has no effect\n";return 60;}
  if(diff(extendedRender({{"delayMix",.6f},{"delaySync",0}}),extendedRender({{"delayMix",.6f},{"delaySync",9}}))<.01||diff(extendedRender({{"delayMix",.6f},{"delaySync",9}}),extendedRender({{"delayMix",.6f},{"delaySync",10}}))<.01){std::cerr<<"Delay tempo sync has no effect\n";return 62;}
  for(float type:{0.f,1.f})if(diff(extendedRender({{"reverbMix",.6f},{"reverbType",type},{"reverbColor",0}}),extendedRender({{"reverbMix",.6f},{"reverbType",type},{"reverbColor",1}}))<.01){std::cerr<<"Reverb colour has no effect on type "<<type<<"\n";return 63;}
  {auto loud=extendedRender({{"delayMix",.65f},{"delayTime",.05f},{"delayFeedback",1.1f}});for(float v:loud)if(!std::isfinite(v)||std::abs(v)>1.001f){std::cerr<<"Delay self-oscillation is not bounded\n";return 61;}}
  std::cout<<"PASS oscillator sync, cross modulation, patch change keeps level, hold, vintage and plate reverbs, delay modulation and feedback\n";
 }
 std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
 auto image=editor->createComponentSnapshot(editor->getLocalBounds());
 juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile("interface.png");
 juce::FileOutputStream stream(file);stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;
 if(!png.writeImageToStream(image,stream)) return 5;
 stream.flush();
 editor.reset();p->setCurrentProgram(13);editor.reset(p->createEditor());
 auto jp=editor->createComponentSnapshot(editor->getLocalBounds());
 juce::FileOutputStream jpStream(juce::File::getCurrentWorkingDirectory().getChildFile("interface-jupiter.png"));jpStream.setPosition(0);jpStream.truncate();
 if(!png.writeImageToStream(jp,jpStream)) return 10;
 std::function<juce::TextButton*(juce::Component&,const juce::String&)> findButton;
 findButton=[&](juce::Component& parent,const juce::String& text)->juce::TextButton*{for(int i=0;i<parent.getNumChildComponents();++i){auto* child=parent.getChildComponent(i);if(auto* button=dynamic_cast<juce::TextButton*>(child))if(button->getButtonText()==text)return button;if(auto* nested=findButton(*child,text))return nested;}return nullptr;};
 std::cout<<"PASS editor render in both voice modes\n";
}
