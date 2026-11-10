#pragma once
// Orbit front panel: an original layout in the spirit of early-80s programmable polysynths.
// Everything is laid out on one fixed 1480 x 740 design surface, which is scaled as a whole
// when the plugin window is resized. Everything is on one face: a row of faders for the playing
// controls, and under it a row of knobs and switches for shapes, modulation and the second filter.

namespace orbitui {
 const Colour panel{0xff232527}, ink{0xfff1eee6}, dim{0xffb9b6ad}, amber{0xffe9a23b}, teal{0xff4fb3c4};
 const Colour neutral{0xffcfccc4}, bright{0xffe6e3db}, led{0xffff5a3c}, slot{0xff0b0c0d}, print{0xff141516};
 inline Font face(float size, bool bold = true) { return Font(FontOptions("Avenir Next Condensed", size, bold ? Font::bold : Font::plain)); }
 inline Font mono(float size) { return Font(FontOptions("Menlo", size, Font::plain)); }
 inline String utf8(const char* text) { return String::fromUTF8(text); }
 inline void tint(TextButton& button, Colour colour) { button.setColour(TextButton::buttonColourId, colour); button.setColour(TextButton::buttonOnColourId, colour); }
 // "led" property on a button: 0 = plain cap, 1 = LED above the cap, 2 = LED left of the cap.
 inline int ledMode(Button& button) { return int(button.getProperties().getWithDefault("led", 0)); }
}

class OrbitLook : public LookAndFeel_V4 {
public:
 OrbitLook() {
  using namespace orbitui;
  setColour(Label::textColourId, ink);
  setColour(TextButton::buttonColourId, neutral); setColour(TextButton::buttonOnColourId, neutral);
  setColour(ComboBox::backgroundColourId, Colours::transparentBlack); setColour(ComboBox::outlineColourId, Colours::transparentBlack);
  setColour(ComboBox::textColourId, led); setColour(ComboBox::arrowColourId, led);
  setColour(PopupMenu::backgroundColourId, Colour(0xff17181a)); setColour(PopupMenu::textColourId, ink); setColour(PopupMenu::headerTextColourId, dim);
  setColour(PopupMenu::highlightedBackgroundColourId, amber); setColour(PopupMenu::highlightedTextColourId, print);
 }
 Font getTextButtonFont(TextButton&, int height) override { return orbitui::face(jlimit(11.f, 15.f, height * .6f)); }
 Font getComboBoxFont(ComboBox&) override { return orbitui::mono(19.f); }
 Font getPopupMenuFont() override { return orbitui::face(16.f, false); }

 void drawButtonBackground(Graphics& g, Button& button, const Colour& colour, bool over, bool down) override {
  using namespace orbitui;
  auto area = button.getLocalBounds().toFloat();
  if (const int mode = ledMode(button); mode != 0) {
   auto spot = (mode == 1 ? area.removeFromTop(12) : area.removeFromLeft(14)).withSizeKeepingCentre(7, 7);
   const bool on = button.getToggleState();
   if (on) { g.setColour(led.withAlpha(.35f)); g.fillEllipse(spot.expanded(3)); }
   g.setColour(on ? led : Colour(0xff3a1512)); g.fillEllipse(spot);
  }
  auto well = area.reduced(1);
  auto cap = well.withTrimmedBottom(3);
  if (down) cap.translate(0, 2);
  g.setColour(Colour(0xff0c0d0e)); g.fillRoundedRectangle(well, 3);
  const auto base = (over ? colour.brighter(.12f) : colour).withMultipliedAlpha(button.isEnabled() ? 1.f : .5f);
  g.setGradientFill(ColourGradient(base, 0, cap.getY(), base.darker(.28f), 0, cap.getBottom(), false)); g.fillRoundedRectangle(cap, 3);
  g.setColour(Colours::white.withAlpha(.4f)); g.drawHorizontalLine(int(cap.getY()) + 1, cap.getX() + 2, cap.getRight() - 2);
 }
 void drawButtonText(Graphics& g, TextButton& button, bool, bool down) override {
  auto area = button.getLocalBounds();
  if (const int mode = orbitui::ledMode(button); mode == 1) area.removeFromTop(12); else if (mode == 2) area.removeFromLeft(14);
  area = area.reduced(3, 1).withTrimmedBottom(3);
  if (down) area.translate(0, 2);
  g.setFont(getTextButtonFont(button, area.getHeight()));
  g.setColour(orbitui::print.withAlpha(button.isEnabled() ? 1.f : .5f));
  g.drawFittedText(button.getButtonText(), area, Justification::centred, 1, .7f);
 }

 // Thin fader: dark slot, tick marks on both sides, small light cap.
 void drawLinearSlider(Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider& slider) override {
  using namespace orbitui;
  const float cx = float(x) + float(w) * .5f, top = float(y), length = float(h);
  g.setColour(dim.withAlpha(slider.isEnabled() ? .9f : .35f));
  const int marks = jmax(1, int(slider.getProperties().getWithDefault("marks", 10))); // a switch-like fader shows one mark per stop
  for (int i = 0; i <= marks; ++i) {
   const float yy = top + length * float(i) / float(marks), mark = (marks != 10 || i % 5 == 0) ? 9.f : 6.f;
   g.fillRect(cx - 8 - mark, yy - .5f, mark, 1.f); g.fillRect(cx + 8, yy - .5f, mark, 1.f);
  }
  g.setColour(slot); g.fillRoundedRectangle(cx - 3, top - 7, 6, length + 14, 3);
  g.setColour(Colour(0xff3a3d40)); g.drawVerticalLine(int(cx) + 3, top - 5, top + length + 5);
  const Rectangle<float> cap(cx - 12, pos - 6.5f, 24, 13);
  g.setColour(Colours::black.withAlpha(.5f)); g.fillRoundedRectangle(cap.translated(0, 3), 2);
  g.setGradientFill(ColourGradient(Colour(0xffe9e7e1), 0, cap.getY(), Colour(0xffa9a7a0), 0, cap.getBottom(), false)); g.fillRoundedRectangle(cap, 2);
  g.setColour(Colour(0xff2a2c2e)); g.fillRect(cap.getX() + 2, cap.getCentreY() - .5f, cap.getWidth() - 4, 1.f);
  if (!slider.isEnabled()) { g.setColour(panel.withAlpha(.6f)); g.fillRoundedRectangle(cap.expanded(1), 2); }
 }
 void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float value, float start, float end, Slider& slider) override {
  using namespace orbitui;
  auto area = Rectangle<float>(float(x), float(y), float(w), float(h));
  const bool selector = slider.getProperties().contains("selector");
  const float diameter = jmin(area.getWidth(), area.getHeight()) - (selector ? 6.f : 14.f);
  area = area.withSizeKeepingCentre(diameter, diameter);
  const auto centre = area.getCentre(); const float angle = start + value * (end - start), outer = diameter * .5f;
  g.setColour(dim);
  if (!selector) for (int i = 0; i <= 10; ++i) { const float a = start + (end - start) * float(i) / 10.f; g.drawLine(centre.x + std::sin(a) * (outer + 2), centre.y - std::cos(a) * (outer + 2), centre.x + std::sin(a) * (outer + 5), centre.y - std::cos(a) * (outer + 5), 1.f); }
  g.setColour(Colours::black.withAlpha(.45f)); g.fillEllipse(area.translated(0, 3));
  g.setColour(Colour(0xff101112)); g.fillEllipse(area);
  const auto dial = area.reduced(3);
  g.setGradientFill(ColourGradient(Colour(0xff5a5d60), dial.getX() + dial.getWidth() * .3f, dial.getY() + dial.getHeight() * .25f, Colour(0xff17181a), dial.getRight(), dial.getBottom(), true)); g.fillEllipse(dial);
  const float radius = dial.getWidth() * .5f;
  g.setColour(ink); g.drawLine(centre.x + std::sin(angle) * radius * .35f, centre.y - std::cos(angle) * radius * .35f, centre.x + std::sin(angle) * (radius - 2), centre.y - std::cos(angle) * (radius - 2), 2.f);
 }
 // The patch selector is drawn as text inside the LED display window painted by the editor.
 void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override {
  Path arrow; arrow.addTriangle(float(w) - 20, float(h) * .5f - 3, float(w) - 8, float(h) * .5f - 3, float(w) - 14, float(h) * .5f + 4);
  g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha(.8f)); g.fillPath(arrow);
 }
 void positionComboBoxText(ComboBox& box, Label& label) override { label.setBounds(12, 0, box.getWidth() - 36, box.getHeight()); label.setFont(getComboBoxFont(box)); }
};

class PianoKeyboard : public MidiKeyboardComponent {
public:
 using MidiKeyboardComponent::MidiKeyboardComponent;
 void drawWhiteNote(int note, Graphics& g, Rectangle<float> r, bool down, bool over, Colour, Colour) override {
  const auto top = down ? Colour(0xffe2b877) : Colour(0xffd9d6cf), body = down ? Colour(0xfff0d3a0) : Colour(0xfff4f2ec);
  g.setGradientFill(ColourGradient(top, 0, r.getY(), body, 0, r.getY() + 30, false)); g.fillRect(r);
  g.setColour(Colour(0xffc9c6be)); g.fillRect(r.getX() + 1, r.getBottom() - 5, r.getWidth() - 2, 4.f);
  g.setColour(Colour(0xff77756f)); g.drawRect(r, .6f);
  if (over && !down) { g.setColour(Colours::white.withAlpha(.3f)); g.fillRect(r); }
  if (note % 12 == 0) { g.setColour(Colour(0xff6d6b65)); g.setFont(orbitui::face(11.f, false)); g.drawText(MidiMessage::getMidiNoteName(note, true, true, 4), r.removeFromBottom(22), Justification::centred); }
 }
 void drawBlackNote(int, Graphics& g, Rectangle<float> r, bool down, bool over, Colour) override {
  g.setColour(Colour(0xff0d0e0f)); g.fillRoundedRectangle(r, 2);
  auto inner = r.reduced(2, 1); inner.removeFromBottom(down ? 3 : 7);
  g.setGradientFill(ColourGradient(down ? Colour(0xffb9853f) : Colour(0xff3a3d40), inner.getX(), 0, Colour(0xff121314), inner.getRight(), 0, false)); g.fillRoundedRectangle(inner, 2);
  if (over && !down) { g.setColour(Colours::white.withAlpha(.12f)); g.fillRoundedRectangle(r, 2); }
 }
};

// Row or column of latching buttons with LEDs, bound to one choice or bool parameter.
class ChoiceButtons : public Component {
public:
 ChoiceButtons(RangedAudioParameter& parameter, const StringArray& names, bool vertical, Colour colour)
  : column(vertical), attachment(parameter, [this](float value) { for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState(i == roundToInt(value), dontSendNotification); }, nullptr) {
  for (int i = 0; i < names.size(); ++i) {
   auto* button = buttons.add(new TextButton(names[i]));
   button->getProperties().set("led", vertical ? 2 : 1); orbitui::tint(*button, colour); button->setWantsKeyboardFocus(false);
   button->onClick = [this, i] { attachment.setValueAsCompleteGesture(float(i)); };
   addAndMakeVisible(button);
  }
  attachment.sendInitialUpdate();
 }
 void resized() override {
  const int count = buttons.size(), gap = 5; if (count == 0) return;
  if (column) { const int each = jmin(30, (getHeight() - gap * (count - 1)) / count); for (int i = 0; i < count; ++i) buttons[i]->setBounds(0, i * (each + gap), getWidth(), each); }
  else { const int each = (getWidth() - gap * (count - 1)) / count; for (int i = 0; i < count; ++i) buttons[i]->setBounds(i * (each + gap), 0, each, getHeight()); }
 }
private:
 bool column; OwnedArray<TextButton> buttons; ParameterAttachment attachment;
};

// Stepped rotary selector with a waveform symbol (or a short text) at each position, bound to a choice parameter.
// Turn the knob or click a symbol; the selected symbol lights up and its name shows underneath.
class WaveKnob : public Component {
public:
 WaveKnob(AudioProcessorValueTreeState& state, const char* id, const StringArray& shapes, bool nameBelow = true) : names(shapes), showName(nameBelow) {
  knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag); knob.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
  knob.setRotaryParameters(startAngle, endAngle, true); knob.setMouseDragSensitivity(90);
  knob.getProperties().set("selector", true); knob.setWantsKeyboardFocus(false); knob.setName(state.getParameter(id)->getName(60));
  addAndMakeVisible(knob);
  attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(state, id, knob);
  knob.onValueChange = [this] { repaint(); };
 }
 void resized() override { knob.setBounds(getWidth() / 2 - 25, centreY - 25, 50, 50); }
 void paint(Graphics& g) override {
  const int current = jlimit(0, names.size() - 1, roundToInt(knob.getValue()));
  for (int i = 0; i < names.size(); ++i) { const bool on = i == current; g.setColour(on ? orbitui::led : orbitui::dim); drawShape(g, names[i], Rectangle<float>(21, 13).withCentre(symbolCentre(i)), on ? 2.f : 1.4f); }
  if (showName) { g.setColour(orbitui::ink); g.setFont(orbitui::face(12.f)); g.drawText(names[current], 0, getHeight() - 16, getWidth(), 14, Justification::centred); }
 }
 void mouseDown(const MouseEvent& event) override { for (int i = 0; i < names.size(); ++i) if (symbolCentre(i).getDistanceFrom(event.position) < 14) knob.setValue(i, sendNotificationSync); }
private:
 static constexpr float startAngle = MathConstants<float>::pi * 1.3f, endAngle = MathConstants<float>::pi * 2.7f;
 enum { centreY = 52 };
 Point<float> symbolCentre(int index) const {
  const float angle = startAngle + (endAngle - startAngle) * float(index) / float(jmax(1, names.size() - 1));
  const float radius = jmin(42.f, (float(getWidth()) * .5f - 11.f) / .81f); // keeps the outer symbols inside a narrow cell
  return {float(getWidth()) * .5f + std::sin(angle) * radius, float(centreY) - std::cos(angle) * radius};
 }
 static void drawShape(Graphics& g, const String& name, Rectangle<float> r, float thickness) {
  Path path; const float x = r.getX(), mid = r.getCentreX(), right = r.getRight(), top = r.getY(), bottom = r.getBottom();
  if (name == "MIX") { // saw and pulse drawn over each other: the blended legacy waveform
   path.startNewSubPath(x, bottom); path.lineTo(right, top); path.lineTo(right, bottom);
   path.startNewSubPath(x, bottom); path.lineTo(x, top); path.lineTo(mid, top); path.lineTo(mid, bottom);
  }
  else if (name == "SAW") { path.startNewSubPath(x, bottom); path.lineTo(right, top); path.lineTo(right, bottom); }
  else if (name == "PULSE" || name == "SQUARE") { path.startNewSubPath(x, bottom); path.lineTo(x, top); path.lineTo(mid, top); path.lineTo(mid, bottom); path.lineTo(right, bottom); }
  else if (name == "TRI") { path.startNewSubPath(x, bottom); path.lineTo(mid, top); path.lineTo(right, bottom); }
  else if (name == "SINE") { for (int k = 0; k <= 16; ++k) { const float px = x + r.getWidth() * float(k) / 16.f, py = r.getCentreY() - std::sin(MathConstants<float>::twoPi * float(k) / 16.f) * r.getHeight() * .5f; if (k == 0) path.startNewSubPath(px, py); else path.lineTo(px, py); } }
  else { g.setFont(orbitui::face(12.f)); g.drawText(name, r.expanded(8, 3), Justification::centred); return; } // octave numbers, routing names
  g.strokePath(path, PathStrokeType(thickness, PathStrokeType::curved, PathStrokeType::rounded));
 }
 StringArray names; bool showName; Slider knob; std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class Editor : public AudioProcessorEditor, private Timer {
 using APVTS = AudioProcessorValueTreeState;
 enum { designW = 1480, designH = 820, left = 40, contentW = 1400, gap = 6 };
 enum { headerY = 16, lineY = 108, panelY = 125, panelH = 190, row2Y = 327, row2H = 166, lowerY = 505, lowerH = 120, keysY = 639, keysH = 159 };
 struct Section { String title; Colour colour; Rectangle<int> area; int page; };
 struct Fader { Slider slider; String id; bool followsLink = false; std::unique_ptr<APVTS::SliderAttachment> attachment; };
 struct Toggle { TextButton button; std::unique_ptr<APVTS::ButtonAttachment> attachment; };
 struct Surface : Component { explicit Surface(Editor& e) : owner(e) {} void paint(Graphics& g) override { owner.paintSurface(g); } Editor& owner; };
 // Hands out consecutive sections of one row; widths are given in fader-sized units.
 struct Row {
  Editor& editor; int x, y, height, page; float unit;
  Row(Editor& e, int rowY, int rowH, int rowPage, float totalUnits, int count) : editor(e), x(left), y(rowY), height(rowH), page(rowPage), unit(float(contentW - gap * (count - 1)) / totalUnits) {}
  Rectangle<int> next(const String& title, Colour colour, float units) {
   const int width = roundToInt(units * unit); const Rectangle<int> area(x, y, width, height);
   editor.sections.push_back({title, colour, area, page}); x += width + gap; return area.withTrimmedTop(34);
  }
 };
public:
 explicit Editor(Orbit& p) : AudioProcessorEditor(p), orbit(p), surface(*this), keyboard(p.keyboard, MidiKeyboardComponent::horizontalKeyboard) {
  setLookAndFeel(&look);
  addAndMakeVisible(surface); surface.setBounds(0, 0, designW, designH);
  buildHeader(); buildPanelA(); buildRowTwo(); buildLowerRow(); buildKeyboard();
  setPage(0);
  setResizable(true, true); getConstrainer()->setFixedAspectRatio(double(designW) / double(designH));
  setResizeLimits(designW * 7 / 10, designH * 7 / 10, designW * 3 / 2, designH * 3 / 2);
  setSize(designW * 17 / 20, designH * 17 / 20);
  timerCallback(); startTimerHz(12);
 }
 ~Editor() override { stopTimer(); setLookAndFeel(nullptr); }
 void paint(Graphics& g) override { g.fillAll(Colours::black); }
 void resized() override { surface.setTransform(AffineTransform::scale(float(getWidth()) / float(designW))); }

private:
 // ---- construction helpers -------------------------------------------------------------
 static std::vector<Rectangle<int>> split(Rectangle<int> area, std::initializer_list<float> weights) {
  float total = 0; for (float w : weights) total += w;
  std::vector<Rectangle<int>> cells; float x = float(area.getX());
  for (float w : weights) { const float width = float(area.getWidth()) * w / total; cells.emplace_back(roundToInt(x), area.getY(), roundToInt(width), area.getHeight()); x += width; }
  return cells;
 }
 void place(Component& component, Rectangle<int> bounds, int page) {
  if (&component != &keyboard) component.setWantsKeyboardFocus(false); // typing keys always go to the piano keyboard
  surface.addAndMakeVisible(component); component.setBounds(bounds); if (page >= 0) paged.push_back({&component, page}); }
 void caption(const String& text, Rectangle<int> bounds, int page, float size = 13.f, Colour colour = orbitui::ink) {
  auto* label = labels.add(new Label({}, text));
  label->setFont(orbitui::face(size)); label->setColour(Label::textColourId, colour); label->setJustificationType(Justification::centred);
  label->setBorderSize({}); label->setMinimumHorizontalScale(.6f); label->setInterceptsMouseClicks(false, false);
  place(*label, bounds.expanded(5, 0), page);
 }
 void setUpSlider(Slider& slider, const char* id, Slider::SliderStyle style) {
  auto* parameter = orbit.state.getParameter(id);
  slider.setSliderStyle(style); slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
  slider.setPopupDisplayEnabled(true, true, &surface); slider.setName(parameter->getName(60));
 }
 Fader& addControl(const char* id, Slider::SliderStyle style, Rectangle<int> bounds, int page) {
  auto* fader = faders.add(new Fader()); fader->id = id;
  setUpSlider(fader->slider, id, style); place(fader->slider, bounds, page);
  fader->attachment = std::make_unique<APVTS::SliderAttachment>(orbit.state, id, fader->slider);
  auto* parameter = orbit.state.getParameter(id);
  fader->slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
  return *fader;
 }
 Fader& addFader(const char* id, const char* text, int page, Rectangle<int> cell) {
  caption(text, cell.withHeight(16), page);
  return addControl(id, Slider::LinearVertical, Rectangle<int>(cell.getCentreX() - 20, cell.getY() + 28, 40, cell.getHeight() - 38), page);
 }
 void addKnob(const char* id, const char* text, Rectangle<int> cell) {
  caption(text, cell.withHeight(14), -1, 12.f);
  const int width = jmin(60, cell.getWidth());
  addControl(id, Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(cell.getCentreX() - width / 2, cell.getY() + 16, width, cell.getHeight() - 18), -1);
 }
 void addRowKnob(const char* id, const char* text, Rectangle<int> cell) {
  caption(text, cell.withHeight(16), 0);
  const int width = jmin(56, cell.getWidth());
  addControl(id, Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(cell.getCentreX() - width / 2, cell.getY() + 50, width, 56), 0);
 }
 void addChoice(const char* id, const char* text, const StringArray& names, int page, Rectangle<int> cell, bool vertical, Colour colour = orbitui::neutral) {
  auto* choice = choices.add(new ChoiceButtons(*orbit.state.getParameter(id), names, vertical, colour));
  if (vertical) { caption(text, cell.withHeight(16), page); place(*choice, Rectangle<int>(cell.getX() + 3, cell.getY() + 28, cell.getWidth() - 6, jmin(names.size() * 35, cell.getHeight() - 30)), page); }
  else { place(*choice, cell.withHeight(46).reduced(3, 0), page); caption(text, Rectangle<int>(cell.getX(), cell.getY() + 52, cell.getWidth(), 14), page, 11.f, orbitui::dim); }
 }
 void addWaveKnob(const char* id, const char* text, const StringArray& names, int page, Rectangle<int> cell, bool nameBelow = true) {
  caption(text, cell.withHeight(16), page);
  place(*waveKnobs.add(new WaveKnob(orbit.state, id, names, nameBelow)), {cell.getX(), cell.getY() + 26, cell.getWidth(), 104}, page);
 }
 // A small knob with its caption, for columns of the second row that stack two controls.
 void addMiniKnob(const char* id, const char* text, Rectangle<int> area) {
  caption(text, area.withHeight(14), 0, 12.f);
  const int width = jmin(50, area.getWidth());
  addControl(id, Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(area.getCentreX() - width / 2, area.getY() + 14, width, 48), 0);
 }
 void addToggle(const char* id, const String& text, int page, Rectangle<int> bounds, int led, Colour colour) {
  auto* toggle = toggles.add(new Toggle());
  toggle->button.setButtonText(text); toggle->button.setClickingTogglesState(true); toggle->button.getProperties().set("led", led); orbitui::tint(toggle->button, colour);
  place(toggle->button, bounds, page);
  toggle->attachment = std::make_unique<APVTS::ButtonAttachment>(orbit.state, id, toggle->button);
 }
 void useOwnFilterEnvelope() {
  auto* link = orbit.state.getParameter("envLink");
  if (link->getValue() > .5f) { link->beginChangeGesture(); link->setValueNotifyingHost(0.f); link->endChangeGesture(); }
 }
 void addButton(TextButton& button, const String& text, Rectangle<int> bounds, Colour colour, int led) {
  button.setButtonText(text); button.getProperties().set("led", led); orbitui::tint(button, colour); place(button, bounds, -1);
 }

 // ---- header: name, LED display, bank / patch memory, save / load, panel switch --------
 void buildHeader() {
  using namespace orbitui;
  refreshPresetMenu();
  presets.setSelectedId(orbit.getCurrentProgram() + 1, dontSendNotification); place(presets, {300, 48, 300, 36}, -1);
  presets.onChange = [this] {
   const int id = presets.getSelectedId();
   if (id >= 1000) { const int index = id - 1000; if (isPositiveAndBelow(index, userNames.size())) showResult(orbit.loadUserPreset(userNames[index])); }
   else if (id > 0) orbit.setCurrentProgram(id - 1);
  };
  for (int i = 0; i < 3; ++i) { auto* button = bankButtons.add(new TextButton()); addButton(*button, String::charToString(juce_wchar('A' + i)), {622 + i * 48, 36, 43, 48}, neutral, 1); button->onClick = [this, i] { shownBank = i; timerCallback(); }; }
  for (int i = 0; i < 8; ++i) { auto* button = patchButtons.add(new TextButton()); addButton(*button, String(i + 1), {782 + i * 48, 36, 43, 48}, i < 4 ? bright : neutral, 1); button->onClick = [this, i] { orbit.setCurrentProgram(shownBank * 8 + i); timerCallback(); }; }
  addButton(saveButton, "SAVE", {1294, 48, 70, 36}, amber, 0); saveButton.onClick = [this] { saveDialog(); };
  addButton(loadButton, "LOAD", {1370, 48, 70, 36}, neutral, 0);
  loadButton.onClick = [this] { refreshPresetMenu(); if (orbit.userPresetName().isNotEmpty()) showResult(orbit.loadUserPreset(orbit.userPresetName())); else orbit.setCurrentProgram(orbit.getCurrentProgram()); };
 }

// ---- first row: the playing faders ------------------------------------------------------
 void buildPanelA() {
  using namespace orbitui;
  Row row(*this, panelY, panelH, 0, 31.4f, 9);
  auto cells = split(row.next("LFO", neutral, 3), {1, 1, 1});
  addFader("rate", "RATE", 0, cells[0]); addFader("lfoDelay", "DELAY", 0, cells[1]); addFader("lfoPitch", "PITCH", 0, cells[2]);
  cells = split(row.next("OSC I", amber, 5), {1, 1, 1, 1, 1});
  addFader("wave", "SAW/PLS", 0, cells[0]); addFader("width", "WIDTH", 0, cells[1]); addFader("pwm", "PWM", 0, cells[2]); addFader("sub", "SUB", 0, cells[3]); addFader("noise", "NOISE", 0, cells[4]);
  cells = split(row.next("OSC II", amber, 3), {1, 1, 1});
  addFader("oscLayer", "LEVEL", 0, cells[0]); addFader("osc2Tune", "TUNE", 0, cells[1]); addFader("detune", "FINE", 0, cells[2]);
  addFader("hpf", "0 - 3", 0, row.next("HPF", teal, 1)).slider.getProperties().set("marks", 3);
  cells = split(row.next("FILTER", teal, 6.4f), {1, 1, 1, 1, 1, 1.4f});
  addFader("cutoff", "FREQ", 0, cells[0]); addFader("resonance", "RES", 0, cells[1]); addFader("env", "ENV", 0, cells[2]); addFader("lfoFilter", "LFO", 0, cells[3]); addFader("keytrack", "KYBD", 0, cells[4]);
  addChoice("slope", "SLOPE", {"12 dB", "24 dB"}, 0, cells[5], true);
  addFader("velocity", "VEL", 0, row.next("AMP", teal, 1));
  cells = split(row.next("ENV / AMP", neutral, 4), {1, 1, 1, 1});
  addFader("attack", "A", 0, cells[0]); addFader("decay", "D", 0, cells[1]); addFader("sustain", "S", 0, cells[2]); addFader("release", "R", 0, cells[3]);
  cells = split(row.next("ENV / FILTER", neutral, 5.4f), {1, 1, 1, 1, 1.4f});
  const char* ids[] = {"fattack", "fdecay", "fsustain", "frelease"}; const char* names[] = {"A", "D", "S", "R"};
  // SOURCE says which envelope drives the filter: its OWN four faders, or a copy of the AMP envelope.
  // Grabbing one of the four faders while AMP is selected switches to OWN, so they always respond.
  for (int i = 0; i < 4; ++i) { auto& fader = addFader(ids[i], names[i], 0, cells[size_t(i)]); fader.followsLink = true; fader.slider.onDragStart = [this] { useOwnFilterEnvelope(); }; }
  addChoice("envLink", "SOURCE", {"OWN", "AMP"}, 0, cells[4], true);
  cells = split(row.next("CHORUS", amber, 2.6f), {1, 1.6f});
  addFader("chorus", "MIX", 0, cells[0]); addChoice("chorusMode", "MODE", {"OFF", "I", "II", "I+II"}, 0, cells[1], true, bright);
 }

 // ---- second row: oscillator shapes, modulation, amplifier mode, second filter and routing ----
 void buildRowTwo() {
  using namespace orbitui;
  const StringArray shapes{"MIX", "SAW", "PULSE", "TRI", "SINE"}, octaves{"-2", "-1", "0", "+1", "+2"}, models{"LADDER", "SVF"};
  Row row(*this, row2Y, row2H, 0, 36.8f, 7);
  auto cells = split(row.next("OSC I / SHAPE", amber, 5.7f), {2.2f, 2.2f, 1.3f});
  addWaveKnob("osc1Shape", "WAVE", shapes, 0, cells[0]); addWaveKnob("osc1Octave", "OCTAVE", octaves, 0, cells[1], false);
  addMiniKnob("osc1Level", "LEVEL", cells[2].withHeight(64)); addMiniKnob("crossMod", "X-MOD", cells[2].withTrimmedTop(68).withHeight(64));
  cells = split(row.next("OSC II / SHAPE", amber, 5.7f), {2.2f, 2.2f, 1.3f});
  addWaveKnob("osc2Shape", "WAVE", shapes, 0, cells[0]); addWaveKnob("osc2Octave", "OCTAVE", octaves, 0, cells[1], false);
  addMiniKnob("osc2Width", "WIDTH", cells[2].withHeight(64)); caption("TO OSC I", cells[2].withTrimmedTop(68).withHeight(14), 0, 11.f, dim);
  addToggle("oscSync", "SYNC", 0, {cells[2].getX() + 1, cells[2].getY() + 86, cells[2].getWidth() - 2, 30}, 2, neutral);
  cells = split(row.next("LFO / MODULATION", neutral, 6.4f), {2.2f, 1.6f, 1.3f, 1.3f});
  addWaveKnob("lfoShape", "WAVE", {"TRI", "SINE", "SAW", "SQUARE"}, 0, cells[0]); addChoice("pwmSource", "PWM BY", {"LFO", "MAN", "ENV"}, 0, cells[1], true);
  addRowKnob("envPitch", "ENV>PITCH", cells[2]); addRowKnob("wheelDepth", "WHEEL", cells[3]);
  cells = split(row.next("PLAY / AMP", neutral, 3.6f), {1, 1});
  addChoice("legato", "ENVELOPE", {"RETRIG", "LEGATO"}, 0, cells[0], true); addChoice("vcaMode", "VCA", {"ENV", "GATE"}, 0, cells[1], true);
  addToggle("hold", "HOLD", 0, {cells[0].getX() + 3, cells[0].getY() + 102, cells[0].getWidth() - 6, 28}, 2, teal);
  cells = split(row.next("FILTER A", teal, 4.7f), {1.8f, 1.6f, 1.3f});
  addChoice("filterAType", "MODEL", models, 0, cells[0], true); addChoice("envInvert", "ENV", {"NORM", "INV"}, 0, cells[1], true); addRowKnob("bendFilter", "BEND", cells[2]);
  cells = split(row.next("FILTER B", teal, 7.2f), {1.8f, 1.5f, 1.3f, 1.3f, 1.3f});
  addChoice("filterBType", "MODEL", models, 0, cells[0], true); addChoice("slopeB", "SLOPE", {"12 dB", "24 dB"}, 0, cells[1], true);
  addRowKnob("cutoffB", "FREQ", cells[2]); addRowKnob("resonanceB", "RES", cells[3]); addRowKnob("envB", "ENV", cells[4]);
  cells = split(row.next("ROUTING", teal, 3.5f), {2.2f, 1.3f});
  addWaveKnob("filterRoute", "PATH", {"A", "B", "A>B", "B>A", "PAR"}, 0, cells[0], false); addRowKnob("filterBlend", "A/B MIX", cells[1]);
 }

 // ---- lower row: voices, glide, arpeggio, effects, output (always visible) ----------------
 void buildLowerRow() {
  using namespace orbitui;
  Row row(*this, lowerY, lowerH, -1, 32.4f, 7);
  auto cells = split(row.next("VOICES", neutral, 5.6f), {2, 3});
  addChoice("mode", "POLYPHONY", {"6", "8"}, -1, cells[0], false); addChoice("voiceMode", "ASSIGN", {"POLY", "MONO", "UNI"}, -1, cells[1], false, bright);
  cells = split(row.next("GLIDE / BEND", neutral, 3.6f), {1, 1, 1});
  addKnob("portamento", "PORTA", cells[0]); addKnob("bendRange", "BEND", cells[1]); addKnob("unisonSpread", "SPREAD", cells[2]);
  cells = split(row.next("ARPEGGIO", teal, 12), {1, 1, 4, 3, 2.4f, 1.3f, 1.3f});
  addToggle("arpOn", "ON", -1, cells[0].withHeight(46).reduced(3, 0), 1, teal); addToggle("arpSync", "SYNC", -1, cells[1].withHeight(46).reduced(3, 0), 1, neutral);
  caption("HOST", {cells[1].getX(), cells[1].getY() + 52, cells[1].getWidth(), 14}, -1, 11.f, dim);
  addChoice("arpPattern", "MODE", {"UP", "DOWN", "U&D", "RND"}, -1, cells[2], false); addChoice("arpDivision", "STEP", {"1/4", "1/8", "1/16"}, -1, cells[3], false, bright); addChoice("arpOctaves", "OCTAVES", {"1", "2", "3"}, -1, cells[4], false);
  addKnob("arpTempo", "TEMPO", cells[5]); addKnob("arpGate", "GATE", cells[6]);
  addKnob("fxDrive", "AMOUNT", row.next("DRIVE", amber, 1.3f));
  cells = split(row.next("DELAY", amber, 3.6f), {1, 1, 1});
  addKnob("delayTime", "TIME", cells[0]); addKnob("delayFeedback", "FDBK", cells[1]); addKnob("delayMix", "LEVEL", cells[2]);
  cells = split(row.next("REVERB", amber, 5.f), {1.5f, 1.17f, 1.17f, 1.17f});
  place(*choices.add(new ChoiceButtons(*orbit.state.getParameter("reverbType"), {"VINT", "PLATE"}, true, bright)), {cells[0].getX() + 2, cells[0].getY() + 6, cells[0].getWidth() - 4, 65}, -1);
  addKnob("reverbSize", "SIZE", cells[1]); addKnob("reverbDamp", "DAMP", cells[2]); addKnob("reverbMix", "LEVEL", cells[3]);
  addKnob("gain", "VOLUME", row.next("OUTPUT", neutral, 1.3f));
 }

 void buildKeyboard() {
  using namespace orbitui;
  caption("KEYBOARD", {left, keysY + 28, 132, 16}, -1);
  addButton(octDown, "OCT -", {left + 10, keysY + 56, 53, 40}, neutral, 0); addButton(octUp, "OCT +", {left + 69, keysY + 56, 53, 40}, neutral, 0);
  octDown.onClick = [this] { octave = jmax(0, octave - 1); updateKeyboard(); }; octUp.onClick = [this] { octave = jmin(4, octave + 1); updateKeyboard(); };
  keyboard.setScrollButtonsVisible(false); keyboard.setKeyWidth(1254.f / 36.f); keyboard.setOctaveForMiddleC(4); keyboard.setBlackNoteLengthProportion(.62f);
  place(keyboard, {left + 146, keysY + 6, 1254, keysH - 6}, -1);
  // Computer keyboard, AZERTY rows: Q S D F G H J K L M are the white keys starting on C,
  // Z E T Y U O P the black keys above them. OCT - / OCT + move the range.
  keyboard.clearKeyMappings();
  const char* keys = "qzsedftgyhujkolpm";
  for (int i = 0; keys[i] != 0; ++i) keyboard.setKeyPressForNote(KeyPress(keys[i], 0, 0), i);
  updateKeyboard();
 }
 void updateKeyboard() { keyboard.setAvailableRange(12 * (octave + 1), 12 * (octave + 1) + 60); keyboard.setKeyPressBaseOctave(octave + 1); surface.repaint(); }
 void setPage(int index) { page = index; for (auto& entry : paged) entry.first->setVisible(entry.second == page); surface.repaint(); }

 // ---- painting -------------------------------------------------------------------------
 void paintSurface(Graphics& g) {
  using namespace orbitui;
  g.fillAll(panel);
  g.setColour(Colour(0xff3d4043)); g.drawHorizontalLine(0, 20, designW - 20);
  for (int x : {0, designW - 20}) { ColourGradient cheek(Colour(0xff17181a), float(x), 0, Colour(0xff17181a), float(x + 20), 0, false); cheek.addColour(.45, Colour(0xff2d2f32)); g.setGradientFill(cheek); g.fillRect(x, 0, 20, int(designH)); }
  g.setColour(ink); g.setFont(Font(FontOptions("Avenir Next Condensed", 52.f, Font::bold | Font::italic)).withExtraKerningFactor(.06f)); g.drawText("ORBIT", left, headerY + 2, 250, 50, Justification::centredLeft);
  g.setColour(dim); g.setFont(face(11.f, false).withExtraKerningFactor(.2f)); g.drawText("PROGRAMMABLE POLYPHONIC SYNTHESIZER", left + 2, headerY + 56, 250, 14, Justification::centredLeft);
  const Rectangle<float> window(300, 22, 300, 64);
  g.setColour(Colour(0xff0a0b0b)); g.fillRoundedRectangle(window, 3); g.setColour(Colour(0xff3a3d40)); g.drawRoundedRectangle(window.reduced(.5f), 3, 1);
  g.setColour(led.withAlpha(.75f)); g.setFont(mono(11.f)); g.drawText(displayLine, 314, 28, 276, 16, Justification::centredLeft);
  g.setColour(dim); g.setFont(face(11.f)); g.drawText("BANK", 622, 20, 139, 14, Justification::centred); g.drawText("PATCH", 782, 20, 379, 14, Justification::centred);
  g.setColour(amber); g.fillRect(left, int(lineY), contentW / 2, 3); g.setColour(teal); g.fillRect(left + contentW / 2, int(lineY), contentW / 2, 3);
  g.setFont(face(14.f).withExtraKerningFactor(.14f));
  for (const auto& section : sections) {
   if (section.page >= 0 && section.page != page) continue;
   const auto bar = section.area.withHeight(22);
   g.setColour(section.colour); g.fillRect(bar); g.setColour(print); g.drawFittedText(section.title, bar.reduced(2, 0), Justification::centred, 1, .6f);
  }
  g.setColour(Colour(0xff1a1b1d)); g.fillRoundedRectangle(float(left), float(keysY + 6), 132, float(keysH - 6), 3);
  g.setColour(dim); g.setFont(face(12.f, false)); g.drawText("C" + String(octave) + " - C" + String(octave + 5), left, keysY + 106, 132, 16, Justification::centred);
  g.setColour(slot); g.fillRect(left + 146, int(keysY), 1254, 6);
 }

 // ---- presets ---------------------------------------------------------------------------
 void refreshPresetMenu() {
  presets.clear(dontSendNotification); presets.addSectionHeading("FACTORY");
  for (int i = 0; i < orbit.getNumPrograms(); ++i) presets.addItem(orbit.getProgramName(i), i + 1);
  userNames = orbit.userPresetNames();
  if (!userNames.isEmpty()) { presets.addSeparator(); presets.addSectionHeading("USER"); for (int i = 0; i < userNames.size(); ++i) presets.addItem("USER / " + userNames[i], 1000 + i); }
 }
 void showResult(const Result& result) { if (result.failed()) AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Preset", result.getErrorMessage()); }
 void saveDialog() {
  using orbitui::utf8;
  auto* dialog = new AlertWindow("Enregistrer un preset", utf8("Enregistrement dans Application Support/SF Instruments/Orbit 106. Un nom existant est conservé."), MessageBoxIconType::NoIcon);
  dialog->addTextEditor("name", orbit.userPresetName(), "Nom du preset");
  dialog->addButton("Enregistrer", 1, KeyPress(KeyPress::returnKey)); dialog->addButton("Annuler", 0, KeyPress(KeyPress::escapeKey));
  Component::SafePointer<Editor> safe(this);
  dialog->enterModalState(true, ModalCallbackFunction::create([safe, dialog](int result) {
   if (result == 1 && safe) { const auto status = safe->orbit.saveUserPreset(dialog->getTextEditorContents("name").trim()); safe->showResult(status); if (status.wasOk()) safe->refreshPresetMenu(); }
  }), true);
 }

 // Follows state the host or a preset can change behind the panel's back.
 void timerCallback() override {
  const int program = orbit.getCurrentProgram(); const auto userName = orbit.userPresetName(); const bool user = userName.isNotEmpty();
  if (program != lastProgram) { lastProgram = program; shownBank = program / 8; }
  for (int i = 0; i < patchButtons.size(); ++i) patchButtons[i]->setToggleState(!user && shownBank * 8 + i == program, dontSendNotification);
  for (int i = 0; i < bankButtons.size(); ++i) bankButtons[i]->setToggleState(i == shownBank, dontSendNotification);
  if (!user) presets.setSelectedId(program + 1, dontSendNotification);
  else if (const int index = userNames.indexOf(userName); index >= 0) presets.setSelectedId(1000 + index, dontSendNotification);
  else presets.setText("USER / " + userName, dontSendNotification);
  const bool eight = orbit.state.getRawParameterValue("mode")->load() > .5f;
  const String line = (user ? String("USER") : "BANK " + String::charToString(juce_wchar('A' + program / 8)) + " / " + String(program % 8 + 1)) + (eight ? "    8 VOICES" : "    6 VOICES") + (orbit.isPatchModified() ? "    EDITED" : "");
  if (line != displayLine) { displayLine = line; surface.repaint(300, 22, 300, 26); }
  // Standalone only: keep the typing keys on the piano keyboard. Inside a DAW the host keeps its own shortcuts.
  if (orbit.wrapperType == AudioProcessor::wrapperType_Standalone && isShowing() && Component::getCurrentlyFocusedComponent() == nullptr && Component::getCurrentlyModalComponent() == nullptr) keyboard.grabKeyboardFocus();
  const bool linked = orbit.state.getRawParameterValue("envLink")->load() > .5f;
  for (auto* fader : faders) if (fader->followsLink) fader->slider.setAlpha(linked ? .4f : 1.f); // dimmed, but still grabbable
 }

 Orbit& orbit; OrbitLook look; Surface surface; PianoKeyboard keyboard;
 std::vector<Section> sections; std::vector<std::pair<Component*, int>> paged;
 OwnedArray<Label> labels; OwnedArray<Fader> faders; OwnedArray<Toggle> toggles; OwnedArray<ChoiceButtons> choices; OwnedArray<WaveKnob> waveKnobs;
 OwnedArray<TextButton> bankButtons, patchButtons;
 ComboBox presets; TextButton saveButton, loadButton, octDown, octUp;
 StringArray userNames; String displayLine;
 int page = 0, octave = 2, shownBank = 0, lastProgram = -1;
};
