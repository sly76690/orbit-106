#pragma once
// ORBIT 106 SEQUENCER: recorded chords and rests, transposition, recording through the plugin, overdub, live
// recording on a blank loop, and the step editor's lanes.
static bool sequencerTests(){
 auto require=[](bool ok,const juce::String& what){if(!ok)std::cerr<<"SEQUENCER: "<<what<<"\n";return ok;};
 // SEQUENCER playback: recorded chords and rests, moved by the lowest held key (relative to the first step's lowest note).
 {Arpeggiator::Sequence seq;const int a[]={60},b[]={64,67},c[]={62};seq.add(a,1);seq.add(b,2);seq.add(nullptr,0);seq.add(c,1);
  if(!require(seq.root==60&&seq.length==4&&seq.toString()=="60;64,67;r;62","sequence text form"))return false;
  {Arpeggiator::Sequence lanes;lanes.fromString("60,67o;64");const int g[]={55};lanes.setLane(0,g,1,true);
   if(!require(lanes.toString()=="55o,60;64","OVD line not replaced on its own: "+lanes.toString()))return false;
   const int h[]={62};lanes.setLane(0,h,1,false);if(!require(lanes.toString()=="55o,62;64"&&lanes.root==62,"SEQ line not replaced on its own: "+lanes.toString()))return false;
   auto merged=lanes;merged.mergeOverdub();lanes.clearOverdub();
   if(!require(lanes.toString()=="62;64"&&merged.toString()=="55,62;64","clear / merge overdub"))return false;}
  Arpeggiator::Sequence back;back.fromString(seq.toString());if(!require(back.toString()==seq.toString()&&back.root==60,"sequence does not read back"))return false;
  Arpeggiator arp;arp.prepare(48000);std::vector<std::vector<int>> steps;int last=-1;
  for(int blk=0;blk<200;++blk){juce::MidiBuffer m;if(blk==0)m.addEvent(juce::MidiMessage::noteOn(1,62,.8f),0);
   arp.process(m,512,true,120,2,4,1,.5f,-1.,&seq);
   for(const auto meta:m){const auto msg=meta.getMessage();if(!msg.isNoteOn())continue;const int at=blk*512+meta.samplePosition;if(at!=last){steps.push_back({});last=at;}steps.back().push_back(msg.getNoteNumber());}}
  const std::vector<std::vector<int>> expected{{62},{66,69},{64},{62},{66,69},{64}}; // the rest gives no note
  if(!require(steps.size()>=expected.size(),"sequencer plays too few steps"))return false;
  for(size_t k=0;k<expected.size();++k)if(!require(steps[k]==expected[k],"sequencer chords, rest or transposition wrong"))return false;
  // No key held: PLAY alone runs the sequence as recorded.
  Arpeggiator free;free.prepare(48000);std::vector<int> first;
  for(int blk=0;blk<40&&first.size()<2;++blk){juce::MidiBuffer m;free.process(m,512,true,120,2,4,1,.5f,-1.,&seq);for(const auto meta:m)if(meta.getMessage().isNoteOn())first.push_back(meta.getMessage().getNoteNumber());}
  if(!require(first.size()>=2&&first[0]==60&&first[1]==64,"sequencer does not run on its own"))return false;}
 // SEQUENCER recording through the plugin: REC, a note, a chord, REST, a note; the sequence is saved with the patch.
 {std::unique_ptr<juce::AudioProcessor> q(createPluginFilter());q->setRateAndBufferSizeDetails(48000,256);q->prepareToPlay(48000,256);
  auto set=[&](const char* id,float v){for(auto* b:q->getParameters())if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(b))if(r->paramID==id)r->setValueNotifyingHost(v);};
  auto block=[&](std::initializer_list<juce::MidiMessage> msgs){juce::AudioBuffer<float> b(2,256);b.clear();juce::MidiBuffer m;int at=0;for(auto& msg:msgs)m.addEvent(msg,at++);q->processBlock(b,m);};
  set("seqRec",1);block({});
  block({juce::MidiMessage::noteOn(1,60,.8f)});block({juce::MidiMessage::noteOff(1,60)});
  block({juce::MidiMessage::noteOn(1,64,.8f),juce::MidiMessage::noteOn(1,67,.8f)});block({juce::MidiMessage::noteOff(1,67)});block({juce::MidiMessage::noteOff(1,64)});
  set("seqRest",1);block({});set("seqRest",0);block({});
  block({juce::MidiMessage::noteOn(1,62,.8f)});block({juce::MidiMessage::noteOff(1,62)});set("seqRec",0);block({});
  juce::MemoryBlock data;q->getStateInformation(data);auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
  if(!require(xml&&xml->getStringAttribute("sequence")=="60;64,67;r;62","sequencer recording wrong: "+(xml?xml->getStringAttribute("sequence"):juce::String())))return false;
  std::unique_ptr<juce::AudioProcessor> r(createPluginFilter());r->setStateInformation(data.getData(),int(data.getSize()));juce::MemoryBlock again;r->getStateInformation(again);
  auto xml2=juce::AudioProcessor::getXmlFromBinary(again.getData(),int(again.getSize()));if(!require(xml2&&xml2->getStringAttribute("sequence")=="60;64,67;r;62","sequence not restored with the session"))return false;
  // OVERDUB: REC while PLAY runs adds the key to the step being played, without clearing the sequence.
  set("seqOvd",1);for(int i=0;i<20;++i)block({});block({juce::MidiMessage::noteOn(1,50,.8f)});block({juce::MidiMessage::noteOff(1,50)});set("seqOvd",0);block({});
  juce::MemoryBlock dub;q->getStateInformation(dub);auto xml3=juce::AudioProcessor::getXmlFromBinary(dub.getData(),int(dub.getSize()));
  const auto text=xml3?xml3->getStringAttribute("sequence"):juce::String();
  if(!require(text.contains("50o")&&text.contains("64,67")&&juce::StringArray::fromTokens(text,";","").size()==4,"overdub wrong: "+text))return false;}
 // LIVE RECORDING: REC then PLAY on an empty sequence: a blank 16-step loop, each key on the nearest step.
 {std::unique_ptr<juce::AudioProcessor> q(createPluginFilter());q->setRateAndBufferSizeDetails(48000,256);q->prepareToPlay(48000,256);
  auto set=[&](const char* id,float v){for(auto* b:q->getParameters())if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(b))if(r->paramID==id)r->setValueNotifyingHost(r->convertTo0to1(v));};
  int blocks=0;auto block=[&](std::initializer_list<juce::MidiMessage> msgs){juce::AudioBuffer<float> b(2,256);b.clear();juce::MidiBuffer m;for(auto& msg:msgs)m.addEvent(msg,0);q->processBlock(b,m);++blocks;};
  set("arpSync",0);set("arpTempo",120);set("arpDivision",2);set("seqRec",1);block({});set("seqPlay",1);blocks=0; // a step: 6000 samples
  while(blocks<14)block({});block({juce::MidiMessage::noteOn(1,60,.8f)});block({juce::MidiMessage::noteOff(1,60)}); // 3584: late, step 2
  while(blocks<95)block({});block({juce::MidiMessage::noteOn(1,62,.8f)});block({juce::MidiMessage::noteOff(1,62)});  // 24320: early, step 5
  set("seqRec",0);set("seqPlay",0);block({});
  juce::MemoryBlock data;q->getStateInformation(data);auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
  const auto steps=juce::StringArray::fromTokens(xml?xml->getStringAttribute("sequence"):juce::String(),";","");
  if(!require(steps.size()==16&&steps[1]=="60"&&steps[4]=="62"&&steps[0]=="r","live recording wrong: "+steps.joinIntoString(";")))return false;}
 std::cout<<"PASS sequencer: steps, rests, transposition, recording, overdub, live recording, lanes\n";
 return true;
}
