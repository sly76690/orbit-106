#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>

// Two original algorithmic reverbs sharing one set of controls (size, damping, level):
//
//  VINTAGE  a deep, slowly moving hall in the manner of early digital studio reverbs: early reflections,
//           then a dense modulated tail whose lows outlast its highs (details above the class).
//  PLATE    a bright, fast-building plate: input diffusion into two cross-coupled loops of all-pass
//           filters and delays, the well-known figure-of-eight tank layout from the reverb literature.
//
// All lengths and coefficients are tuning values chosen by design, to be adjusted by ear.
namespace orbitfx {

// Circular delay line. read(d) returns the sample written d writes ago, counted from the next write (d >= 1).
class Line {
public:
 void prepare(int maxSamples) { data.assign(size_t(juce::nextPowerOfTwo(juce::jmax(16, maxSamples + 8))), 0.f); mask = int(data.size()) - 1; position = 0; }
 void clear() { std::fill(data.begin(), data.end(), 0.f); }
 void write(float x) { position = (position + 1) & mask; data[size_t(position)] = x; }
 float read(int delay) const { return data[size_t((position + 1 - delay) & mask)]; }
 float tap(float delay) const { const float whole = std::floor(delay), fraction = delay - whole; const int d = int(whole); const float a = read(d), b = read(d + 1); return a + fraction * (b - a); }
private:
 std::vector<float> data; int mask = 0, position = 0;
};

// All-pass diffuser built on a Line: passes every frequency at equal level but smears it in time.
// COLOR's saturation: transparent at zero drive, then louder passages compress and thicken. Never louder than its input.
inline float shaped(float x, float drive) { return drive < .05f ? x : std::tanh(drive * x) / drive; }

inline float allpass(Line& line, float x, float gain, float delay) { const float z = line.tap(delay), v = x - gain * z; line.write(v); return z + gain * v; }

// VINTAGE hall. What makes it deep rather than merely long:
//  - the dry sound first produces a handful of early reflections, different on each side, and only after a
//    pre-delay that grows with SIZE does the late tail start: the ear reads that gap as distance;
//  - the tail comes from eight long delay lines that all feed each other equally (a Hadamard mix), each with
//    its own all-pass inside the loop, so echoes multiply very quickly into a smooth wash instead of a flutter;
//  - every line is slowly and smoothly modulated at its own speed, which gives the gently moving, chorused tail;
//  - low frequencies are left to ring longer than the rest (bassMultiply) while DAMP shortens the highs,
//    as in a real hall;
//  - left and right inputs stay separate all the way through, so the image is wide;
//  - the loop saturates softly, so a loud tail thickens instead of staying clinically clean.
class VintageHall {
public:
 void prepare(double sampleRate) {
  sr = float(sampleRate);
  for (size_t i = 0; i < count; ++i) {
   lines[i].prepare(int((lengthMs[i] * 1.35f + 6.f) * .001f * sr) + 64); inner[i].prepare(int(innerMs[i] * .001f * sr) + 16);
   phase[i] = float(i) / float(count);
  }
  for (size_t i = 0; i < 3; ++i) { diffuserL[i].prepare(int(diffuserLeftMs[i] * .001f * sr) + 16); diffuserR[i].prepare(int(diffuserRightMs[i] * .001f * sr) + 16); }
  preL.prepare(int(.1f * sr) + 16); preR.prepare(int(.1f * sr) + 16);
  inputTone = coefficient(8500.f); lowSplit = coefficient(250.f); damp = coefficient(6000.f);
  stretch.reset(sampleRate, .2); stretch.setCurrentAndTargetValue(.9f); preDelay.reset(sampleRate, .2); preDelay.setCurrentAndTargetValue(23.f);
  clear();
 }
 void clear() {
  for (auto& line : lines) line.clear(); for (auto& line : inner) line.clear();
  for (auto& line : diffuserL) line.clear(); for (auto& line : diffuserR) line.clear();
  preL.clear(); preR.clear(); highState.fill(0.f); lowState.fill(0.f); tonedL = tonedR = 0;
 }
 // colour: 0 = clean and almost still, 1 = heavily modulated and saturated. 0.4 is the reference voicing.
 void configure(float size, float damping, float colour) {
  colour = juce::jlimit(0.f, 1.f, colour); wobble = (.4f + 2.75f * colour) * .001f * sr * .5f; drive = 2.5f * colour;
  const float target = .5f + .8f * size, seconds = .6f + 9.f * size * size; // tail: 0.6 s to about 10 s
  stretch.setTargetValue(target); preDelay.setTargetValue(8.f + 30.f * size);
  for (size_t i = 0; i < count; ++i) {
   const float loop = (lengthMs[i] * target + innerMs[i]) * .001f;
   high[i] = std::pow(10.f, -3.f * loop / seconds); low[i] = std::pow(10.f, -3.f * loop / (seconds * bassMultiply));
  }
  damp = coefficient(12000.f * std::pow(.125f, juce::jlimit(0.f, 1.f, damping))); // highs roll off from 12 kHz down to 1.5 kHz
 }
 void process(float inL, float inR, float& left, float& right) {
  const float pre = preDelay.getNextValue() * .001f * sr + 1.f;
  float a = preL.tap(pre), b = preR.tap(pre), earlyL = 0, earlyR = 0;
  for (size_t i = 0; i < 4; ++i) { earlyL += earlyGain[i] * preL.read(samples(earlyLeftMs[i])); earlyR += earlyGain[i] * preR.read(samples(earlyRightMs[i])); }
  preL.write(inL); preR.write(inR);
  for (size_t i = 0; i < 3; ++i) { a = allpass(diffuserL[i], a, .62f, diffuserLeftMs[i] * .001f * sr); b = allpass(diffuserR[i], b, .62f, diffuserRightMs[i] * .001f * sr); }
  tonedL += inputTone * (a - tonedL); tonedR += inputTone * (b - tonedR);
  const float scale = stretch.getNextValue() * .001f * sr;
  std::array<float, count> v;
  for (size_t i = 0; i < count; ++i) {
   phase[i] += rateHz[i] / sr; if (phase[i] >= 1.f) phase[i] -= 1.f;
   const float x = 2.f * phase[i] - 1.f, slow = 4.f * x * (1.f - std::abs(x)); // smooth sine-like sweep, no corners
   float quick = phase[i] * 5.f; quick -= std::floor(quick); const float y2 = 2.f * quick - 1.f;
   const float sine = (1.f - flutterShare) * slow + flutterShare * 4.f * y2 * (1.f - std::abs(y2));
   float y = lines[i].tap(lengthMs[i] * scale + wobble * (1.f + sine) + 2.f);
   y = allpass(inner[i], y, .5f, innerMs[i] * .001f * sr);
   highState[i] += damp * (y - highState[i]); lowState[i] += lowSplit * (highState[i] - lowState[i]);
   v[i] = high[i] * highState[i] + (low[i] - high[i]) * lowState[i];
  }
  left = .4f * (v[0] - v[2] + v[4] - v[6]) + earlyLevel * earlyL; right = .4f * (v[1] - v[3] + v[5] - v[7]) + earlyLevel * earlyR;
  for (size_t length = 1; length < count; length <<= 1) for (size_t i = 0; i < count; i += length * 2) for (size_t j = i; j < i + length; ++j) { const float p = v[j], q = v[j + length]; v[j] = p + q; v[j + length] = p - q; }
  for (size_t i = 0; i < count; ++i) lines[i].write(shaped(v[i] * .35355339f + .5f * ((i & 1) ? tonedR : tonedL), drive)); // 1/sqrt(8): the mix neither adds nor loses energy; tanh: loud tails compress and thicken like old hardware
 }
private:
 static constexpr size_t count = 8;
 static constexpr float bassMultiply = 1.5f;   // how much longer the lows ring than the mids
 static constexpr float earlyLevel = .6f;     // early reflections against the tail
 static constexpr float flutterShare = .2f;   // a faster, smaller movement on top of it
 static constexpr std::array<float, count> lengthMs{43.3f, 51.7f, 61.9f, 71.3f, 83.9f, 97.1f, 113.3f, 131.9f};
 static constexpr std::array<float, count> innerMs{11.3f, 13.7f, 17.9f, 19.3f, 23.9f, 27.1f, 29.7f, 33.1f};
 static constexpr std::array<float, count> rateHz{.07f, .11f, .17f, .23f, .31f, .41f, .47f, .59f};
 static constexpr std::array<float, 3> diffuserLeftMs{4.7f, 7.3f, 11.1f}, diffuserRightMs{5.3f, 8.1f, 12.3f};
 static constexpr std::array<float, 4> earlyLeftMs{7.1f, 13.7f, 22.3f, 31.9f}, earlyRightMs{9.3f, 16.9f, 25.1f, 35.3f}, earlyGain{.7f, .55f, .4f, .3f};
 float coefficient(float hz) const { return 1.f - std::exp(-juce::MathConstants<float>::twoPi * hz / sr); }
 int samples(float ms) const { return juce::jmax(1, int(ms * .001f * sr)); }
 std::array<Line, count> lines, inner; std::array<Line, 3> diffuserL, diffuserR; Line preL, preR;
 std::array<float, count> phase{}, highState{}, lowState{}, high{}, low{};
 juce::SmoothedValue<float> stretch, preDelay; float sr = 44100.f, inputTone = 1.f, lowSplit = .03f, damp = 1.f, tonedL = 0, tonedR = 0, wobble = 33.f, drive = 1.f;
};

class Plate {
public:
 void prepare(double sampleRate) {
  sr = float(sampleRate); k = sr / 29761.f; // lengths below are given at this reference rate
  const float in[4] = {142, 107, 379, 277};
  for (size_t i = 0; i < 4; ++i) { inLength[i] = in[i] * k; input[i].prepare(int(inLength[i]) + 16); }
  apA1.prepare(int(704 * k) + 16); dA1.prepare(int(4453 * k) + 16); apA2.prepare(int(1800 * k) + 16); dA2.prepare(int(3720 * k) + 16);
  apB1.prepare(int(940 * k) + 16); dB1.prepare(int(4217 * k) + 16); apB2.prepare(int(2656 * k) + 16); dB2.prepare(int(3163 * k) + 16);
  clear();
 }
 void clear() { for (auto& line : input) line.clear(); for (auto* line : {&apA1, &dA1, &apA2, &dA2, &apB1, &dB1, &apB2, &dB2}) line->clear(); dampA = dampB = 0; phase = 0; }
 // colour: below 0.4 the plate moves less; above it, it starts to saturate. 0.4 is the reference voicing.
 void configure(float size, float damping, float colour) {
  colour = juce::jlimit(0.f, 1.f, colour); sweepDepth = juce::jmin(14.f, 30.f * colour) * k; drive = juce::jmax(0.f, colour - .4f) * 4.f;
  decay = .5f + .47f * juce::jlimit(0.f, 1.f, size); damp = 1.f - std::exp(-juce::MathConstants<float>::twoPi * 14000.f * std::pow(.14f, juce::jlimit(0.f, 1.f, damping)) / sr); }
 void process(float x, float& left, float& right) {
  float in = x;
  for (size_t i = 0; i < 4; ++i) in = allpass(input[i], in, i < 2 ? .75f : .625f, inLength[i]);
  const float tailA = dA2.read(at(3720)), tailB = dB2.read(at(3163));
  phase += 1.f / sr; if (phase >= 1.f) phase -= 1.f;
  const float swing = 2.f * phase - 1.f, sweep = sweepDepth * 4.f * swing * (1.f - std::abs(swing)); // slow, smooth movement inside the first all-pass of each loop
  float a = allpass(apA1, in + decay * tailB, .7f, 672 * k + 14 * k + sweep);
  const float a1 = dA1.read(at(4453)); dA1.write(shaped(a, drive)); dampA += damp * (a1 - dampA);
  dA2.write(allpass(apA2, dampA * decay, .5f, 1800 * k));
  float b = allpass(apB1, in + decay * tailA, .7f, 908 * k + 14 * k - sweep);
  const float b1 = dB1.read(at(4217)); dB1.write(shaped(b, drive)); dampB += damp * (b1 - dampB);
  dB2.write(allpass(apB2, dampB * decay, .5f, 2656 * k));
  left = .6f * (dA1.read(at(266)) + dA1.read(at(2974)) - apB2.read(at(1913)) + dB2.read(at(1996)) - dB1.read(at(1990)) - apA2.read(at(187)) - dA2.read(at(1066)));
  right = .6f * (dB1.read(at(353)) + dB1.read(at(3627)) - apA2.read(at(1228)) + dA2.read(at(2673)) - dA1.read(at(2111)) - apB2.read(at(335)) - dB2.read(at(121)));
 }
private:
 int at(float referenceSamples) const { return juce::jmax(1, int(referenceSamples * k)); }
 std::array<Line, 4> input; std::array<float, 4> inLength{};
 Line apA1, dA1, apA2, dA2, apB1, dB1, apB2, dB2;
 float sr = 44100.f, k = 1.f, decay = .7f, damp = .5f, dampA = 0, dampB = 0, phase = 0, sweepDepth = 12.f, drive = 0;
};

} // namespace orbitfx

// The reverb stage of the instrument: adds the selected reverb on top of the dry signal.
class OrbitReverb {
public:
 enum Type { vintage = 0, plate = 1 };
 void prepare(double sampleRate) { hall.prepare(sampleRate); tank.prepare(sampleRate); wet.reset(sampleRate, .05); wet.setCurrentAndTargetValue(0.f); running = false; }
 void reset() { hall.clear(); tank.clear(); running = false; }
 void process(juce::AudioBuffer<float>& buffer, int type, float size, float damping, float colour, float amount) {
  if (buffer.getNumChannels() < 2) return;
  if (type != lastType) { hall.clear(); tank.clear(); lastType = type; } // never let one reverb's tail leak into the other
  wet.setTargetValue(juce::jlimit(0.f, 1.f, amount) * 1.4f);
  if (amount <= 0.f && !wet.isSmoothing()) { if (running) reset(); return; } // level at zero: no cost, and no stale tail later
  running = true;
  if (type == plate) tank.configure(size, damping, colour); else hall.configure(size, damping, colour);
  auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);
  for (int i = 0; i < buffer.getNumSamples(); ++i) {
   float wetL = 0, wetR = 0; const float in = .5f * (left[i] + right[i]);
   if (type == plate) tank.process(in, wetL, wetR); else hall.process(left[i], right[i], wetL, wetR);
   const float level = wet.getNextValue(); left[i] += level * wetL; right[i] += level * wetR;
  }
 }
private:
 orbitfx::VintageHall hall; orbitfx::Plate tank; juce::SmoothedValue<float> wet; int lastType = -1; bool running = false;
};
