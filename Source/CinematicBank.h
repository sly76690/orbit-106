#pragma once
#include <juce_core/juce_core.h>
#include <CinemaData.h>

// Embedded, portable add-on bank. Existing factory programs and disk sounds keep their IDs.
class CinematicBank {
public:
 static juce::String nameAt(int index) {
  return juce::String::fromUTF8(OrbitCinemaData::originalFilenames[index]).upToLastOccurrenceOf(".orbitpreset", false, false);
 }
 static juce::StringArray names() {
  juce::StringArray result;
  for(int i=0;i<OrbitCinemaData::namedResourceListSize;++i)result.add(nameAt(i));
  result.sort(true);return result;
 }
 static bool contains(const juce::String& name) {
  for(int i=0;i<OrbitCinemaData::namedResourceListSize;++i)if(nameAt(i)==name)return true;
  return false;
 }
 static std::unique_ptr<juce::XmlElement> load(const juce::String& name) {
  for(int i=0;i<OrbitCinemaData::namedResourceListSize;++i)if(nameAt(i)==name){
   int size=0;const auto* data=OrbitCinemaData::getNamedResource(OrbitCinemaData::namedResourceList[i],size);
   return data?juce::XmlDocument::parse(juce::String::fromUTF8(data,size)):nullptr;
  }
  return {};
 }
};
