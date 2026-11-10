#pragma once

// Cached painted-metal surface, recreated from the supplied photographic references.
// No photo, logo or lettering is sampled into the controls.
static Image makeSatinPanel(int width,int height) {
 Image result(Image::RGB,width,height,false);Random grain{106};
 for(int y=0;y<height;++y)for(int x=0;x<width;++x){
  const float nx=float(x)/width,ny=float(y)/height;
  const float light=7.f*std::exp(-std::pow((nx-.22f)*2.3f,2.f)-std::pow(ny*1.4f,2.f));
  const float uneven=1.1f*std::sin(nx*13.f+ny*4.f)+.6f*std::sin(nx*31.f-ny*17.f);
  const float pores=(grain.nextFloat()-.5f)*4.f;
  int shade=roundToInt(48.f+light+uneven+pores);
  result.setPixelAt(x,y,Colour(uint8(shade+5),uint8(shade+2),uint8(shade-3)));
 }
 Graphics g(result);g.setColour(Colour(0xffe8d5b1).withAlpha(.035f));
 for(int i=0;i<34;++i){float x=grain.nextFloat()*width,y=grain.nextFloat()*height;g.drawLine(x,y,x+1+grain.nextFloat()*4,y-.5f,.5f);}
 return result;
}


class PanelLook : public LookAndFeel_V4 {
public:
 Colour accent{0xff9a6842};
 PanelLook() {
  setColour(Label::textColourId,Colour(0xffdfcfad));setColour(TextButton::buttonOnColourId,Colour(0xffd5b675));setColour(TextButton::textColourOnId,Colour(0xff30271a));
  setColour(ComboBox::backgroundColourId,Colour(0xff241e18));setColour(ComboBox::textColourId,Colour(0xfff5cf83));
  setColour(ComboBox::outlineColourId,Colour(0xff77664f));setColour(PopupMenu::backgroundColourId,Colour(0xff30281f));
  setColour(PopupMenu::highlightedBackgroundColourId,Colour(0xff766043));
  setColour(TextButton::buttonColourId,Colour(0xffb8a47c));setColour(TextButton::textColourOffId,Colour(0xff29221b));
 }
 void drawButtonBackground(Graphics& g,Button& button,const Colour& colour,bool over,bool down) override {
  auto r=button.getLocalBounds().toFloat().reduced(1);auto base=colour;
  if(over) base=base.brighter(.13f);if(down)base=base.darker(.12f);
  g.setColour(Colour(0xff1c1712));g.fillRoundedRectangle(r,3);r.removeFromBottom(3);
  g.setGradientFill(ColourGradient(base.brighter(.10f),0,r.getY(),base.darker(.12f),0,r.getBottom(),false));g.fillRoundedRectangle(r.reduced(1),2);
  g.setColour(Colour(0xfff7e8c4).withAlpha(.22f));g.drawHorizontalLine(int(r.getY()+2),r.getX()+3,r.getRight()-3);
  if(button.getToggleState()){g.setColour(Colour(0xffe69530).withAlpha(.2f));g.fillEllipse(r.getRight()-14,r.getY()+1,11,11);g.setColour(Colour(0xffffb34b));g.fillEllipse(r.getRight()-11,r.getY()+4,5,5);g.setColour(Colour(0xffffe2a3));g.fillEllipse(r.getRight()-10,r.getY()+4,2,2);}

 }
 void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float value,float start,float end,Slider& slider) override {
  auto r=Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(6);const float diameter=jmin(r.getWidth(),r.getHeight());r=r.withSizeKeepingCentre(diameter,diameter);
  auto centre=r.getCentre();float angle=start+value*(end-start);
  for(int i=0;i<=10;++i){float tick=start+(end-start)*i/10.f;float outer=diameter*.49f,inner=diameter*.44f;g.setColour(Colour(0xffbaa581));g.drawLine(centre.x+std::sin(tick)*inner,centre.y-std::cos(tick)*inner,centre.x+std::sin(tick)*outer,centre.y-std::cos(tick)*outer,1.f);}
  g.setColour(Colours::black.withAlpha(.35f));g.fillEllipse(r.reduced(4).translated(1,3));
  auto dial=r.reduced(5);g.setGradientFill(ColourGradient(Colour(0xff685e50),dial.getX(),dial.getY(),Colour(0xff231f1a),dial.getRight(),dial.getBottom(),false));g.fillEllipse(dial);g.setColour(Colour(0xff17130f));g.drawEllipse(dial,1);
  const float radius=dial.getWidth()*.38f;g.setColour(Colour(0xfff0e6ca));g.drawLine(centre.x+std::sin(angle)*radius*.4f,centre.y-std::cos(angle)*radius*.4f,centre.x+std::sin(angle)*radius,centre.y-std::cos(angle)*radius,2);
  if(!slider.isEnabled()){g.setColour(Colour(0xff383026).withAlpha(.6f));g.fillEllipse(r);}
 }
 void drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float,float,Slider::SliderStyle,Slider&) override {
  const float cx=float(x)+float(w)*.5f;
  for(int i=0;i<=10;++i) {float yy=float(y)+float(h)*i/10.f;g.setColour(Colour(0xff9a8b70));g.drawLine(cx-17,yy,cx-8,yy);g.drawLine(cx+8,yy,cx+17,yy);}
  g.setColour(Colour(0xff17130f));g.fillRoundedRectangle(cx-4,float(y)-3,8,float(h)+6,3);
  g.setColour(Colour(0xffe5d4b2).withAlpha(.13f));g.drawLine(cx+4,float(y)+1,cx+4,float(y+h)-1,.7f);
  g.setColour(Colours::black.withAlpha(.48f));g.fillRoundedRectangle(cx-15,pos-7,34,29,2);
  g.setGradientFill(ColourGradient(Colour(0xffc4b38f),0,pos-13,Colour(0xff746044),0,pos+12,false));g.fillRect(cx-14,pos-13,28.f,25.f);
  Path bevel;bevel.startNewSubPath(cx-14,pos-13);bevel.lineTo(cx-10,pos-17);bevel.lineTo(cx+12,pos-17);bevel.lineTo(cx+14,pos-13);bevel.closeSubPath();g.setColour(Colour(0xffd9c7a0));g.fillPath(bevel);
  g.setColour(Colour(0xffe4d2ad));g.drawLine(cx-14,pos-13,cx+14,pos-13,1);g.drawLine(cx-14,pos-13,cx-14,pos+10,1);
  g.setColour(Colour(0xff4c3c2c));g.fillRect(cx-12,pos+7,24.f,4.f);
  g.setColour(Colour(0xfff0ddad));g.fillRect(cx-10,pos-6,20.f,3.f);
  g.setColour(Colour(0xffcbbb9b));g.setFont(FontOptions(8.f));g.drawText("10",int(cx-26),y-6,12,10,Justification::centred);g.drawText("0",int(cx-24),y+h-5,10,10,Justification::centred);

 }
};
class PianoKeyboard : public MidiKeyboardComponent {
public:
 PianoKeyboard(MidiKeyboardState& state,Orientation orientation):MidiKeyboardComponent(state,orientation) {}
 void drawWhiteNote(int note,Graphics& g,Rectangle<float> r,bool down,bool over,Colour,Colour) override {
  auto top=down?Colour(0xffdeb572):Colour(0xffb9ad93);auto bottom=down?Colour(0xfff3ce8e):Colour(0xffeee3ca);
  g.setGradientFill(ColourGradient(top,r.getX(),r.getY(),bottom,r.getX(),r.getY()+25,false));g.fillRect(r.reduced(.6f,0));
  g.setGradientFill(ColourGradient(Colour(0xfffff4d7).withAlpha(.24f),r.getX()+2,r.getY(),Colours::transparentBlack,r.getRight()-2,r.getY(),false));g.fillRect(r.reduced(1,1));
  g.setGradientFill(ColourGradient(Colours::black.withAlpha(.23f),0,r.getY(),Colours::transparentBlack,0,r.getY()+16,false));g.fillRect(r.withHeight(16));
  g.setColour(Colour(0xff8e8069));g.drawRect(r,.6f);g.setColour(Colours::white.withAlpha(.55f));g.drawLine(r.getX()+2,r.getY()+8,r.getX()+2,r.getBottom()-7);
  g.setColour(Colour(0xffc1b295));g.fillRect(r.getX()+1,r.getBottom()-5,r.getWidth()-2,4.f);
  if(over&&!down){g.setColour(Colours::white.withAlpha(.3f));g.fillRect(r);}
  if(note%12==0){g.setColour(Colour(0xff887960));g.setFont(FontOptions(10.f));g.drawText(MidiMessage::getMidiNoteName(note,true,true,4),r.removeFromBottom(22),Justification::centred);}
 }
 void drawBlackNote(int,Graphics& g,Rectangle<float> r,bool down,bool over,Colour) override {
  g.setColour(Colour(0xff17130f));g.fillRoundedRectangle(r,2);
  auto inner=r.reduced(2,1);inner.removeFromBottom(6);
  g.setGradientFill(ColourGradient(down?Colour(0xffc08445):Colour(0xff5a4c3c),inner.getX(),inner.getY(),Colour(0xff251e18),inner.getX()+inner.getWidth(),inner.getY(),false));g.fillRoundedRectangle(inner,2);
  g.setColour(Colour(0xff81705a));g.drawLine(inner.getX()+1,inner.getY()+2,inner.getX()+1,inner.getBottom()-1);
  auto reflection=inner.reduced(inner.getWidth()*.22f,0);reflection.setHeight(inner.getHeight()*.78f);
  g.setGradientFill(ColourGradient(Colour(0xfff0dfbf).withAlpha(.17f),reflection.getX(),reflection.getY(),Colours::transparentBlack,reflection.getRight(),reflection.getBottom(),false));g.fillRoundedRectangle(reflection,2);
  g.setColour(Colour(0xffe1cfaa).withAlpha(.24f));g.drawLine(inner.getX()+2,inner.getY()+2,inner.getRight()-2,inner.getY()+2,.7f);
  g.setColour(down?Colour(0xffedb66d):Colour(0xff3d3024));g.fillRect(r.getX()+2,r.getBottom()-7,r.getWidth()-4,3.f);
  if(over&&!down){g.setColour(Colours::white.withAlpha(.12f));g.fillRect(r);}
 }
};
// Detail panel keeps the performance surface and keyboard uncluttered.
class SynthEditPanel : public Component {
 struct Fader {Label label;Slider slider;int page;std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;};
 struct Selector : Component {
  Label label;OwnedArray<TextButton> buttons;std::unique_ptr<ParameterAttachment> attachment;
  Selector(RangedAudioParameter& parameter,const String& title,StringArray names,Colour colour){
   label.setText(title,dontSendNotification);label.setFont(FontOptions(10.f,Font::bold));label.setJustificationType(Justification::centred);addAndMakeVisible(label);
   for(int i=0;i<names.size();++i){auto* b=buttons.add(new TextButton(names[i]));addAndMakeVisible(b);b->setColour(TextButton::buttonColourId,colour);b->setColour(TextButton::buttonOnColourId,colour.brighter(.12f));b->setColour(TextButton::textColourOffId,Colour(0xff202729));b->onClick=[this,i]{attachment->setValueAsCompleteGesture(float(i));};}
   attachment=std::make_unique<ParameterAttachment>(parameter,[this](float value){for(int i=0;i<buttons.size();++i)buttons[i]->setToggleState(i==roundToInt(value),dontSendNotification);},nullptr);attachment->sendInitialUpdate();
  }
  void resized()override{label.setBounds(0,0,getWidth(),25);for(int i=0;i<buttons.size();++i)buttons[i]->setBounds(5,34+i*31,getWidth()-10,25);}
 };
 struct Switch {std::unique_ptr<Selector> selector;int page;};
public:
 explicit SynthEditPanel(AudioProcessorValueTreeState& s):state(s),surface(makeSatinPanel(1190,330)){
  const char* titles[]={"01   OSCILLATORS","02   MODULATION / VOICES","03   DUAL FILTER"};
  for(int i=0;i<3;++i){tabs[size_t(i)].setButtonText(titles[i]);tabs[size_t(i)].setColour(TextButton::buttonColourId,Colour(0xffb8ab8b));tabs[size_t(i)].setColour(TextButton::buttonOnColourId,Colour(0xffd5b675));addAndMakeVisible(tabs[size_t(i)]);tabs[size_t(i)].setBounds(14+i*236,9,224,28);tabs[size_t(i)].onClick=[this,i]{showPage(i);};}
  select("osc1Shape","WAVE",{"MIX","SAW","PULSE","TRI","SINE"},0,18,92);select("osc1Octave","OCTAVE",{"-2","-1","0","+1","+2"},0,114,62);
  fader("osc1Level","LEVEL",0,186);fader("width","WIDTH",0,264);fader("wave","SAW / PULSE",0,342);fader("sub","SUB",0,420);fader("noise","NOISE",0,498);
  select("osc2Shape","WAVE",{"MIX","SAW","PULSE","TRI","SINE"},0,608,92);select("osc2Octave","OCTAVE",{"-2","-1","0","+1","+2"},0,704,62);
  fader("oscLayer","LEVEL",0,786);fader("osc2Width","WIDTH",0,864);fader("osc2Tune","SEMITONES",0,942);fader("detune","FINE",0,1020);
  select("lfoShape","WAVE",{"TRI","SINE","SAW","SQUARE"},1,18,92);
  fader("rate","RATE",1,118);fader("pwm","PWM",1,196);fader("lfoPitch","PITCH",1,274);fader("lfoFilter","FILTER",1,352);fader("envPitch","ENV > PITCH",1,430);fader("wheelDepth","MOD WHEEL",1,508);
  select("voiceMode","ASSIGN",{"POLY","MONO","UNISON"},1,608,100);
  select("legato","ENVELOPE",{"RETRIG","LEGATO"},1,716,100);
  fader("unisonSpread","SPREAD",1,834);fader("bendRange","BEND",1,934);fader("portamento","GLIDE",1,1034);
  select("filterAType","MODEL",{"LADDER","SVF"},2,18,90);select("slope","SLOPE",{"12 dB","24 dB"},2,110,65);
  fader("cutoff","CUTOFF",2,178);fader("resonance","RES",2,250);fader("env","ENV",2,322);fader("keytrack","KEY",2,394);
  select("filterBType","MODEL",{"LADDER","SVF"},2,488,90);select("slopeB","SLOPE",{"12 dB","24 dB"},2,580,65);
  fader("cutoffB","CUTOFF",2,650);fader("resonanceB","RES",2,722);fader("envB","ENV",2,794);
  select("filterRoute","SIGNAL PATH",{"A ONLY","B ONLY","A > B","B > A","PARALLEL"},2,900,138);fader("filterBlend","A / B MIX",2,1060);
  showPage(0);
 }
 void showPage(int index){page=index;for(int i=0;i<3;++i)tabs[size_t(i)].setToggleState(page==i,dontSendNotification);for(auto& f:faders){f->slider.setVisible(f->page==page);f->label.setVisible(f->page==page);}for(auto& c:switches)c.selector->setVisible(c.page==page);repaint();}
 void paint(Graphics& g)override{
  g.drawImageAt(surface,0,0);
  g.setColour(Colour(0xffa38a62));g.drawHorizontalLine(0,0,1190);g.setColour(Colour(0xff211a14));g.drawRect(getLocalBounds(),2);
  auto section=[&](int x,int width,const String& title,Colour colour){g.setColour(colour);g.fillRect(x,49,width,22);g.setColour(Colour(0xfff0dfb8));g.setFont(FontOptions(12.f,Font::bold));g.drawText(title,x+9,49,width-18,22,Justification::centredLeft);g.setColour(Colour(0xff8e7b5c));g.drawVerticalLine(x+width,76,288);};
  if(page==0){section(10,574,"DCO I   /   OSCILLATOR",Colour(0xff956041));section(600,574,"VCO II   /   OSCILLATOR",Colour(0xffa57b43));}
  if(page==1){section(10,574,"LFO   /   MODULATION",Colour(0xff956041));section(600,574,"KEY ASSIGN   /   PERFORMANCE",Colour(0xff7b805b));}
  if(page==2){section(10,458,"VCF A",Colour(0xff956041));section(480,398,"VCF B",Colour(0xffa57b43));section(890,284,"ROUTING",Colour(0xff7b805b));}
  g.setColour(Colour(0xff292219));g.fillRect(10,298,1164,23);g.setColour(Colour(0xffbdad90));g.setFont(FontOptions(10.f));
  const char* hints[]={"MIX : original saw / pulse blend · oscillator II follows width I in MIX mode","LEGATO : envelopes stay open between overlapping notes in MONO / UNISON","LADDER / SVF : two filter models · A / B MIX controls the parallel balance"};g.drawText(hints[page],22,299,1120,21,Justification::centredLeft);
  g.setColour(Colour(0xffc5b495));g.setFont(FontOptions(10.f));g.drawText("ORBIT  /  SYNTHESIS",758,12,402,22,Justification::centredRight);
 }
private:
 void fader(const char* id,const char* title,int p,int x){auto f=std::make_unique<Fader>();f->page=p;f->label.setText(title,dontSendNotification);f->label.setFont(FontOptions(10.f,Font::bold));f->label.setJustificationType(Justification::centred);addAndMakeVisible(f->label);f->label.setBounds(x,80,72,24);f->slider.setSliderStyle(Slider::LinearVertical);f->slider.setTextBoxStyle(Slider::NoTextBox,false,0,0);f->slider.setPopupDisplayEnabled(true,true,this);f->slider.setName(state.getParameter(id)->getName(60));f->slider.setDoubleClickReturnValue(true,state.getParameter(id)->convertFrom0to1(state.getParameter(id)->getDefaultValue()));addAndMakeVisible(f->slider);f->slider.setBounds(x+12,119,48,157);f->attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(state,id,f->slider);faders.push_back(std::move(f));}
 void select(const char* id,const char* title,StringArray names,int p,int x,int width){Switch c;c.page=p;c.selector=std::make_unique<Selector>(*state.getParameter(id),title,names,p==2?Colour(0xffcfb480):Colour(0xffc1ba96));addAndMakeVisible(*c.selector);c.selector->setBounds(x,80,width,210);switches.push_back(std::move(c));}
 AudioProcessorValueTreeState& state;Image surface;int page=0;std::array<TextButton,3> tabs;std::vector<std::unique_ptr<Fader>> faders;std::vector<Switch> switches;
};
class Editor : public AudioProcessorEditor, private Timer {
 struct Control {String id;std::unique_ptr<Slider> slider;std::unique_ptr<Label> label;std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;};
 struct Group {const char* name;int count;};
 const std::array<Group,8> groups{{{"LFO",2},{"OSCILLATOR",5},{"HPF",1},{"VCF",4},{"VCA",2},{"AMP ENV",4},{"FILTER ENV",4},{"CHORUS",1}}};
public:
 Editor(Orbit& p):AudioProcessorEditor(p),processor(p),keyboard(p.keyboard,MidiKeyboardComponent::horizontalKeyboard) {
  setLookAndFeel(&look);
  mode.addItemList({"106 / 6 VOICES","JUPITER / 8 VOICES"},1);addAndMakeVisible(mode);modeAttachment=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
  slope.addItemList({"12 dB / OCT","24 dB / OCT"},1);addAndMakeVisible(slope);slopeAttachment=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"slope",slope);
  chorusMode.addItemList({"CHORUS I","CHORUS II"},1);addAndMakeVisible(chorusMode);chorusAttachment=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"chorusMode",chorusMode);
  const char* ids[]={"rate","pwm","wave","width","sub","detune","noise","hpf","cutoff","resonance","env","keytrack","gain","velocity","attack","decay","sustain","release","fattack","fdecay","fsustain","frelease","chorus"};
  const char* names[]={"RATE","PWM","WAVE","WIDTH","SUB","DETUNE","NOISE","FREQ","FREQ","RES","ENV","KEY","LEVEL","VEL","A","D","S","R","A","D","S","R","MIX"};
  for(int i=0;i<23;++i) {
   Control c;c.id=ids[i];c.slider=std::make_unique<Slider>();c.label=std::make_unique<Label>();
   c.slider->setSliderStyle(Slider::LinearVertical);c.slider->setTextBoxStyle(Slider::NoTextBox,false,0,0);c.slider->setPopupDisplayEnabled(true,true,this);c.slider->setName(p.state.getParameter(ids[i])->getName(40));c.slider->setTooltip(c.slider->getName());c.slider->setDoubleClickReturnValue(true,p.state.getParameter(ids[i])->convertFrom0to1(p.state.getParameter(ids[i])->getDefaultValue()));
   c.label->setText(names[i],dontSendNotification);c.label->setFont(FontOptions(11.f));c.label->setJustificationType(Justification::centred);c.label->setColour(Label::textColourId,Colour(0xffd9c7a2));
   addAndMakeVisible(*c.slider);addAndMakeVisible(*c.label);c.attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],*c.slider);controls.push_back(std::move(c));
  }
  refreshPresetMenu();
  presets.setSelectedId(p.getCurrentProgram()+1,dontSendNotification);addAndMakeVisible(presets);
  presets.onChange=[this]{int id=presets.getSelectedId();if(id>=1000){auto index=id-1000;if(isPositiveAndBelow(index,userNames.size()))showResult(processor.loadUserPreset(userNames[index]));}else if(id>0)processor.setCurrentProgram(id-1);};
  saveButton.setButtonText("SAVE");addAndMakeVisible(saveButton);saveButton.setBounds(800,25,60,27);saveButton.onClick=[this]{saveDialog();};
  loadButton.setButtonText("LOAD");addAndMakeVisible(loadButton);loadButton.setBounds(800,56,60,27);loadButton.onClick=[this]{refreshPresetMenu();if(processor.userPresetName().isNotEmpty())showResult(processor.loadUserPreset(processor.userPresetName()));else processor.setCurrentProgram(processor.getCurrentProgram());};
  for(auto* b:{&previous,&next,&octDown,&octUp}) addAndMakeVisible(*b);
  previous.setButtonText("PREV");next.setButtonText("NEXT");octDown.setButtonText("- OCT");octUp.setButtonText("+ OCT");
  previous.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+processor.getNumPrograms()-1)%processor.getNumPrograms());};
  next.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+1)%processor.getNumPrograms());};
  octDown.onClick=[this]{octave=jmax(0,octave-1);updateKeyboard();};octUp.onClick=[this]{octave=jmin(4,octave+1);updateKeyboard();};
  keyboard.setAvailableRange(0,127);keyboard.setScrollButtonsVisible(false);keyboard.setKeyWidth(810.f/36.f);keyboard.setOctaveForMiddleC(4);keyboard.setBlackNoteLengthProportion(.63f);
  keyboard.setColour(MidiKeyboardComponent::whiteNoteColourId,Colour(0xffeeeadd));keyboard.setColour(MidiKeyboardComponent::blackNoteColourId,Colour(0xff11171a));keyboard.setColour(MidiKeyboardComponent::keyDownOverlayColourId,Colour(0xffe3a454).withAlpha(.7f));keyboard.setColour(MidiKeyboardComponent::mouseOverKeyOverlayColourId,Colours::white.withAlpha(.15f));keyboard.setColour(MidiKeyboardComponent::textLabelColourId,Colour(0xff657070));
  for(int i=0;i<8;++i){auto button=std::make_unique<TextButton>(String(i+1));button->setTooltip(p.getProgramName(i));button->setColour(TextButton::buttonColourId,i<4?Colour(0xffb3b490):Colour(0xffdfc996));button->setColour(TextButton::textColourOffId,Colour(0xff26312f));button->onClick=[this,i]{processor.setCurrentProgram(shownBank*8+i);};addAndMakeVisible(*button);patchButtons.push_back(std::move(button));}
  for(int i=0;i<3;++i){auto button=std::make_unique<TextButton>(String::charToString(char('A'+i)));button->setColour(TextButton::buttonColourId,Colour(0xffd5bd8d));button->setColour(TextButton::textColourOffId,Colour(0xff25312d));button->onClick=[this,i]{shownBank=i;refreshMemory();};addAndMakeVisible(*button);bankButtons.push_back(std::move(button));}
  addKnob("oscLayer","OSC II LAYER",434,12,100);
  envLink.setButtonText("LINK AMP / FILTER ENV");envLink.setClickingTogglesState(true);addAndMakeVisible(envLink);linkAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state,"envLink",envLink);envLink.setBounds(565,31,218,30);
  metal=makeSatinPanel(1244,800);
  addKnob("portamento","PORTAMENTO",38,478,108);
  addKnob("arpTempo","TEMPO",180,512,90);addKnob("arpGate","GATE",280,512,90);
  addChoice(arpPattern,"arpPattern",{"UP","DOWN","UP / DOWN"});addChoice(arpDivision,"arpDivision",{"1/4","1/8","1/16"});addChoice(arpOctaves,"arpOctaves",{"1 OCT","2 OCT","3 OCT"});
  arpOn.setButtonText("ARP ON");sync.setButtonText("HOST SYNC");arpOn.setClickingTogglesState(true);sync.setClickingTogglesState(true);addAndMakeVisible(arpOn);addAndMakeVisible(sync);
  arpAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state,"arpOn",arpOn);syncAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state,"arpSync",sync);
  addKnob("fxDrive","DRIVE",1000,490,72);addKnob("chorus","CHORUS",1000,590,72);addKnob("gain","OUTPUT",1000,690,72);
  addKnob("delayTime","TIME",1076,490,72);addKnob("delayFeedback","FEEDBACK",1076,590,72);addKnob("delayMix","LEVEL",1076,690,72);
  addKnob("reverbSize","SIZE",1152,490,72);addKnob("reverbDamp","DAMP",1152,590,72);addKnob("reverbMix","LEVEL",1152,690,72);
  addAndMakeVisible(keyboard);
  editPanel=std::make_unique<SynthEditPanel>(p.state);addChildComponent(*editPanel);editPanel->setBounds(34,103,1190,330);
  editButton.setButtonText("SYNTH EDIT");editButton.setClickingTogglesState(true);addAndMakeVisible(editButton);editButton.setBounds(260,26,145,30);editButton.onClick=[this]{editPanel->setVisible(editButton.getToggleState());editPanel->toFront(false);editButton.setButtonText(editButton.getToggleState()?"BACK / PLAY":"SYNTH EDIT");};
  setSize(1280,820);updateKeyboard();timerCallback();startTimerHz(15);
 }
 ~Editor() override {stopTimer();setLookAndFeel(nullptr);}
 void paint(Graphics& g) override {
  g.fillAll(Colour(0xff241b14));
  g.drawImageAt(metal,18,10);
  g.setGradientFill(ColourGradient(Colour(0xff8b7554),0,10,Colour(0xff4c3e2d),0,22,false));g.fillRect(18,10,1244,12);
  g.setColour(Colour(0xffa28b64));g.drawHorizontalLine(10,20,1260);g.setColour(Colour(0xff21180f));g.fillRect(18,802,1244,8);
  g.setColour(Colour(0xffb58d4b));g.fillRect(34,88,1190,4);
  g.setColour(Colour(0xffb9a98b));g.drawHorizontalLine(350,34,1224);
  // Walnut cheeks: long, irregular grain, satin edges and recessed screws.
  for(int side:{0,1254}){
   g.setGradientFill(ColourGradient(Colour(0xff86603c),float(side),0,Colour(0xff3a2418),float(side+26),0,false));g.fillRoundedRectangle(float(side),0,26,820,5);
   for(int line=0;line<17;++line){Path grain;for(int y=0;y<=820;y+=8){float x=float(side)+2+line*1.4f+1.3f*std::sin(y*.014f+line*.83f)+.5f*std::sin(y*.041f+line);if(y==0)grain.startNewSubPath(x,0);else grain.lineTo(x,float(y));}g.setColour((line%3==0?Colour(0xffd3a36b):Colour(0xff27160e)).withAlpha(.18f));g.strokePath(grain,PathStrokeType(line%3==0?.7f:1.1f));}
   g.setColour(Colour(0xffd5ad74).withAlpha(.35f));g.drawVerticalLine(side+2,7,813);g.setColour(Colour(0xff1c120c));g.drawVerticalLine(side+25,4,816);
  }
  for(int x:{13,1267}) for(int y:{24,796}) {g.setColour(Colour(0xff7b8383));g.fillEllipse(float(x-3),float(y-3),6,6);g.setColour(Colour(0xff20282d));g.drawLine(float(x-2),float(y),float(x+2),float(y));}
  g.setColour(Colour(0xfff0dfb8));g.setFont(FontOptions(38.f,Font::bold|Font::italic));g.drawText("orbit",42,20,190,44,Justification::centredLeft);
  g.setFont(FontOptions(11.f));g.setColour(Colour(0xffbdae8e));g.drawText("POLYPHONIC   /   ANALOG SPIRIT",40,64,380,18,Justification::centredLeft);
  g.setColour(look.accent);g.setFont(FontOptions(25.f,Font::bold));g.drawText(jupiter?"JUPITER / 08":"CLASSIC / 106",870,20,365,44,Justification::centredRight);
  g.setColour(Colour(0xffc4b18b));g.setFont(FontOptions(10.f));g.drawText("OSC II + SUB / INDEPENDENT FILTER",545,66,245,16,Justification::centred);
  int idx=0;float x=34;const float unit=51.8f;
  for(const auto& group:groups) {
   float width=group.count*unit;g.setColour(look.accent);g.fillRect(x,103.f,width-3,23.f);
   g.setColour(Colour(0xfffcf3e7));g.setFont(FontOptions(12.f,Font::bold));g.drawText(group.name,Rectangle<float>(x,103,width-3,23),Justification::centred);
   g.setColour(Colour(0xff8e7e61));g.drawVerticalLine(int(x+width-2),130,342);x+=width;idx+=group.count;
  }
  g.setColour(look.accent);g.fillRect(34,345,1190,3);
  g.setColour(Colour(0xff30281d));g.fillRoundedRectangle(34,365,1190,70,4);
  g.setFont(FontOptions(10.f,Font::bold));g.setColour(Colour(0xffc9b793));g.drawText("VOICE ALLOCATION",46,370,220,16,Justification::centredLeft);g.drawText(processor.isPatchModified()?"PATCH MODIFIED / SAVED WITH YOUR SESSION":"FACTORY PATCH / 24 ORIGINAL SOUNDS",400,370,390,16,Justification::centredLeft);g.drawText("FILTER SLOPE",925,370,130,16,Justification::centredLeft);g.drawText("ENSEMBLE",1080,370,130,16,Justification::centredLeft);
  g.setColour(Colour(0xff30281e));g.fillRoundedRectangle(34,447,944,35,3);
  g.setFont(FontOptions(11.f,Font::bold));g.setColour(Colour(0xffc6b18b));g.drawText("BANK",46,451,70,24,Justification::centredLeft);
  g.setColour(Colour(0xff7c8056));g.fillRect(164,493,814,20);g.setColour(Colour(0xfff0dfb8));g.drawText("ARPEGGIATOR",174,493,240,20,Justification::centredLeft);
  g.setColour(Colour(0xffc9b794));g.setFont(FontOptions(10.f));g.drawText("PATTERN",400,553,145,16,Justification::centredLeft);g.drawText("STEP",570,553,120,16,Justification::centredLeft);g.drawText("RANGE",710,553,120,16,Justification::centredLeft);
  g.setColour(Colour(0xff30271d));g.fillRoundedRectangle(992,447,240,342,4);
  const char* effects[]={"EFFECTS","DELAY","REVERB"};for(int i=0;i<3;++i){g.setColour(look.accent);g.fillRect(998+i*76,453,74,23);g.setColour(Colour(0xfff5e1b5));g.setFont(FontOptions(11.f,Font::bold));g.drawText(effects[i],998+i*76,453,74,23,Justification::centred);}
  g.setColour(Colour(0xffcbbb98));g.setFont(FontOptions(11.f));g.drawText("KEYBOARD",38,597,110,18,Justification::centred);g.drawText("C"+String(octave),38,714,110,22,Justification::centred);
  g.setColour(Colour(0xff231c14));g.fillRect(164,603,810,179);
  g.setColour(Colour(0xffaa987a));g.setFont(FontOptions(10.f));g.drawText("DOUBLE-CLICK TO RESET  /  HOVER FOR VALUE  /  PATCHES SAVED WITH SESSION",164,790,810,16,Justification::centredLeft);

 }
 void resized() override {
  for(size_t i=0;i<controls.size();++i) {int x=34+int(i*51.8f);controls[i].label->setBounds(x,139,51,24);controls[i].slider->setBounds(x+2,169,47,160);}
  mode.setBounds(46,391,240,31);previous.setBounds(308,391,70,31);presets.setBounds(400,391,400,31);next.setBounds(818,391,70,31);slope.setBounds(925,391,135,31);chorusMode.setBounds(1080,391,130,31);
  for(int i=0;i<3;++i)bankButtons[size_t(i)]->setBounds(122+i*53,452,43,24);
  for(int i=0;i<int(patchButtons.size());++i)patchButtons[size_t(i)]->setBounds(310+i*82,452,70,24);
  arpOn.setBounds(400,523,145,26);sync.setBounds(570,523,145,26);arpPattern.setBounds(400,573,145,25);arpDivision.setBounds(570,573,120,25);arpOctaves.setBounds(710,573,120,25);
  octDown.setBounds(40,630,105,25);octUp.setBounds(40,668,105,25);keyboard.setBounds(164,610,810,170);
 }
private:
 void refreshPresetMenu(){presets.clear(dontSendNotification);presets.addSectionHeading("FACTORY / 24 PATCHES");for(int i=0;i<processor.getNumPrograms();++i)presets.addItem(processor.getProgramName(i),i+1);userNames=processor.userPresetNames();if(!userNames.isEmpty()){presets.addSeparator();presets.addSectionHeading("USER / SAVED PATCHES");for(int i=0;i<userNames.size();++i)presets.addItem("USER / "+userNames[i],1000+i);}}
 void showResult(const Result& result){if(result.failed())AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Preset",result.getErrorMessage());}
 void saveDialog(){auto* dialog=new AlertWindow("Enregistrer un preset","Enregistrement dans VSTJUNO/Presets/User. Un nom existant est conservé.",MessageBoxIconType::NoIcon);dialog->addTextEditor("name",processor.userPresetName(),"Nom du preset");dialog->addButton("Enregistrer",1,KeyPress(KeyPress::returnKey));dialog->addButton("Annuler",0,KeyPress(KeyPress::escapeKey));Component::SafePointer<Editor> safe(this);dialog->enterModalState(true,ModalCallbackFunction::create([safe,dialog](int result){if(result==1 && safe){auto status=safe->processor.saveUserPreset(dialog->getTextEditorContents("name").trim());safe->showResult(status);if(status.wasOk())safe->refreshPresetMenu();}}),true);}
 void addKnob(const char* id,const char* name,int x,int y,int width){Control c;c.id=id;c.slider=std::make_unique<Slider>();c.label=std::make_unique<Label>();c.slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);c.slider->setTextBoxStyle(Slider::NoTextBox,false,0,0);c.slider->setPopupDisplayEnabled(true,true,this);c.slider->setName(processor.state.getParameter(id)->getName(40));c.slider->setColour(Slider::rotarySliderFillColourId,Colour(0xffdbab67));c.slider->setColour(Slider::thumbColourId,Colour(0xffeff0de));c.slider->setDoubleClickReturnValue(true,processor.state.getParameter(id)->convertFrom0to1(processor.state.getParameter(id)->getDefaultValue()));c.label->setText(name,dontSendNotification);c.label->setFont(FontOptions(10.f));c.label->setJustificationType(Justification::centred);addAndMakeVisible(*c.slider);addAndMakeVisible(*c.label);c.label->setBounds(x,y,width,18);c.slider->setBounds(x+5,y+18,width-10,60);c.attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,*c.slider);knobs.push_back(std::move(c));}
 void addChoice(ComboBox& box,const char* id,StringArray items){box.addItemList(items,1);addAndMakeVisible(box);extraChoices.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,id,box));}
 void updateKeyboard(){keyboard.setAvailableRange(12*(octave+1),12*(octave+1)+60);keyboard.setKeyPressBaseOctave(octave+1);repaint();}
 void timerCallback() override {
  bool now=processor.state.getRawParameterValue("mode")->load()>.5f;
  if(now!=jupiter){jupiter=now;look.accent=jupiter?Colour(0xffaa8048):Colour(0xff946344);repaint();}
  if(wasModified!=processor.isPatchModified()){wasModified=processor.isPatchModified();repaint();}
  const auto userName=processor.userPresetName();if(userName.isEmpty())presets.setSelectedId(processor.getCurrentProgram()+1,dontSendNotification);else {int index=userNames.indexOf(userName);if(index>=0)presets.setSelectedId(1000+index,dontSendNotification);else presets.setText("USER / "+userName,dontSendNotification);}
  for(auto& c:controls){bool enabled=true;if(c.id=="fattack" || c.id=="fdecay" || c.id=="fsustain" || c.id=="frelease") enabled=processor.state.getRawParameterValue("envLink")->load()<.5f;c.slider->setEnabled(enabled);c.label->setAlpha(enabled?1.f:.4f);}
  slope.setEnabled(true);
  if(lastProgram!=processor.getCurrentProgram()){lastProgram=processor.getCurrentProgram();shownBank=lastProgram/8;}
  refreshMemory();
 } 
 void refreshMemory(){for(int i=0;i<8;++i){patchButtons[size_t(i)]->setToggleState(shownBank*8+i==processor.getCurrentProgram() && processor.userPresetName().isEmpty(),dontSendNotification);patchButtons[size_t(i)]->setTooltip(processor.getProgramName(shownBank*8+i));}for(int i=0;i<3;++i)bankButtons[size_t(i)]->setToggleState(i==shownBank,dontSendNotification);

 }
 Orbit& processor;PanelLook look;PianoKeyboard keyboard;ComboBox mode,presets,slope,chorusMode;TextButton previous,next,octDown,octUp;int octave=2;bool jupiter=false,wasModified=false;
 std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment,slopeAttachment,chorusAttachment;
 std::unique_ptr<SynthEditPanel> editPanel;TextButton editButton;
 StringArray userNames;TextButton saveButton,loadButton;
 Image metal;int shownBank=0,lastProgram=-1;TextButton envLink;std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;std::vector<std::unique_ptr<TextButton>> bankButtons;
 ComboBox arpPattern,arpDivision,arpOctaves;TextButton arpOn,sync;
 std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> arpAttachment,syncAttachment;
 std::vector<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>> extraChoices;
 std::vector<std::unique_ptr<TextButton>> patchButtons;
 std::vector<Control> controls,knobs;
};
