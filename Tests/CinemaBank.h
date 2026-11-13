#pragma once
#include "../Source/CinematicBank.h"

static bool cinemaBankTests(){
 const auto names=CinematicBank::names();
 if(names.size()!=16){std::cerr<<"Midnight Signals: expected 16 sounds\n";return false;}
 const auto folder=juce::File::getCurrentWorkingDirectory().getChildFile("MidnightSignals-preview");
 if(folder.createDirectory().failed())return false;
 constexpr int rate=48000,seconds=24,previewSeconds=12,demoSeconds=8;
 juce::AudioBuffer<float> demo(2,rate*demoSeconds*names.size());demo.clear();
 juce::String report="# Midnight Signals - audio validation\n\n48 kHz, stereo. Active RMS measured over the first 6 s; tail over the final second of a 24 s render.\n\n| Preset | RMS dBFS | Peak dBFS | Tail dBFS |\n|---|---:|---:|---:|\n";
 auto writeWave=[](const juce::File& file,const juce::AudioBuffer<float>& audio,int length){
  auto stream=file.createOutputStream();if(!stream)return false;stream->setPosition(0);stream->truncate();
  juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),rate,2,16,{},0));
  if(!writer)return false;stream.release();return writer->writeFromAudioSampleBuffer(audio,0,length);
 };
 std::array<int,4> categories{};
 for(int patch=0;patch<names.size();++patch){
  const auto& name=names[patch];const bool pad=name.startsWith("PAD"),bass=name.startsWith("BASS"),brass=name.startsWith("BRASS");++categories[pad?0:bass?1:brass?2:3];
  std::unique_ptr<juce::AudioProcessor> processor(createPluginFilter());
  auto xml=CinematicBank::load(name);if(!xml||!xml->hasTagName("OrbitState"))return false;
  for(auto* child:xml->getChildIterator()){
   auto* param=dynamic_cast<juce::RangedAudioParameter*>(parameterOf(*processor,child->getStringAttribute("id").toRawUTF8()));
   const float value=float(child->getDoubleAttribute("value"));
   if(!param||!std::isfinite(value)||value<param->getNormalisableRange().start||value>param->getNormalisableRange().end){std::cerr<<"Invalid preset parameter in "<<name<<"\n";return false;}
  }
  // Select through the real preset menu, not by feeding XML directly to the processor.
  std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
  std::function<bool(juce::Component&)> select=[&](juce::Component& component){
   if(auto* combo=dynamic_cast<juce::ComboBox*>(&component))for(int i=0;i<combo->getNumItems();++i)if(combo->getItemText(i)==name){combo->setSelectedId(combo->getItemId(i),juce::sendNotificationSync);return true;}
   for(int i=0;i<component.getNumChildComponents();++i)if(select(*component.getChildComponent(i)))return true;return false;
  };
  if(!select(*editor)){std::cerr<<"Missing menu preset: "<<name<<"\n";return false;}
  juce::MemoryBlock saved;processor->getStateInformation(saved);
  auto recalled=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));
  if(recalled->getStringAttribute("soundName")!=name)return false;
  for(auto* child:xml->getChildIterator()){
   auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(parameterOf(*processor,child->getStringAttribute("id").toRawUTF8()));
   if(std::abs(parameter->convertFrom0to1(parameter->getValue())-float(child->getDoubleAttribute("value")))>.002f){std::cerr<<"Menu did not load parameter: "<<name<<"\n";return false;}
  }
  if(name=="PAD - Hawkins Afterglow"){
   editor.reset();editor.reset(processor->createEditor());
   editor->setSize(1600,1000);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
   auto stream=folder.getChildFile("Panel.png").createOutputStream();if(!stream)return false;
   stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;if(!png.writeImageToStream(image,*stream))return false;
  }
  editor.reset();
  processor->setRateAndBufferSizeDetails(rate,256);processor->prepareToPlay(rate,256);
  juce::MidiBuffer score;
  const auto note=[&](int pitch,double start,double duration,float velocity){score.addEvent(juce::MidiMessage::noteOn(1,pitch,velocity),int(start*rate));score.addEvent(juce::MidiMessage::noteOff(1,pitch),int((start+duration)*rate));};
  if(pad){for(int pitch:{48,55,60,63})note(pitch,0,6,.8f);}
  else if(brass){for(int pitch:{48,55,60,63})note(pitch,0,1.1,.8f);for(int pitch:{46,53,58,62})note(pitch,1.8,1.1,.9f);for(int pitch:{48,55,60,63})note(pitch,3.6,2,.75f);}
  else {const int bassNotes[]={48,48,51,55,46,48,43,46},leadNotes[]={60,63,67,70,67,65,63,60};for(int n=0;n<8;++n)note(bass?bassNotes[n]:leadNotes[n],n*.65,bass?.42:.72,n%2?.85f:.75f);}
  juce::AudioBuffer<float> preview(2,rate*previewSeconds);preview.clear();
  double energy=0,tail=0,peak=0;long hot=0;
  for(int start=0;start<rate*seconds;start+=256){const int count=juce::jmin(256,rate*seconds-start);juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;midi.addEvents(score,start,count,-start);processor->processBlock(audio,midi);
   for(int c=0;c<2;++c)for(int i=0;i<count;++i){const float x=audio.getSample(c,i);if(!std::isfinite(x))return false;peak=std::max(peak,double(std::abs(x)));if(std::abs(x)>.95f)++hot;
    if(start+i<rate*6)energy+=x*x;if(start+i>=rate*(seconds-1))tail+=x*x;}
   if(start<preview.getNumSamples())for(int c=0;c<2;++c)preview.copyFrom(c,start,audio,c,0,juce::jmin(count,preview.getNumSamples()-start));
  }
  const double rms=std::sqrt(energy/(rate*6*2)),tailRms=std::sqrt(tail/(rate*2));
  std::cout<<"CINEMA "<<name<<": rms "<<juce::Decibels::gainToDecibels(rms)<<" peak "<<juce::Decibels::gainToDecibels(peak)<<" tail "<<juce::Decibels::gainToDecibels(tailRms)<<" dBFS\n";
  if(rms<.025||rms>.25||peak>.995||hot>rate/100||tailRms>rms*.02+.00001){std::cerr<<"Cinema level/release check failed: "<<name<<"\n";return false;}
  report+="| "+name+" | "+juce::String(juce::Decibels::gainToDecibels(rms),1)+" | "+juce::String(juce::Decibels::gainToDecibels(peak),1)+" | "+juce::String(juce::Decibels::gainToDecibels(tailRms),1)+" |\n";
  for(int c=0;c<2;++c){demo.copyFrom(c,patch*rate*demoSeconds,preview,c,0,rate*demoSeconds);demo.applyGainRamp(c,patch*rate*demoSeconds,rate/100,0,1);demo.applyGainRamp(c,(patch+1)*rate*demoSeconds-rate,rate,1,0);preview.applyGainRamp(c,rate*previewSeconds-rate/2,rate/2,1,0);}
  if(!writeWave(folder.getChildFile(name+".wav"),preview,preview.getNumSamples()))return false;
 }
 for(int n:categories)if(n!=4)return false;
 if(!writeWave(folder.getChildFile("Midnight Signals - Demo.wav"),demo,demo.getNumSamples()))return false;
 if(!folder.getChildFile("Validation.md").replaceWithText(report))return false;
 std::cout<<"PASS Midnight Signals: 16 embedded presets, real menu selection, parameter ranges, audio levels and tails\n";
 return true;
}
