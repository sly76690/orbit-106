#pragma once
#include <BinaryData.h>
// Orbit front panel: an original layout in the spirit of early-80s programmable polysynths.
// The vector panel uses a fixed design surface, scaled as a whole
// when the plugin window is resized. Everything is on one face: a row of faders for the playing
// controls, and under it a row of knobs and switches for shapes, modulation and the second filter.

namespace orbitui {
 const Colour panel{0xff202329}, ink{0xffe3ded2}, dim{0xffb1b1aa}, amber{0xffe58b46}, teal{0xff438c9d};
 const Colour neutral{0xffd9d3c3}, bright{0xffe6e0d1}, led{0xffedac62}, slot{0xff0b0c0d}, print{0xff141516};
 inline Font face(float size, bool bold = false) { return Font(FontOptions("DIN Condensed", size, bold ? Font::bold : Font::plain)); }
 inline Font legend(float size) { return Font(FontOptions("Helvetica Neue", size, Font::plain)); }
 inline Font mono(float size) { return Font(FontOptions("Menlo", size, Font::plain)); }
 inline String utf8(const char* text) { return String::fromUTF8(text); }
 inline void tint(TextButton& button, Colour colour) { button.setColour(TextButton::buttonColourId, colour); button.setColour(TextButton::buttonOnColourId, colour); }
 // "led" property on a button: 0 = plain cap, 1 = LED above the cap, 2 = LED left of the cap.
 inline int ledMode(Button& button) { return int(button.getProperties().getWithDefault("led", 0)); }
}

// Rendered artwork ("skin"). When every image is present in the Skin folder, the editor shows the rendered
// panel and controls instead of drawing them; when one is missing it falls back to the drawn interface.
// The images are produced by tools/orbit_render.py from the layout file the editor writes in the same folder.
struct OrbitSkin {
 Image background, knob, fader, led; std::array<Image, 3> index; // index line in amber, teal, neutral
 bool ready = false;
 static File folder() { return Orbit::presetRoot().getChildFile("Skin"); }
 // Where the installer puts the artwork for every user of the computer (/Library/Application Support/... on macOS).
 static File sharedFolder() { return File::getSpecialLocation(File::commonApplicationDataDirectory).getChildFile("Application Support").getChildFile("SF Instruments").getChildFile("Orbit 106").getChildFile("Skin"); }
 void load(const File& directory) {
  const auto get = [&directory](const char* name) { return ImageFileFormat::loadFrom(directory.getChildFile(name)); };
  background = get("background.jpg"); knob = get("knob.png"); fader = get("fader.png"); led = get("led.png");
  index[0] = get("index_amber.png"); index[1] = get("index_teal.png"); index[2] = get("index_neutral.png");
  ready = background.isValid() && knob.isValid() && fader.isValid() && led.isValid() && index[0].isValid() && index[1].isValid() && index[2].isValid();
 }
 const Image& indexFor(Colour colour) const { return colour == orbitui::amber ? index[0] : (colour == orbitui::teal ? index[1] : index[2]); }
 // Sizes of the sprites, in design pixels: the knob image is this many times the knob's diameter (room for
 // its shadow); the fader cap and the lit LED are drawn in squares of these sides.
 static constexpr float knobFrame = 1.6f, faderFrame = 48.f, ledFrame = 28.f;
 // The artwork is rendered from slightly in front of the panel, not straight above (TILT in orbit_render.py),
 // so the top of a knob is seen as a flattened circle, raised above the knob's centre.
 static constexpr float tiltCos = 1.f;      // cos(TILT): 1 = artwork seen from straight above
 static constexpr float topLift = 0.f;      // top of the knob sits on its centre when seen from above
};

// The approved artwork is the material atlas. Coordinates are in its native 1568x1003 space.
// Only material patches are sampled; all text, hit regions and parameter bindings remain live.
struct ReferenceMaterials {
 Image atlas = ImageCache::getFromMemory(BinaryData::referenceapproved_png, BinaryData::referenceapproved_pngSize);
 Image knob;
 ReferenceMaterials() {
  knob = Image(Image::ARGB, 49, 51, true);
  // Remove the fixed twelve-o'clock index with adjacent cap material, then draw the live index.
  { Graphics g(knob); g.drawImage(atlas, 0, 0, 49, 51, 139, 387, 49, 51); }
  for (int y = 0; y < 25; ++y) for (int x = 21; x <= 27; ++x)
   knob.setPixelAt(x, y, knob.getPixelAt(16, y).interpolatedWith(knob.getPixelAt(32, y), .5f));
  // Discard surrounding panel pixels so the atlas does not produce a rectangular tile.
  for (int y = 0; y < knob.getHeight(); ++y) for (int x = 0; x < knob.getWidth(); ++x) {
   const float dx = float(x) - 24.f, dy = float(y) - 25.f;
   const float alpha = jlimit(0.f, 1.f, 22.5f - std::sqrt(dx * dx + dy * dy));
   knob.setPixelAt(x, y, knob.getPixelAt(x, y).withMultipliedAlpha(alpha));
  }
 }
 void patch(Graphics& g, Rectangle<float> target, Rectangle<int> source) const {
  g.setOpacity(1.f); g.drawImage(atlas, roundToInt(target.getX()), roundToInt(target.getY()), roundToInt(target.getWidth()), roundToInt(target.getHeight()), source.getX(), source.getY(), source.getWidth(), source.getHeight());
 }
};

class OrbitLook : public LookAndFeel_V4 {
public:
 ReferenceMaterials materials;
 OrbitLook() {
  using namespace orbitui;
  setColour(Label::textColourId, ink);
  setColour(TextButton::buttonColourId, neutral); setColour(TextButton::buttonOnColourId, neutral);
  setColour(ComboBox::backgroundColourId, Colours::transparentBlack); setColour(ComboBox::outlineColourId, Colours::transparentBlack);
  setColour(ComboBox::textColourId, led); setColour(ComboBox::arrowColourId, led);
  setColour(PopupMenu::backgroundColourId, Colour(0xff17181a)); setColour(PopupMenu::textColourId, ink); setColour(PopupMenu::headerTextColourId, dim);
  setColour(PopupMenu::highlightedBackgroundColourId, amber); setColour(PopupMenu::highlightedTextColourId, print);
 }
 const OrbitSkin* skin = nullptr;
 bool skinned() const { return skin != nullptr && skin->ready; }
 Font getTextButtonFont(TextButton&, int height) override { return orbitui::face(jlimit(12.f, 17.f, height * .72f)); }
 Font getComboBoxFont(ComboBox&) override { return orbitui::mono(19.f); }
 Font getPopupMenuFont() override { return orbitui::face(16.f, false); }

 void drawButtonBackground(Graphics& g, Button& button, const Colour& colour, bool over, bool down) override {
  using namespace orbitui;
  auto area = button.getLocalBounds().toFloat();
  if (skinned()) { // the cap, its legend and the unlit LED are in the background picture; only what changes is drawn
   if (const int mode = ledMode(button); mode != 0) {
    const auto spot = (mode == 1 ? area.removeFromTop(12) : area.removeFromLeft(14)).getCentre();
    if (button.getToggleState()) g.drawImage(skin->led, Rectangle<float>(OrbitSkin::ledFrame, OrbitSkin::ledFrame).withCentre(spot));
   }
   const auto cap = area.reduced(1).withTrimmedBottom(3);
   const bool cream = button.getProperties().contains("cream");
   if (button.getToggleState() && !cream && ledMode(button) == 0) { // dark cap: the light bar set in its top edge comes on
    const Colour glow(0xffffa42a);
    const auto bar = Rectangle<float>(cap.getWidth() * .44f, 3.f).withCentre({cap.getCentreX(), cap.getY() + 5.5f});
    for (int i = 1; i <= 5; ++i) { g.setColour(glow.withAlpha(.075f)); g.fillRoundedRectangle(bar.expanded(float(i) * 1.7f), float(i) * 1.7f); }
    g.setColour(Colour(0xffffb84a)); g.fillRoundedRectangle(bar, 1.5f);
    g.setColour(Colours::white.withAlpha(.55f)); g.fillRoundedRectangle(bar.reduced(2.f, .8f), 1.f);
   }
   if (button.getToggleState() && cream) { // a selected cap is lit from inside: amber over the rendered cap, and a soft glow around it
    const Colour lit(0xffffa42a);
    for (int i = 1; i <= 4; ++i) { g.setColour(lit.withAlpha(.045f)); g.fillRoundedRectangle(cap.expanded(float(i) * 1.5f), 3.f + float(i)); }
    g.setColour(lit.withAlpha(.82f)); g.fillRoundedRectangle(cap.reduced(.5f), 3);
    g.setColour(Colours::white.withAlpha(.18f)); g.fillRoundedRectangle(cap.reduced(2).withHeight(cap.getHeight() * .4f), 2);
   }
   if (down) { g.setColour(Colours::black.withAlpha(.28f)); g.fillRoundedRectangle(cap, 3); }
   else if (over) { g.setColour(Colours::white.withAlpha(.07f)); g.fillRoundedRectangle(cap, 3); }
   ignoreUnused(colour); return;
  }
  if (const int mode = ledMode(button); mode != 0) {
   auto spot = (mode == 1 ? area.removeFromTop(12) : area.removeFromLeft(14)).withSizeKeepingCentre(7, 7);
   const bool on = button.getToggleState();
   if (on) {
    for (int i = 5; i > 0; --i) { g.setColour(led.withAlpha(.035f)); g.fillEllipse(spot.expanded(float(i))); }
   }
   g.setColour(on ? led : Colour(0xff302b21)); g.fillEllipse(spot);
   if (on) { g.setColour(Colour(0xffffe9b5)); g.fillEllipse(spot.reduced(2)); }
  }
  const auto well = area.reduced(1).withTrimmedBottom(2);
  const auto cap = down ? well.translated(0, 1) : well;
  const int sourceX = button.getToggleState() ? 854 : 899;
  materials.patch(g, cap, {sourceX, 33, 40, 31});
  // Replace the atlas legend with unprinted material from the same button.
  materials.patch(g, cap.withTrimmedLeft(cap.getWidth() * .22f).withTrimmedRight(cap.getWidth() * .22f).reduced(0, 4), {sourceX + 5, 39, 4, 18});
  if (over) { g.setColour(Colours::white.withAlpha(.08f)); g.fillRoundedRectangle(cap, 2); }
  if (!button.isEnabled()) { g.setColour(panel.withAlpha(.45f)); g.fillRoundedRectangle(cap, 2); }
  ignoreUnused(colour);

 }
 void drawButtonText(Graphics& g, TextButton& button, bool, bool down) override {
  const bool creamCap = button.getProperties().contains("cream");
  if (skinned() && button.getProperties().contains("legendAbove")) return; // its legend is printed on the panel, above the key
  if (skinned() && !button.getProperties().contains("liveText") && !(button.getToggleState() && creamCap)) return;
  if (skinned() && button.getProperties().contains("liveText")) { // a small display window: amber characters on dark glass
   g.setFont(orbitui::mono(12.f)); g.setColour(orbitui::led.withAlpha(button.isEnabled() ? .95f : .4f));
   g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 1), Justification::centred, 1, .8f); return;
  } // legend already printed on the rendered cap (a lit cap gets its legend redrawn)
  auto area = button.getLocalBounds();
  if (const int mode = orbitui::ledMode(button); mode == 1) area.removeFromTop(12); else if (mode == 2) area.removeFromLeft(14);
  area = area.reduced(3, 1).withTrimmedBottom(3);
  if (down) area.translate(0, 2);
  g.setFont(getTextButtonFont(button, area.getHeight()));
  g.setColour((skinned() && !creamCap ? orbitui::ink : orbitui::print).withAlpha(button.isEnabled() ? 1.f : .5f)); // light legend on a dark rendered cap
  g.drawFittedText(button.getButtonText(), area, Justification::centred, 1, .7f);
 }

 // Thin fader: dark slot, tick marks on both sides, small light cap.
 void drawLinearSlider(Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider& slider) override {
  using namespace orbitui;
  if (skinned()) { g.drawImage(skin->fader, Rectangle<float>(OrbitSkin::faderFrame, OrbitSkin::faderFrame).withCentre({float(x) + float(w) * .5f, pos})); ignoreUnused(y, h, slider); return; }
  const float cx = float(x) + float(w) * .5f, top = float(y), length = float(h);
  g.setColour(dim.withAlpha(slider.isEnabled() ? .8f : .3f));
  const int marks = jmax(1, int(slider.getProperties().getWithDefault("marks", 10))); // a switch-like fader shows one mark per stop
  for (int i = 0; i <= marks; ++i) {
   const float yy = top + length * float(i) / float(marks), mark = (marks != 10 || i % 5 == 0) ? 9.f : 6.f;
   g.fillRect(cx - 7 - mark, yy - .4f, mark, .8f); g.fillRect(cx + 7, yy - .4f, mark, .8f);
  }
  g.setColour(slot); g.fillRoundedRectangle(cx - 3, top - 7, 6, length + 14, 3);
  g.setColour(Colour(0xff3a3d40)); g.drawVerticalLine(int(cx) + 3, top - 5, top + length + 5);
  const Rectangle<float> cap(cx - 13, pos - 9, 26, 20);
  materials.patch(g, cap, {44, 208, 32, 25});
  if (!slider.isEnabled()) { g.setColour(panel.withAlpha(.5f)); g.fillRect(cap); }

 }
 // Orbit's knob: a dark skirt with a fine bright rim, a raised cap lit from above, and an index line in the
 // colour of the section the knob belongs to, so the eye finds the setting and the section at once.
 void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float value, float start, float end, Slider& slider) override {
  using namespace orbitui;
  auto area = Rectangle<float>(float(x), float(y), float(w), float(h));
  const bool selector = slider.getProperties().contains("selector");
  const float diameter = jmin(float(slider.getProperties().getWithDefault("faceDiameter", selector ? 38.f : 36.f)),
                              jmin(area.getWidth(), area.getHeight()) - 8.f);
  area = area.withSizeKeepingCentre(diameter, diameter);
  const auto centre = area.getCentre(); const float angle = start + value * (end - start), outer = diameter * .5f;
  const auto accent = slider.findColour(Slider::rotarySliderFillColourId);
  if (skinned()) { // rendered body with its shadow, then the index line turned to the current value
   g.drawImage(skin->knob, area.withSizeKeepingCentre(diameter * OrbitSkin::knobFrame, diameter * OrbitSkin::knobFrame));
   const auto& line = skin->indexFor(accent); const float scale = diameter * OrbitSkin::knobFrame / float(line.getWidth());
   g.drawImageTransformed(line, AffineTransform::translation(-float(line.getWidth()) * .5f, -float(line.getHeight()) * .5f).scaled(scale).rotated(angle).scaled(1.f, OrbitSkin::tiltCos).translated(centre.x, centre.y - diameter * OrbitSkin::topLift));
   ignoreUnused(outer); return;
  }
  const auto point = [centre, angle](float radius) { return Point<float>(centre.x + std::sin(angle) * radius, centre.y - std::cos(angle) * radius); };
  g.setImageResamplingQuality(Graphics::highResamplingQuality);
  for (int i = 4; i > 0; --i) { g.setColour(Colours::black.withAlpha(.06f)); g.fillEllipse(area.translated(1, 3).expanded(float(i))); }
  g.setOpacity(1.f); g.drawImage(materials.knob, area.expanded(diameter * .08f));
  const auto from = point(outer * .42f), to = point(outer * .87f);
  g.setColour(Colours::white.withAlpha(.4f)); g.drawLine(from.x + .7f, from.y, to.x + .7f, to.y, jmax(1.f, diameter * .035f));
  g.setColour(print); g.drawLine(from.x, from.y, to.x, to.y, jmax(1.2f, diameter * .035f));                               // index line, section colour
  if (!slider.isEnabled()) { g.setColour(panel.withAlpha(.5f)); g.fillEllipse(area.expanded(1)); }
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
 const OrbitSkin* skin = nullptr;
 bool skinned() const { return skin != nullptr && skin->ready; }
 // With the rendered keyboard the keys are in the background picture: only a held or hovered key is tinted.
 void tint(Graphics& g, int note, Rectangle<float> r, bool down, bool over, bool black) {
  if (!down && !over) return;
  Graphics::ScopedSaveState state(g);
  if (!black) for (int neighbour : {note - 1, note + 1}) if (MidiMessage::isMidiNoteBlack(neighbour)) g.excludeClipRegion(getRectangleForKey(neighbour).getSmallestIntegerContainer()); // not over the black keys
  g.setColour(down ? orbitui::amber.withAlpha(black ? .55f : .42f) : Colours::white.withAlpha(black ? .10f : .16f));
  g.fillRoundedRectangle(r.reduced(1.f, 0.f), 2.f);
 }
 void drawWhiteNote(int note, Graphics& g, Rectangle<float> r, bool down, bool over, Colour, Colour) override {
  if (skinned()) { tint(g, note, r, down, over, false); return; }
  auto key = r.reduced(.6f, 0);
  const auto ivory = down ? Colour(0xffe4bb7c) : Colour(0xffe8e1d0);
  ColourGradient material(Colour(0xff9b968a), key.getX(), key.getY(), ivory, key.getCentreX(), key.getBottom(), false);
  material.addColour(.15, ivory.brighter(.08f)); material.addColour(.92, ivory);
  material.addColour(.96, ivory.darker(.15f));
  g.setGradientFill(material); g.fillRoundedRectangle(key, 2);
  g.setColour(Colours::black.withAlpha(.65f)); g.drawRoundedRectangle(key, 2, .6f);
  g.setColour(Colours::white.withAlpha(.3f)); g.drawVerticalLine(int(key.getX()) + 1, key.getY() + 12, key.getBottom() - 8);
  if (over && !down) { g.setColour(Colours::white.withAlpha(.12f)); g.fillRoundedRectangle(key, 2); }

 }
 void drawBlackNote(int note, Graphics& g, Rectangle<float> r, bool down, bool over, Colour) override {
  if (skinned()) { tint(g, note, r, down, over, true); return; }
  for (int i = 4; i > 0; --i) { g.setColour(Colours::black.withAlpha(.09f)); g.fillRoundedRectangle(r.translated(2, 4).expanded(float(i)), 2); }
  g.setColour(Colour(0xff090a0a)); g.fillRoundedRectangle(r, 2);
  auto cap = r.reduced(2, 1).withTrimmedBottom(down ? 3.f : 7.f);
  ColourGradient material(Colour(0xff3e4140), cap.getX(), cap.getY(), Colour(0xff171919), cap.getRight(), cap.getY(), false);
  material.addColour(.2, Colour(0xff292d2c)); material.addColour(.85, Colour(0xff161818));
  g.setGradientFill(material); g.fillRoundedRectangle(cap, 2);
  g.setColour(Colours::white.withAlpha(.15f)); g.drawVerticalLine(int(cap.getX()) + 1, cap.getY() + 3, cap.getBottom() - 3);
  if (down || over) { g.setColour(down ? orbitui::amber.withAlpha(.4f) : Colours::white.withAlpha(.1f)); g.fillRoundedRectangle(cap, 2); }

 }
};

// Row or column of latching buttons with LEDs, bound to one choice or bool parameter.
class ChoiceButtons : public Component {
public:
 ChoiceButtons(RangedAudioParameter& parameter, const StringArray& names, bool vertical, Colour colour)
  : column(vertical), attachment(parameter, [this](float value) { for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState(i == roundToInt(value), dontSendNotification); }, nullptr) {
  for (int i = 0; i < names.size(); ++i) {
   auto* button = buttons.add(new TextButton(names[i]));
   button->getProperties().set("led", 0); orbitui::tint(*button, colour); button->setWantsKeyboardFocus(false);
   button->onClick = [this, i] { attachment.setValueAsCompleteGesture(float(i)); };
   addAndMakeVisible(button);
  }
  attachment.sendInitialUpdate();
 }
 void resized() override {
  const int count = buttons.size(), gap = 5; if (count == 0) return;
  if (column) { const int each = jmin(30, (getHeight() - gap * (count - 1)) / count); for (int i = 0; i < count; ++i) buttons[i]->setBounds(0, i * (each + gap), getWidth(), each); }
  else { const int each = (getWidth() - gap * (count - 1)) / count, top = ledOnTop ? 0 : 12; for (int i = 0; i < count; ++i) buttons[i]->setBounds(i * (each + gap), top, each, jmax(20, getHeight() - top)); }
 }
 // Plain coloured caps with an LED above each key and the legend printed on the panel above it,
 // instead of the dark caps with a light bar.
 void useColouredCaps(Colour colour) {
  ledOnTop = true;
  for (auto* button : buttons) { button->getProperties().set("led", 1); button->getProperties().set("cream", true); button->getProperties().set("legendAbove", true); orbitui::tint(*button, colour); }
  resized();
 }
private:
 bool column, ledOnTop = false; OwnedArray<TextButton> buttons; ParameterAttachment attachment;
};

// Two-position slide switch bound to a choice parameter: a short fader with one stop at each end and the two
// names printed beside the stops. The first choice is the upper position. Click a name or drag the cap.
class SlideSwitch : public Component {
public:
 SlideSwitch(RangedAudioParameter& parameter, const StringArray& names)
  : last(jmax(1, names.size() - 1)), attachment(parameter, [this](float value) { slider.setValue(double(last - roundToInt(value)), dontSendNotification); }, nullptr) {
  slider.setSliderStyle(Slider::LinearVertical); slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0); slider.setRange(0.0, double(last), 1.0);
  slider.getProperties().set("marks", last); slider.setWantsKeyboardFocus(false); slider.setName(parameter.getName(60));
  slider.onValueChange = [this] { attachment.setValueAsCompleteGesture(float(last - roundToInt(slider.getValue()))); };
  addAndMakeVisible(slider);
  for (int i = 0; i <= last; ++i) {
   auto* text = legends.add(new Label({}, names[i]));
   text->setFont(orbitui::face(15.f)); text->setColour(Label::textColourId, orbitui::ink);
   text->setJustificationType(Justification::centredLeft); text->setInterceptsMouseClicks(false, false); text->setBorderSize({});
   addAndMakeVisible(text);
  }
  attachment.sendInitialUpdate();
 }
 void resized() override { // the cap travels from 12 px below the top to 12 px above the bottom of the slider
  slider.setBounds(getWidth() / 2 - 38, 0, 28, getHeight());
  const int x = slider.getRight() + 8, width = getWidth() - x;
  for (int i = 0; i <= last; ++i) legends[i]->setBounds(x, 4 + (getHeight() - 24) * i / last, width, 16); // one name level with each stop
 }
 void mouseDown(const MouseEvent& event) override { if (event.x > slider.getRight()) attachment.setValueAsCompleteGesture(float(jlimit(0, last, roundToInt(float(event.y - 12) * float(last) / float(getHeight() - 24))))); }
 void hideLegends() { for (auto* text : legends) text->setVisible(false); } // printed on the rendered panel instead
private:
 int last; Slider slider; OwnedArray<Label> legends; ParameterAttachment attachment;
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
 void setKnobColour(Colour colour) { knob.setColour(Slider::rotarySliderFillColourId, colour); }
 void paint(Graphics& g) override {
  const int current = jlimit(0, names.size() - 1, roundToInt(knob.getValue()));
  for (int i = 0; i < names.size(); ++i) { const bool on = i == current; g.setColour(on ? orbitui::led : orbitui::dim); drawShape(g, names[i], Rectangle<float>(14, 9).withCentre(symbolCentre(i)), on ? 1.5f : 1.f); }
  ignoreUnused(showName);
 }
 void mouseDown(const MouseEvent& event) override { for (int i = 0; i < names.size(); ++i) if (symbolCentre(i).getDistanceFrom(event.position) < 14) knob.setValue(i, sendNotificationSync); }
private:
 static constexpr float startAngle = MathConstants<float>::pi * 1.3f, endAngle = MathConstants<float>::pi * 2.7f;
 enum { centreY = 42 };
 // Each symbol sits around the knob, at the angle the index line points to for that position.
 Point<float> symbolCentre(int index) const {
  const float angle = startAngle + (endAngle - startAngle) * float(index) / float(jmax(1, names.size() - 1)), radius = 33.f;
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

// Small button that shows the current value of a choice parameter and opens a menu to change it.
class ChoiceMenuButton : public TextButton {
public:
 explicit ChoiceMenuButton(RangedAudioParameter& choice)
  : parameter(choice), attachment(choice, [this](float value) { const auto names = parameter.getAllValueStrings(); setButtonText(names[jlimit(0, names.size() - 1, roundToInt(value))]); }, nullptr) {
  getProperties().set("liveText", true); // its legend changes, so it is drawn rather than printed on the rendered cap
  onClick = [this] {
   PopupMenu menu; const auto names = parameter.getAllValueStrings(); const int current = roundToInt(parameter.convertFrom0to1(parameter.getValue()));
   for (int i = 0; i < names.size(); ++i) menu.addItem(i + 1, names[i], true, i == current);
   menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [safe = Component::SafePointer<ChoiceMenuButton>(this)](int result) { if (safe != nullptr && result > 0) safe->attachment.setValueAsCompleteGesture(float(result - 1)); });
  };
  attachment.sendInitialUpdate();
 }
private:
 RangedAudioParameter& parameter; ParameterAttachment attachment;
};

// Key with an LED that puts a time control on the tempo: off, the knob sets a free time; on, the same place holds a
// stepped knob over the note values. The key remembers the last note value, so switching back and forth keeps it.
class SyncKey : public TextButton {
public:
 SyncKey(RangedAudioParameter& choice, int fallback) : remembered(fallback),
  attachment(choice, [this](float value) { const int index = roundToInt(value); if (index > 0) remembered = index; setToggleState(index > 0, dontSendNotification); }, nullptr) {
  setButtonText("SYNC"); getProperties().set("led", 2);
  onClick = [this] { attachment.setValueAsCompleteGesture(getToggleState() ? 0.f : float(remembered)); };
  attachment.sendInitialUpdate();
 }
private:
 int remembered; ParameterAttachment attachment;
};

// Stepped knob over the note values of a sync choice; FREE is left to the SYNC key.
class SyncKnob : public Slider {
public:
 explicit SyncKnob(RangedAudioParameter& choice) : names(choice.getAllValueStrings()),
  attachment(choice, [this](float value) { const int index = roundToInt(value); if (index > 0) setValue(index, dontSendNotification); }, nullptr) {
  setSliderStyle(RotaryHorizontalVerticalDrag); setTextBoxStyle(NoTextBox, false, 0, 0); setRange(1, jmax(2, names.size() - 1), 1);
  setMouseDragSensitivity(120);
  onDragStart = [this] { attachment.beginGesture(); }; onDragEnd = [this] { attachment.endGesture(); };
  onValueChange = [this] { attachment.setValueAsPartOfGesture(float(getValue())); };
  getProperties().set("overlay", true); // sits over the free knob: not part of the rendered layout
  attachment.sendInitialUpdate();
 }
 String getTextFromValue(double value) override { return names[jlimit(0, names.size() - 1, roundToInt(value))]; }
private:
 StringArray names; ParameterAttachment attachment;
};

class Editor : public AudioProcessorEditor, private Timer {
 using APVTS = AudioProcessorValueTreeState;
 enum { designW = 1600, designH = 1000, left = 32, contentW = 1536, gap = 12 };
 enum { headerY = 4, lineY = 80, panelY = 94, panelH = 216, row2Y = 326, row2H = 250, lowerY = 590, lowerH = 164, keysY = 774, keysH = 208 }; // Space below effects separates the keyboard from the editing surface.
 struct Section { String title; Colour colour; Rectangle<int> area; int page; };
 struct Fader { Slider slider; String id; bool followsLink = false; std::unique_ptr<APVTS::SliderAttachment> attachment; };
 struct Toggle { TextButton button; std::unique_ptr<APVTS::ButtonAttachment> attachment; };
 struct Surface : Component { explicit Surface(Editor& e) : owner(e) {} void paint(Graphics& g) override { owner.paintSurface(g); } Editor& owner; };
 // Hands out consecutive sections of one row; widths are given in fader-sized units.
 struct Row {
  Editor& editor; int x, y, height, page; float unit;
  Row(Editor& e, int rowY, int rowH, int rowPage, float totalUnits, int count, int origin = left, int availableWidth = contentW) : editor(e), x(origin), y(rowY), height(rowH), page(rowPage), unit(float(availableWidth - gap * (count - 1)) / totalUnits) {}
  Rectangle<int> next(const String& title, Colour colour, float units) {
   const int width = roundToInt(units * unit); const Rectangle<int> area(x, y, width, height);
   editor.sections.push_back({title, colour, area, page}); editor.sectionColour = colour; x += width + gap; return area.withTrimmedTop(34);
  }
 };
public:
 explicit Editor(Orbit& p) : AudioProcessorEditor(p), orbit(p), surface(*this), keyboard(p.keyboard, MidiKeyboardComponent::horizontalKeyboard) {
  setLookAndFeel(&look);
  addAndMakeVisible(surface); surface.setBounds(0, 0, designW, designH);
  buildHeader(); buildPanelA(); buildRowTwo(); buildLowerRow(); buildKeyboard();
  setPage(0);
  // Rendered artwork: write the layout for tools/orbit_render.py, then use its pictures when they match this layout.
  // Pictures made for another layout would not sit under the controls: the renderer leaves a copy of the layout it
  // used beside them, and they are only used while it matches. The user's own folder (fresh renders) comes first,
  // then the folder filled by the installer.
  const String layout = exportLayout().removeCharacters("\r");
  for (const auto& directory : {OrbitSkin::folder(), OrbitSkin::sharedFolder()}) {
   skin.load(directory);
   if (skin.ready && directory.getChildFile("layout.rendered.json").loadFileAsString().removeCharacters("\r") == layout) break;
   skin.ready = false;
  }
  look.skin = &skin;
  if (skin.ready) {
   for (auto* label : labels) if (!label->getProperties().contains("live")) label->setVisible(false); // captions are printed on the rendered panel
   for (auto* item : switches) item->hideLegends();
   keyboard.skin = &skin;                                // and the keys are in the picture too
   for (auto id : {MidiKeyboardComponent::whiteNoteColourId, MidiKeyboardComponent::shadowColourId, MidiKeyboardComponent::keySeparatorLineColourId}) keyboard.setColour(id, Colours::transparentBlack);
  }
  setResizable(true, true); getConstrainer()->setFixedAspectRatio(double(designW) / double(designH));
  setResizeLimits(designW * 8 / 10, designH * 8 / 10, designW * 3 / 2, designH * 3 / 2);
  setSize(designW * 9 / 10, designH * 9 / 10);
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
 void caption(const String& text, Rectangle<int> bounds, int page, float size = 15.f, Colour colour = orbitui::ink) {
  auto* label = labels.add(new Label({}, text));
  label->setFont(orbitui::legend(jmax(13.f, size))); label->setColour(Label::textColourId, colour); label->setJustificationType(Justification::centred);
  label->setBorderSize({}); label->setMinimumHorizontalScale(.9f); label->setInterceptsMouseClicks(false, false);
  place(*label, bounds, page);
 }
 void setUpSlider(Slider& slider, const char* id, Slider::SliderStyle style) {
  auto* parameter = orbit.state.getParameter(id);
  slider.setSliderStyle(style); slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
  slider.setPopupDisplayEnabled(true, true, &surface); slider.setName(parameter->getName(60));
 }
 Fader& addControl(const char* id, Slider::SliderStyle style, Rectangle<int> bounds, int page) {
  auto* fader = faders.add(new Fader()); fader->id = id;
  setUpSlider(fader->slider, id, style); fader->slider.setColour(Slider::rotarySliderFillColourId, sectionColour); place(fader->slider, bounds, page);
  fader->attachment = std::make_unique<APVTS::SliderAttachment>(orbit.state, id, fader->slider);
  auto* parameter = orbit.state.getParameter(id);
  fader->slider.setDoubleClickReturnValue(true, parameter->convertFrom0to1(parameter->getDefaultValue()));
  return *fader;
 }
 Fader& addFader(const char* id, const char* text, int page, Rectangle<int> cell) {
  caption(text, cell.withHeight(16), page);
  return addControl(id, Slider::LinearVertical, Rectangle<int>(cell.getCentreX() - 20, cell.getY() + 28, 40, cell.getHeight() - 38), page);
 }
 void addEffectScale(const char* id, Rectangle<int> cell) {
  const auto range = orbit.state.getParameter(id)->getNormalisableRange();
  ignoreUnused(range, id); // every knob reads 0 to 10: just the two ends of the scale, at the foot of the knob
  const int cx = cell.getCentreX(), y = cell.getY() + 67;
  caption("0", {cx - 25, y - 5, 12, 10}, -1, 8.f, orbitui::dim); caption("10", {cx + 13, y - 5, 12, 10}, -1, 8.f, orbitui::dim); // kept inside the knob's own column
 }
 void addKnob(const char* id, const char* text, Rectangle<int> cell, bool showScale = false) {
  caption(text, cell.withHeight(16), -1, 13.f);
  const int width = jmin(60, cell.getWidth());
  auto& control = addControl(id, Slider::RotaryHorizontalVerticalDrag,
    {cell.getCentreX() - width / 2, cell.getY() + 15, width, jmin(54, cell.getHeight() - 18)}, -1);
  if (showScale) addEffectScale(id, cell);
  if (StringRef(id) == StringRef("gain")) control.slider.getProperties().set("faceDiameter", 44.f);
 }
 void addRowKnob(const char* id, const char* text, Rectangle<int> cell) {
  caption(text, cell.withHeight(16), 0);
  const int width = jmin(56, cell.getWidth());
  auto& control = addControl(id, Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(cell.getCentreX() - width / 2, cell.getY() + 26, width, 56), 0);
  control.slider.getProperties().set("faceDiameter", 39.f);
  auto* valueLabel = labels.add(new Label());
  valueLabel->setFont(orbitui::legend(13.f)); valueLabel->setColour(Label::textColourId, orbitui::ink);
  valueLabel->setJustificationType(Justification::centred); valueLabel->setInterceptsMouseClicks(false, false);
  valueLabel->getProperties().set("live", true); // changes with the control: never printed in the rendered artwork
  place(*valueLabel, {cell.getX(), cell.getY() + 77, cell.getWidth(), 14}, 0);
  auto update = [valueLabel, slider = &control.slider] { auto text = slider->getTextFromValue(slider->getValue());
   if (text.containsChar('(')) text = text.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
   valueLabel->setText(text, dontSendNotification); };
  control.slider.onValueChange = update; update();
 }
 void addChoice(const char* id, const char* text, const StringArray& names, int page, Rectangle<int> cell, bool vertical, Colour colour = orbitui::neutral) {
  if (vertical && names.size() <= 3) { // a choice between two or three: a slide switch under its caption
   caption(text, cell.withHeight(16), page);
   place(*switches.add(new SlideSwitch(*orbit.state.getParameter(id), names)), {cell.getX(), cell.getY() + 22, cell.getWidth(), 52}, page);
   return;
  }
  auto* choice = choices.add(new ChoiceButtons(*orbit.state.getParameter(id), names, vertical, colour));
  if (vertical) { caption(text, cell.withHeight(16), page); place(*choice, Rectangle<int>(cell.getX() + 3, cell.getY() + 28, cell.getWidth() - 6, jmin(names.size() * 35, cell.getHeight() - 30)), page); }
  else { place(*choice, cell.withHeight(46).reduced(3, 0), page); caption(text, Rectangle<int>(cell.getX(), cell.getY() + 52, cell.getWidth(), 14), page, 11.f, orbitui::dim); }
 }
 void addWaveKnob(const char* id, const char* text, const StringArray& names, int page, Rectangle<int> cell, bool nameBelow = true) {
  caption(text, cell.withHeight(16).translated(0, -5), page); // raised a little: a symbol sits right above the knob
  auto* selector = waveKnobs.add(new WaveKnob(orbit.state, id, names, nameBelow)); selector->setKnobColour(sectionColour);
  place(*selector, {cell.getX(), cell.getY() + 12, cell.getWidth(), 80}, page); // knob centre level with the plain knobs of the row
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
 // SYNC key at keyBounds, and a stepped note-value knob stacked over the free knob; only one of the two knobs shows.
 void addSync(const char* id, int fallback, Slider& freeKnob, Rectangle<int> keyBounds, int page, Label* value) {
  auto* sync = syncs.add(new Sync()); sync->id = id; sync->free = &freeKnob; sync->value = value;
  auto& parameter = *orbit.state.getParameter(id);
  sync->key = std::make_unique<SyncKey>(parameter, fallback); orbitui::tint(*sync->key, orbitui::neutral); place(*sync->key, keyBounds, page);
  sync->knob = std::make_unique<SyncKnob>(parameter); sync->knob->setPopupDisplayEnabled(true, true, &surface); sync->knob->setName(parameter.getName(60));
  sync->knob->setColour(Slider::rotarySliderFillColourId, freeKnob.findColour(Slider::rotarySliderFillColourId));
  if (freeKnob.getProperties().contains("faceDiameter")) sync->knob->getProperties().set("faceDiameter", freeKnob.getProperties()["faceDiameter"]);
  place(*sync->knob, freeKnob.getBounds(), page);
 }
 void useOwnFilterEnvelope() {
  auto* link = orbit.state.getParameter("envLink");
  if (link->getValue() > .5f) { link->beginChangeGesture(); link->setValueNotifyingHost(0.f); link->endChangeGesture(); }
 }
 void addButton(TextButton& button, const String& text, Rectangle<int> bounds, Colour colour, int led) {
  button.setButtonText(text); button.getProperties().set("led", led); button.getProperties().set("cream", true); // memory and utility keys keep light caps
  orbitui::tint(button, colour); place(button, bounds, -1);
 }

 // ---- header: name, LED display, bank / patch memory, save / load, panel switch --------
 void buildHeader() {
  using namespace orbitui;
  refreshPresetMenu();
  presets.setSelectedId(orbit.getCurrentProgram() + 1, dontSendNotification); place(presets, {570, 30, 276, 32}, -1);
  presets.onChange = [this] {
   const int id = presets.getSelectedId();
   if (id >= 1000) { const int index = id - 1000; if (isPositiveAndBelow(index, userNames.size())) showResult(orbit.loadUserPreset(userNames[index])); }
   else if (id > 0) orbit.setCurrentProgram(id - 1);
  };
  for (int i = 0; i < 3; ++i) { auto* button = bankButtons.add(new TextButton()); addButton(*button, String::charToString(juce_wchar('A' + i)), {876 + i * 41, 24, 36, 38}, neutral, 1); button->onClick = [this, i] { shownBank = i; timerCallback(); }; }
  for (int i = 0; i < 8; ++i) { auto* button = patchButtons.add(new TextButton()); addButton(*button, String(i + 1), {1024 + i * 39, 24, 34, 38}, i < 4 ? bright : neutral, 1); button->onClick = [this, i] { orbit.setCurrentProgram(shownBank * 8 + i); timerCallback(); }; }
  addButton(saveButton, "SAVE", {1416, 30, 70, 32}, amber, 0); saveButton.onClick = [this] { saveDialog(); };
  addButton(midiButton, "MIDI", {1344, 18, 60, 44}, neutral, 1); midiButton.onClick = [this] { midiMenu(); }; // MIDI learn
  addButton(loadButton, "LOAD", {1494, 30, 70, 32}, neutral, 0);
  loadButton.onClick = [this] { refreshPresetMenu(); if (orbit.userPresetName().isNotEmpty()) showResult(orbit.loadUserPreset(orbit.userPresetName())); else orbit.setCurrentProgram(orbit.getCurrentProgram()); };
 }

// ---- first row: the playing faders ------------------------------------------------------
 void buildPanelA() {
  using namespace orbitui;
  // Controls are packed at a fixed pitch inside each section (one unit = one fader, 41 px); the width left over
  // becomes bare panel between the sections, so every group reads as a block and the row still spans the panel.
  const float unit = 41.f, total = 5.6f + 6.2f + 9.6f + 5.6f + 7.f; const int gutter = roundToInt((float(contentW) - total * unit) / 4.f);
  int x = left;
  auto next = [this, &x, unit, gutter](const char* title, Colour colour, float units) {
   const Rectangle<int> area(x, panelY, roundToInt(units * unit), panelH); x += area.getWidth() + gutter;
   sections.push_back({title, colour, area, 0}); sectionColour = colour; return area.withTrimmedTop(34);
  };
  // Small weights are spacers: a margin at the section's edge, or the gap between the faders and the knobs / switches.
  auto cells = split(next("OSC I", amber, 5.6f), {.3f, 1, 1, 1, 1, 1, .3f});
  addFader("wave", "SAW/PLS", 0, cells[1]); addFader("width", "WIDTH", 0, cells[2]); addFader("pwm", "PWM", 0, cells[3]); addFader("sub", "SUB", 0, cells[4]); addFader("noise", "NOISE", 0, cells[5]);
  cells = split(next("OSC II", amber, 6.2f), {.3f, 1, 1, .5f, 1.7f, 1.7f});
  addFader("oscLayer", "LEVEL", 0, cells[1]); addFader("osc2Width", "WIDTH", 0, cells[2]);
  addRowKnob("osc2Tune", "TUNE", cells[4].withTrimmedTop(32)); addRowKnob("detune", "FINE", cells[5].withTrimmedTop(32));
  cells = split(next("FILTER A", teal, 9.6f), {.3f, .8f, 1, 1, 1, 1, .9f, .9f, .5f, 2.2f});
  addFader("hpfFreq", "HPF", 0, cells[1]); // bass boost at the bottom, then flat, then a rising high-pass
  addFader("cutoff", "FREQ", 0, cells[2]); addFader("resonance", "RES", 0, cells[3]); addFader("env", "ENV", 0, cells[4]); addFader("lfoFilter", "LFO", 0, cells[5]); addFader("keytrack", "KYBD", 0, cells[6]); addFader("velFilter", "VEL", 0, cells[7]); // velocity opens both filters
  addChoice("slope", "SLOPE", {"12 dB", "24 dB"}, 0, cells[9], true);
  addChoice("filterAType", "MODEL", {"LADDER", "SVF"}, 0, cells[9].withTrimmedTop(100), true);
  cells = split(next("ENV / AMP", amber, 5.6f), {.3f, 1, 1, 1, 1, 1, .3f});
  addFader("attack", "A", 0, cells[1]); addFader("decay", "D", 0, cells[2]); addFader("sustain", "S", 0, cells[3]); addFader("release", "R", 0, cells[4]); addFader("velocity", "VEL", 0, cells[5]);
  cells = split(next("ENV / FILTER", amber, 7.f), {.3f, 1, 1, 1, 1, .5f, 2.2f});
  const char* ids[] = {"fattack", "fdecay", "fsustain", "frelease"}; const char* names[] = {"A", "D", "S", "R"};
  // SOURCE says which envelope drives the filter: its OWN four faders, or a copy of the AMP envelope.
  // Grabbing one of the four faders while AMP is selected switches to OWN, so they always respond.
  for (int i = 0; i < 4; ++i) { auto& fader = addFader(ids[i], names[i], 0, cells[size_t(i) + 1]); fader.followsLink = true; fader.slider.onDragStart = [this] { useOwnFilterEnvelope(); }; }
  addChoice("envLink", "SOURCE", {"OWN", "AMP"}, 0, cells[6], true);
  addChoice("envInvert", "POLARITY", {"NORM", "INV"}, 0, cells[6].withTrimmedTop(100), true);
 }

 // ---- second row: oscillator shapes, modulation, second filter and routing ----
 // Four blocks of two lines of controls, each as wide as its controls; the rest of the row is bare panel between them.
 void buildRowTwo() {
  using namespace orbitui;
  const StringArray shapes{"MIX", "SAW", "PULSE", "TRI", "SINE"}, octaves{"-2", "-1", "0", "+1", "+2"};
  enum { oscW = 382, modW = 310, filterW = 252, routeW = 270, chorusW = 208, plain = 74, selector = 88 }; // plain knob and selector knob columns
  const int gutter = (contentW - oscW - modW - filterW - routeW - chorusW) / 4; int x = left;
  auto section = [this, &x, gutter](const char* title, Colour colour, int width) {
   const Rectangle<int> bounds(x, row2Y, width, row2H); x += width + gutter;
   sections.push_back({title, colour, bounds, 0}); sectionColour = colour; return bounds.withTrimmedTop(30);
  };
  const auto osc = section("OSC SHAPE", amber, oscW);
  for (int i = 0; i < 2; ++i) {
   const int y = osc.getY() + i * 110;
   caption(i == 0 ? "OSC I" : "OSC II", {osc.getX() + 5, y + 44, 42, 26}, 0, 17, print);
   labels.getLast()->setColour(Label::backgroundColourId, amber);
   auto cells = split(Rectangle<int>(osc.getX() + 48, y, selector + selector + plain, 122), {float(selector), float(selector), float(plain)});
   addWaveKnob(i == 0 ? "osc1Shape" : "osc2Shape", "WAVE", shapes, 0, cells[0]);
   addWaveKnob(i == 0 ? "osc1Octave" : "osc2Octave", "OCTAVE", octaves, 0, cells[1], false);
   if (i == 0) addRowKnob("osc1Level", "LEVEL", cells[2]);
   else addRowKnob("crossMod", "X-MOD", cells[2]);
  }
  addToggle("oscSync", "SYNC", 0, {osc.getX() + 306, osc.getY() + 30, 70, 36}, 1, neutral);

  const auto mod = section("LFO / MODULATION", teal, modW);
  auto top = split(mod.withHeight(120), {float(plain), float(plain), float(selector), float(plain)});
  addRowKnob("rate", "RATE", top[0]);
  auto& rateKnob = faders.getLast()->slider; auto* rateValue = labels.getLast(); // the value under RATE shows the note value while synced
  addRowKnob("lfoDelay", "DELAY", top[1]);
  addWaveKnob("lfoShape", "WAVE", {"TRI", "SINE", "SAW", "SQUARE"}, 0, top[2]);
  addRowKnob("lfoPitch", "PITCH", top[3]);
  // Under the LFO knobs: the tempo sync window, KEY TRIG and ENV TRIG.
  const auto under = [](Rectangle<int> cell) { return Rectangle<int>(cell.getX() + 4, cell.getY() + 92, cell.getWidth() - 8, 21); };
  addSync("lfoSync", 5, rateKnob, under(top[0]), 0, rateValue); // 5 = 1/4
  addToggle("lfoKeyTrig", "KEY TRIG", 0, under(top[1]), 2, neutral);
  addToggle("envLfoTrig", "ENV TRIG", 0, under(top[2]), 2, neutral);
  addToggle("lfoManual", "MAN", 0, under(top[3]), 2, neutral); // the LFO only runs while LFO TRIG is held
  auto bottom = split(mod.withTrimmedTop(118), {float(selector), float(plain), float(plain), float(plain)});
  addWaveKnob("pwmSource", "PWM", {"LFO", "MAN", "F+", "F-", "A+", "A-"}, 0, bottom[0]); // filter or amp envelope, either way up
  addRowKnob("envPitch", "ENV>PITCH", bottom[1]); addRowKnob("wheelDepth", "WHEEL", bottom[2]); addRowKnob("wheelFilter", "WHL>FLT", bottom[3]);

  const auto filter = section("FILTER B", teal, filterW);
  auto cells = split(filter.withHeight(120), {float(filterW - 2 * plain), float(plain), float(plain)});
  addChoice("filterBType", "MODEL", {"LADDER", "SVF"}, 0, cells[0], true);
  addRowKnob("cutoffB", "FREQ", cells[1]); addRowKnob("resonanceB", "RES", cells[2]);
  cells = split(filter.withTrimmedTop(110), {float(filterW - 2 * plain), float(plain), float(plain)});
  addChoice("slopeB", "SLOPE", {"12 dB", "24 dB"}, 0, cells[0], true);
  addRowKnob("envB", "ENV", cells[1]);

  const auto route = section("ROUTING", teal, routeW);
  caption("SIGNAL PATH", route.withHeight(16), 0);
  addChoice("filterRoute", "", {"A", "B", "A>B", "B>A", "PAR"}, 0, route.withHeight(120).withTrimmedTop(24), false);
  cells = split(route.withTrimmedTop(110), {float(plain), float(plain), float(plain), float(routeW - 3 * plain)});
  addRowKnob("bendFilter", "BEND", cells[0]);
  addRowKnob("filterBlend", "A/B MIX", cells[1]);
  addRowKnob("atFilter", "PRESS", cells[2]); // key pressure opens both filters, as the bender does with BEND

  // The chorus sits on the main panel, last block of the row, with orange caps of its own.
  // CHORUS above DRIVE, stacked in the last column of the row like FILTER B and ROUTING.
  const Rectangle<int> chorusBox(x, row2Y, chorusW, 126), driveBox(x, row2Y + 128, chorusW, 122);
  sections.push_back({"CHORUS", amber, chorusBox, 0}); sectionColour = amber;
  const auto chorus = chorusBox.withTrimmedTop(30);
  auto* chorusModes = choices.add(new ChoiceButtons(*orbit.state.getParameter("chorusMode"), {"OFF", "I", "II", "I+II"}, false, amber));
  chorusModes->useColouredCaps(amber);
  place(*chorusModes, Rectangle<int>(chorus.getX() + 2, chorus.getY() + 22, chorusW - plain - 4, 48), 0); // the four keys, MIX beside them
  addRowKnob("chorus", "MIX", chorus.withTrimmedLeft(chorusW - plain));
  sections.push_back({"DRIVE", amber, driveBox, 0}); sectionColour = amber;
  // PRE: inside each voice, after its filters (eight-voice instrument); POST: the whole instrument, ahead of the delay.
  cells = split(driveBox.withTrimmedTop(30), {1, 1});
  addRowKnob("voiceDrive", "PRE", cells[0]); addRowKnob("fxDrive", "POST", cells[1]);
 }

 // ---- lower row: voices, glide, arpeggio, effects, output (always visible) ----------------
 void buildLowerRow() {
  using namespace orbitui;
  Row row(*this, lowerY, lowerH, -1, 27.2f, 6);
  auto voices = row.next("VOICES", amber, 4.9f);
  auto cells = split(voices.withHeight(74), {1, 1, .8f, .8f}); // two switches and the eight-voice character above, the assign keys and HOLD below
  addChoice("mode", "COUNT", {"6", "8"}, -1, cells[0], true);
  addChoice("vcaMode", "VCA", {"ENV", "GATE", "F.ENV"}, -1, cells[1], true);
  addKnob("age", "AGE", cells[2]); addKnob("voiceWidth", "WIDTH", cells[3]); // act on the eight-voice instrument only
  cells = split(voices.withTrimmedTop(80).withHeight(46), {3, 1.25f});
  place(*choices.add(new ChoiceButtons(*orbit.state.getParameter("voiceMode"), {"POLY", "MONO", "UNI"}, false, neutral)), cells[0].reduced(2, 0), -1);
  addToggle("hold", "HOLD", -1, cells[1].reduced(2, 0), 1, neutral);
  auto glide = row.next("GLIDE", amber, 4.5f);
  cells = split(glide, {1, 1, 1, 1.5f});
  addKnob("portamento", "PORTA", cells[0]); addKnob("bendRange", "BEND", cells[1]); addKnob("unisonSpread", "SPREAD", cells[2]);
  addChoice("legato", "MODE", {"RETRIG", "LEGATO"}, -1, cells[3], true);
  auto arp = row.next("ARPEGGIO", teal, 6.2f);
  cells = split(arp.withHeight(48), {1, 1, 4});
  addToggle("arpOn", "ON", -1, cells[0].withHeight(38), 1, neutral);
  addToggle("arpSync", "SYNC", -1, cells[1].withHeight(38), 1, neutral);
  addChoice("arpPattern", "", {"UP", "DOWN", "U&D", "RND"}, -1, cells[2].withHeight(38), false);
  cells = split(arp.withTrimmedTop(52), {3, 2.4f, 1.2f, 1.2f});
  addChoice("arpDivision", "STEP", {"1/4", "1/8", "1/16", "1/32"}, -1, cells[0], false);
  addChoice("arpOctaves", "OCT", {"1", "2", "3"}, -1, cells[1], false);
  addKnob("arpTempo", "TEMPO", cells[2]); addKnob("arpGate", "GATE", cells[3]);
  cells = split(row.next("DELAY", teal, 4.6f), {1, 1, 1, 1});
  // TIME shares its column with the sync selector: FREE uses the knob, a note value follows the tempo.
  caption("TIME", cells[0].withHeight(14), -1, 12.f);
  const int timeWidth = jmin(50, cells[0].getWidth());
  delayTimeKnob = &addControl("delayTime", Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(cells[0].getCentreX() - timeWidth / 2, cells[0].getY() + 15, timeWidth, 54), -1).slider;
  addEffectScale("delayTime", cells[0]);
  caption({}, {cells[0].getCentreX() - 13, cells[0].getY() + 61, 26, 12}, -1, 10.f); // the note value while synced, between 0 and 10
  labels.getLast()->getProperties().set("live", true);
  addSync("delaySync", 6, *delayTimeKnob, {cells[0].getX() + 1, cells[0].getY() + 88, cells[0].getWidth() - 2, 24}, -1, labels.getLast()); // 6 = 1/8
  addKnob("delayFeedback", "FDBK", cells[1], true); addKnob("delayMod", "MOD", cells[2], true); addKnob("delayMix", "LEVEL", cells[3], true);
  addToggle("delayOn", "ON", -1, {cells[3].getX() + 2, cells[3].getY() + 80, cells[3].getWidth() - 4, 44}, 1, neutral); // switches the delay off without touching its settings
  // The reverb type (VINT / PLATE / SPACE) is chosen on the keys under the four knobs.
  cells = split(row.next("REVERB", teal, 4.6f), {1, 1, 1, 1});
  caption("SIZE", cells[0].withHeight(14), -1, 12.f);
  const int sizeWidth = jmin(50, cells[0].getWidth());
  addControl("reverbSize", Slider::RotaryHorizontalVerticalDrag, Rectangle<int>(cells[0].getCentreX() - sizeWidth / 2, cells[0].getY() + 15, sizeWidth, 54), -1);
  addEffectScale("reverbSize", cells[0]);
  const auto reverbArea = cells[0].getUnion(cells[2]); // the three reverbs on lit keys, then ON, across the foot of the section
  addToggle("reverbOn", "ON", -1, {cells[3].getX() + 2, cells[3].getY() + 80, cells[3].getWidth() - 4, 44}, 1, neutral);
  place(*choices.add(new ChoiceButtons(*orbit.state.getParameter("reverbType"), {"VINT", "PLATE", "SPACE"}, false, neutral)), {reverbArea.getX() + 2, reverbArea.getY() + 80, reverbArea.getWidth() - 4, 44}, -1);
  addKnob("reverbDamp", "DAMP", cells[1], true); addKnob("reverbColor", "COLOR", cells[2], true); addKnob("reverbMix", "LEVEL", cells[3], true);
  cells = split(row.next("OUTPUT", neutral, 2.4f), {1, 1});
  addKnob("gain", "VOLUME", cells[0]); addKnob("masterTune", "TUNE", cells[1]); // TUNE: the whole instrument, +/- 50 cents
 }

 void buildKeyboard() {
  using namespace orbitui;
  caption("KEYBOARD", {left, keysY + 28, 132, 16}, -1);
  addButton(octDown, "OCT -", {left + 10, keysY + 56, 53, 40}, neutral, 0); addButton(octUp, "OCT +", {left + 69, keysY + 56, 53, 40}, neutral, 0);
  octDown.onClick = [this] { octave = jmax(0, octave - 1); updateKeyboard(); }; octUp.onClick = [this] { octave = jmin(4, octave + 1); updateKeyboard(); };
  // LFO TRIG: held down, it brings the LFO in when LFO MAN is on (it lights while held, from the panel or a learned MIDI button).
  addButton(lfoTrigButton, "LFO TRIG", {left + 10, keysY + 134, 112, 42}, neutral, 0);
  lfoTrigButton.onStateChange = [this] {
   const bool down = lfoTrigButton.isDown(); if (down == lfoTrigDown) return; lfoTrigDown = down;
   auto* trig = orbit.state.getParameter("lfoTrig"); trig->beginChangeGesture(); trig->setValueNotifyingHost(down ? 1.f : 0.f); trig->endChangeGesture();
  };
  keyboard.setScrollButtonsVisible(false); keyboard.setKeyWidth(float(contentW - 146) / 36.f); keyboard.setOctaveForMiddleC(4); keyboard.setBlackNoteLengthProportion(.62f);
  place(keyboard, {left + 146, keysY + 6, contentW - 146, keysH - 6}, -1);
  // Computer keyboard, AZERTY rows: Q S D F G H J K L M are the white keys starting on C,
  // Z E T Y U O P the black keys above them. OCT - / OCT + move the range.
  keyboard.clearKeyMappings();
  const char* keys = "qzsedftgyhujkolpm";
  for (int i = 0; keys[i] != 0; ++i) keyboard.setKeyPressForNote(KeyPress(keys[i], 0, 0), i);
  updateKeyboard();
 }
 void updateKeyboard() {
  // Release every note held from the computer keyboard or the mouse before moving the range. Otherwise a key
  // still down during the change is released as a different note, and the original one never stops.
  keyboard.focusLost(Component::focusChangedDirectly);
  keyboard.setAvailableRange(12 * (octave + 1), 12 * (octave + 1) + 60);
  keyboard.setKeyPressBaseOctave(octave + 1); surface.repaint();
 }
 void setPage(int index) { page = index; for (auto& entry : paged) entry.first->setVisible(entry.second == page); surface.repaint(); }

 // ---- painting -------------------------------------------------------------------------
 void paintSurface(Graphics& g) {
  using namespace orbitui;
  if (skin.ready) { // rendered panel; only the texts that change are drawn over it
   g.drawImage(skin.background, Rectangle<float>(0, 0, float(designW), float(designH)));
   g.setColour(led.withAlpha(.75f)); g.setFont(mono(11.f)); g.drawText(displayLine, 580, 13, 260, 16, Justification::centredLeft);
   g.setColour(dim); g.setFont(face(12.f, false)); g.drawText("C" + String(octave) + " - C" + String(octave + 5), left, keysY + 106, 132, 16, Justification::centred);
   return;
  }
  g.setGradientFill(ColourGradient(Colour(0xff272a2b), 0, 0, Colour(0xff181b1d), 0, float(designH), false));
  g.fillAll();
  g.setTiledImageFill(panelGrain, 0, 0, 1.f); g.fillRect(16, 0, designW - 32, int(designH));
  for (int seam : {int(lineY), row2Y - 10, lowerY - 10, keysY - 12}) {
   g.setColour(Colours::black.withAlpha(.7f)); g.fillRect(16, seam, designW - 32, 3);
   g.setColour(Colours::white.withAlpha(.15f)); g.drawHorizontalLine(seam + 3, 16, designW - 16);
  }
  // Slim walnut cheeks, deliberately quiet so the controls remain the focal point.
  for (int x : {0, designW - 16}) {
   ColourGradient wood(Colour(0xff261b16), float(x), 0, Colour(0xff3f3025), float(x + 12), 0, false);
   wood.addColour(.45, Colour(0xff554434)); g.setGradientFill(wood); g.fillRect(x, 0, 16, int(designH));
   g.setColour(Colour(0xff1c1714).withAlpha(.3f));
   for (int i = 2; i < 16; i += 2) {
    Path grain; grain.startNewSubPath(float(x + i), 0);
    for (int y = 8; y <= designH; y += 8) grain.lineTo(float(x + i) + .6f * std::sin(float(y) * .025f + float(i)), float(y));
    g.strokePath(grain, PathStrokeType(.6f));
   }
  }
  g.setColour(Colours::white.withAlpha(.10f)); g.drawRoundedRectangle(17.f, 1.f, float(designW - 34), float(designH - 14), 5.f, 1.f);
  g.setColour(ink); g.setFont(face(48.f, true).withExtraKerningFactor(.025f));
  g.drawText("ORBIT", left, headerY + 2, 150, 50, Justification::centredLeft);
  g.setColour(amber); g.drawText("106", left + 112, headerY + 2, 90, 50, Justification::centredLeft);
  g.setColour(ink.withAlpha(.45f)); g.drawVerticalLine(left + 192, 20, 56);
  g.setColour(dim); g.setFont(face(14.f, false).withExtraKerningFactor(.12f));
  g.drawText("POLYPHONIC SYNTHESIZER", left + 202, headerY + 25, 270, 16, Justification::centredLeft);
  const Rectangle<float> window(564, 10, 290, 56);
  g.setColour(Colours::black.withAlpha(.35f)); g.fillRoundedRectangle(window.expanded(2), 4);
  g.setGradientFill(ColourGradient(Colour(0xff080b0c), 0, 22, Colour(0xff191c1d), 0, 86, false)); g.fillRoundedRectangle(window, 3);
  g.setColour(Colour(0xff454747)); g.drawRoundedRectangle(window, 3, .7f);
  g.setColour(led.withAlpha(.8f)); g.setFont(mono(11.f)); g.drawText(displayLine, 580, 13, 260, 16, Justification::centredLeft);
  g.setColour(dim); g.setFont(face(12.f)); g.drawText("BANK", 876, 7, 118, 14, Justification::centred); g.drawText("PATCH", 1024, 7, 307, 14, Justification::centred);
  g.setColour(Colours::black.withAlpha(.4f)); g.drawHorizontalLine(lineY, 20, designW - 20);
  g.setColour(Colours::white.withAlpha(.08f)); g.drawHorizontalLine(lineY + 1, 20, designW - 20);
  for (const auto& section : sections) {
   if (section.page >= 0 && section.page != page) continue;
   auto bar = section.area.withHeight(22);
   if (section.area.getX() > left) {
    g.setColour(ink.withAlpha(.16f));
    g.drawVerticalLine(section.area.getX() - 7, float(section.area.getY() + 4), float(section.area.getBottom() - 8));
   }
   g.setFont(face(20.f).withExtraKerningFactor(.06f)); g.setColour(ink);
   g.drawFittedText(section.title, bar.reduced(3, 0), Justification::centredLeft, 1, .75f);
   g.setColour(section.colour.withAlpha(.85f)); g.fillRect(bar.getX(), bar.getBottom() + 2, bar.getWidth(), 2);
  }
  g.setColour(ink.withAlpha(.35f));
  g.drawHorizontalLine(row2Y + 134, left + 4, left + 366);
  g.drawVerticalLine(left + 378, float(row2Y + 38), float(row2Y + row2H - 6));
  g.drawHorizontalLine(row2Y + 126, left + 1008, left + contentW);
  g.setColour(Colours::black.withAlpha(.35f)); g.fillRect(20, keysY - 12, designW - 40, 10);
  g.setColour(Colours::white.withAlpha(.08f)); g.drawHorizontalLine(keysY - 12, 20, designW - 20);
  g.setColour(dim); g.setFont(face(12.f, false)); g.drawText("C" + String(octave) + " - C" + String(octave + 5), left, keysY + 106, 132, 16, Justification::centred);
  g.setColour(slot); g.fillRect(left + 146, int(keysY), contentW - 146, 6);
  g.setGradientFill(ColourGradient(Colour(0xff573d2b), 0, designH - 12.f, Colour(0xff281c15), 0, float(designH), false));
  g.fillRect(16, designH - 12, designW - 32, 12);

 }

 // ---- layout export: tells tools/orbit_render.py where every control sits ----------------------
 static String colourName(Colour colour) { using namespace orbitui; return colour == amber ? "amber" : colour == teal ? "teal" : colour == bright ? "bright" : colour == dim ? "dim" : colour == ink ? "ink" : "neutral"; }
 static DynamicObject::Ptr boxed(Rectangle<int> r) { DynamicObject::Ptr item = new DynamicObject(); item->setProperty("x", r.getX()); item->setProperty("y", r.getY()); item->setProperty("w", r.getWidth()); item->setProperty("h", r.getHeight()); return item; }
 void collect(Component& parent, Array<var>& texts, Array<var>& faderList, Array<var>& knobList, Array<var>& buttonList) {
  for (auto* child : parent.getChildren()) {
   if (!child->isVisible() || child->getProperties().contains("overlay") || dynamic_cast<ComboBox*>(child) != nullptr || dynamic_cast<MidiKeyboardComponent*>(child) != nullptr) continue;
   const auto area = surface.getLocalArea(child, child->getLocalBounds()); auto item = boxed(area);
   if (auto* label = dynamic_cast<Label*>(child)) {
    if (label->getProperties().contains("live")) continue; // a value readout: it would make the layout differ from one patch to the next
    item->setProperty("text", label->getText()); item->setProperty("size", label->getFont().getHeight()); item->setProperty("colour", colourName(label->findColour(Label::textColourId))); texts.add(var(item.get()));
   } else if (auto* slider = dynamic_cast<Slider*>(child)) {
    if (slider->isRotary()) {
     const bool selector = slider->getProperties().contains("selector"); const auto travel = slider->getRotaryParameters();
     item->setProperty("cx", area.getCentreX()); item->setProperty("cy", area.getCentreY()); item->setProperty("d", jmin(area.getWidth(), area.getHeight()) - (selector ? 6 : 14));
     item->setProperty("selector", selector); item->setProperty("start", travel.startAngleRadians); item->setProperty("end", travel.endAngleRadians);
     item->setProperty("colour", colourName(slider->findColour(Slider::rotarySliderFillColourId))); knobList.add(var(item.get()));
    } else { // the cap travels between top and bottom (the look-and-feel keeps 12 px clear at each end)
     item->setProperty("cx", area.getCentreX()); item->setProperty("top", area.getY() + 12); item->setProperty("bottom", area.getBottom() - 12);
     item->setProperty("marks", int(slider->getProperties().getWithDefault("marks", 10))); faderList.add(var(item.get()));
    }
   } else if (auto* button = dynamic_cast<TextButton*>(child)) {
    const bool above = button->getProperties().contains("legendAbove");
    if (above) { auto legend = boxed(area.withY(area.getY() - 18).withHeight(14)); legend->setProperty("text", button->getButtonText()); legend->setProperty("size", 13); legend->setProperty("colour", "ink"); texts.add(var(legend.get())); }
    item->setProperty("text", button->getProperties().contains("liveText") || above ? String() : button->getButtonText()); item->setProperty("led", orbitui::ledMode(*button)); item->setProperty("dark", !button->getProperties().contains("cream")); item->setProperty("colour", colourName(button->findColour(TextButton::buttonColourId)));
    item->setProperty("live", button->getProperties().contains("liveText")); buttonList.add(var(item.get()));
   } else collect(*child, texts, faderList, knobList, buttonList);
  }
 }
 String exportLayout() { // returns the layout text; also writes it for the renderer when the folder is writable
  Array<var> sectionList, texts, faderList, knobList, buttonList;
  for (const auto& section : sections) { auto item = boxed(section.area.withHeight(22)); item->setProperty("title", section.title); item->setProperty("colour", colourName(section.colour)); sectionList.add(var(item.get())); }
  collect(surface, texts, faderList, knobList, buttonList);
  for (const auto& fixed : {std::make_pair("BANK", Rectangle<int>(876, 7, 118, 14)), std::make_pair("PATCH", Rectangle<int>(1024, 7, 307, 14))}) { auto item = boxed(fixed.second); item->setProperty("text", fixed.first); item->setProperty("size", 11); item->setProperty("colour", "dim"); texts.add(var(item.get())); }
  DynamicObject::Ptr root = new DynamicObject();
  root->setProperty("width", int(designW)); root->setProperty("height", int(designH)); root->setProperty("left", int(left)); root->setProperty("contentWidth", int(contentW));
  root->setProperty("headerY", int(headerY)); root->setProperty("accentY", int(lineY));
  root->setProperty("display", var(boxed({564, 10, 290, 56}).get())); root->setProperty("keyboardBlock", var(boxed({left, keysY + 6, 132, keysH - 6}).get())); root->setProperty("keyboard", var(boxed({left + 146, keysY, contentW - 146, keysH}).get()));
  Array<var> keyList; // every key of the on-screen keyboard, so that the rendered keys sit exactly under the playable ones
  for (int note = keyboard.getRangeStart(); note <= keyboard.getRangeEnd(); ++note) {
   const auto key = keyboard.getRectangleForKey(note).translated(float(keyboard.getX()), float(keyboard.getY()));
   DynamicObject::Ptr item = new DynamicObject(); item->setProperty("x", key.getX()); item->setProperty("y", key.getY()); item->setProperty("w", key.getWidth()); item->setProperty("h", key.getHeight());
   item->setProperty("black", MidiMessage::isMidiNoteBlack(note)); keyList.add(var(item.get()));
  }
  root->setProperty("keys", keyList);
  root->setProperty("sections", sectionList); root->setProperty("labels", texts); root->setProperty("faders", faderList); root->setProperty("knobs", knobList); root->setProperty("buttons", buttonList);
  const auto file = OrbitSkin::folder().getChildFile("layout.json"); const auto text = JSON::toString(var(root.get()));
  if (file.loadFileAsString() != text) { file.getParentDirectory().createDirectory(); file.replaceWithText(text); }
  return text;
 }

 // ---- presets ---------------------------------------------------------------------------
 void refreshPresetMenu() {
  presets.clear(dontSendNotification); presets.addSectionHeading("FACTORY");
  for (int i = 0; i < orbit.getNumPrograms(); ++i) presets.addItem(orbit.getProgramName(i), i + 1);
  userNames = orbit.userPresetNames();
  if (!userNames.isEmpty()) { presets.addSeparator(); presets.addSectionHeading("USER"); for (int i = 0; i < userNames.size(); ++i) presets.addItem("USER / " + userNames[i], 1000 + i); }
 }
 void midiMenu() {
  using orbitui::utf8;
  PopupMenu menu; const int touched = orbit.lastTouchedParameter();
  const auto nameOf = [this](int index) { return orbit.getParameters()[index]->getName(30); };
  if (orbit.isLearning()) menu.addItem(1, utf8("Annuler l'apprentissage"));
  else menu.addItem(1, utf8("Apprendre : touche un réglage, puis tourne un bouton de ton contrôleur"));
  menu.addItem(2, touched >= 0 && orbit.ccFor(touched) >= 0 ? utf8("Oublier le CC de « ") + nameOf(touched) + utf8(" »") : utf8("Oublier le CC du dernier réglage touché"), touched >= 0 && orbit.ccFor(touched) >= 0);
  menu.addItem(3, utf8("Tout oublier"));
  bool any = false;
  for (int index = 0; index < orbit.getParameters().size(); ++index) if (const int cc = orbit.ccFor(index); cc >= 0) { if (!any) menu.addSeparator(); any = true; menu.addItem(100 + index, "CC " + String(cc) + utf8("  →  ") + nameOf(index), false); }
  menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&midiButton), [safe = Component::SafePointer<Editor>(this), touched](int result) {
   if (safe == nullptr) return;
   if (result == 1) { if (safe->orbit.isLearning()) safe->orbit.stopLearning(); else safe->orbit.startLearning(); }
   if (result == 2 && touched >= 0) safe->orbit.forget(touched);
   if (result == 3) safe->orbit.forgetAll();
   safe->timerCallback(); });
 }
 void showResult(const Result& result) { if (result.failed()) AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Preset", result.getErrorMessage()); }
 void saveDialog() {
  using orbitui::utf8;
  auto* dialog = new AlertWindow("Enregistrer un preset", utf8("Enregistrement dans Application Support/SF Instruments/Orbit 106. Un nom existant peut être remplacé après confirmation."), MessageBoxIconType::NoIcon);
  dialog->addTextEditor("name", orbit.userPresetName(), "Nom du preset");
  dialog->addButton("Enregistrer", 1, KeyPress(KeyPress::returnKey)); dialog->addButton("Annuler", 0, KeyPress(KeyPress::escapeKey));
  Component::SafePointer<Editor> safe(this);
  dialog->enterModalState(true, ModalCallbackFunction::create([safe, dialog](int result) {
   if (result != 1 || !safe) return;
    const String name = dialog->getTextEditorContents("name").trim();
    const auto store = [safe, name](bool replace) { if (!safe) return; const auto status = safe->orbit.saveUserPreset(name, replace); safe->showResult(status); if (status.wasOk()) safe->refreshPresetMenu(); };
    if (!safe->orbit.userPresetExists(name)) { store(false); return; }
    AlertWindow::showAsync(MessageBoxOptions().withIconType(MessageBoxIconType::QuestionIcon).withTitle("Remplacer le preset ?")
     .withMessage(utf8("Un preset « ") + name + utf8(" » existe déjà. Le remplacer par les réglages actuels ?")).withButton("Remplacer").withButton("Annuler").withAssociatedComponent(safe.getComponent()),
     [store](int choice) { if (choice == 1) store(true); });
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
  midiButton.setToggleState(orbit.isLearning(), dontSendNotification);
  String shown = line;
  if (orbit.isLearning()) { const int touched = orbit.lastTouchedParameter();
   shown = touched < 0 ? String("MIDI LEARN    TOUCH A CONTROL") : "MIDI LEARN    " + orbit.getParameters()[touched]->getName(14).toUpperCase() + "  <  MOVE A CC"; }
  if (shown != displayLine) { displayLine = shown; surface.repaint(564, 10, 290, 24); }
  // Standalone only: keep the typing keys on the piano keyboard. Inside a DAW the host keeps its own shortcuts.
  if (orbit.wrapperType == AudioProcessor::wrapperType_Standalone && isShowing() && Component::getCurrentlyFocusedComponent() == nullptr && Component::getCurrentlyModalComponent() == nullptr) keyboard.grabKeyboardFocus();
  lfoTrigButton.setToggleState(orbit.state.getRawParameterValue("lfoTrig")->load() > .5f, dontSendNotification);
  for (auto* sync : syncs) { // synced: the stepped note-value knob takes the place of the free one
   const bool synced = orbit.state.getRawParameterValue(sync->id)->load() > .5f;
   sync->free->setAlpha(synced ? 0.f : 1.f); sync->free->setInterceptsMouseClicks(!synced, false);
   sync->knob->setAlpha(synced ? 1.f : 0.f); sync->knob->setInterceptsMouseClicks(synced, false);
   if (synced) sync->value->setText(sync->knob->getTextFromValue(sync->knob->getValue()), dontSendNotification);
   else if (sync->wasSynced) { if (sync->free->onValueChange) sync->free->onValueChange(); else sync->value->setText({}, dontSendNotification); }
   sync->wasSynced = synced;
  }
  const bool linked = orbit.state.getRawParameterValue("envLink")->load() > .5f;
  for (auto* fader : faders) if (fader->followsLink) fader->slider.setAlpha(linked ? .4f : 1.f); // dimmed, but still grabbable
 }

 Image panelGrain = [] {
  Image tile(Image::ARGB, 256, 256, true); Random random(106);
  for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x)
   tile.setPixelAt(x, y, (random.nextBool() ? Colours::white : Colours::black).withAlpha(float(random.nextInt(8)) / 255.f));
  return tile;
 }();
 Orbit& orbit; OrbitSkin skin; OrbitLook look; Surface surface; PianoKeyboard keyboard;
 std::vector<Section> sections; std::vector<std::pair<Component*, int>> paged;
 OwnedArray<Label> labels; OwnedArray<Fader> faders; OwnedArray<Toggle> toggles; OwnedArray<ChoiceButtons> choices; OwnedArray<WaveKnob> waveKnobs; OwnedArray<SlideSwitch> switches; OwnedArray<ChoiceMenuButton> menuButtons; Slider* delayTimeKnob = nullptr;
 struct Sync { std::unique_ptr<SyncKey> key; std::unique_ptr<SyncKnob> knob; Slider* free = nullptr; Label* value = nullptr; const char* id = ""; bool wasSynced = true; };
 OwnedArray<Sync> syncs;
 OwnedArray<TextButton> bankButtons, patchButtons;
 ComboBox presets; TextButton saveButton, loadButton, octDown, octUp, midiButton, lfoTrigButton; bool lfoTrigDown = false;
 StringArray userNames; String displayLine; Colour sectionColour{orbitui::neutral};
 int page = 0, octave = 2, shownBank = 0, lastProgram = -1;
};
