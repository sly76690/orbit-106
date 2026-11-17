#pragma once
// PHASER (ported from Octant): transparent when off, heard when on.
static bool phaserTests(){
 using namespace juce;
 auto set=[](AudioProcessor& p,const char* id,float value){auto* parameter=dynamic_cast<RangedAudioParameter*>(parameterOf(p,id));parameter->setValueNotifyingHost(parameter->convertTo0to1(value));};
 auto run=[&](bool on,float mix){std::unique_ptr<AudioProcessor> p(createPluginFilter());p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
  set(*p,"reverbOn",0);set(*p,"delayOn",0);set(*p,"chorusMode",0);set(*p,"cutoff",8000);set(*p,"phaserOn",on?1.f:0.f);set(*p,"phaserMix",mix);
  AudioBuffer<float> all(2,48128);all.clear();
  for(int done=0;done<48128;done+=256){AudioBuffer<float> piece(all.getArrayOfWritePointers(),2,done,256);MidiBuffer m;if(done==0)m.addEvent(MidiMessage::noteOn(1,48,.8f),0);p->processBlock(piece,m);}
  return all;};
 const auto off=run(false,1),mixZero=run(true,0),on=run(true,1);
 float same=0,diff=0;for(int i=4800;i<48000;++i){same=jmax(same,std::abs(off.getSample(0,i)-mixZero.getSample(0,i)));diff+=std::abs(off.getSample(0,i)-on.getSample(0,i));}
 if(same>1e-4f||diff/43200.f<.005f){std::cerr<<"PHASER: off/mix 0 differ "<<same<<", on change "<<diff/43200.f<<"\n";return false;}
 std::cout<<"PASS phaser: transparent off or at mix 0, heard when on\n";return true;
}
