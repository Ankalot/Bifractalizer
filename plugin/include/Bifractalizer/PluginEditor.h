#pragma once

#include "PluginProcessor.h"
#include "KnobElement.h"
#include "TexturedButton.h"


namespace audio_plugin {
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)

  // -~-~-~-~-~-~-~-~-~-~- Textures -~-~-~-~-~-~-~-~-~-~-
  juce::File assetsDir;
  juce::Image backgroundImage;

  // -~-~-~-~-~-~-~-~-~-~- Buttons -~-~-~-~-~-~-~-~-~-~-
  TexturedButton modeButton;
  //std::unique_ptr<CircleEffectComponent> circleEffect; 
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment;
  juce::Label fractalizerLabel;

  // -~-~-~-~-~-~-~-~-~-~- Knobs -~-~-~-~-~-~-~-~-~-~-
  juce::Slider freqSlider, phaseSlider, gainSlider;
  juce::Label freqLabel, phaseLabel, gainLabel;
  std::unique_ptr<KnobElement> freqKnobElement, phaseKnobElement, gainKnobElement;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment, 
    phaseAttachment, gainAttachment;

  void setupKnob(juce::Slider& slider, float min, float max, float dval, int numDec,
    std::unique_ptr<KnobElement>& knobElement, juce::Label& label, const std::string& labelText,
    const std::string& suffix = "");


};
}  // namespace audio_plugin