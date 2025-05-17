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
  juce::Image backgroundImage;

  // -~-~-~-~-~-~-~-~-~-~- Buttons -~-~-~-~-~-~-~-~-~-~-
  TexturedButton modeButton;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment;
  juce::Label fractalizerLabel;

  // -~-~-~-~-~-~-~-~-~-~- Knobs -~-~-~-~-~-~-~-~-~-~-
  juce::Slider freqSlider, phaseSlider, gainSlider, alphaSlider, betaSlider;
  juce::Label freqLabel, phaseLabel, gainLabel, alphaLabel, betaLabel;
  std::unique_ptr<KnobElement> freqKnobElement, phaseKnobElement, gainKnobElement,
    alphaKnobElement, betaKnobElement;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment, 
    phaseAttachment, gainAttachment, alphaAttachment, betaAttachment;

  void setupKnob(juce::Slider& slider, float min, float max, float dval, int numDec,
    std::unique_ptr<KnobElement>& knobElement, juce::Label& label, const std::string& labelText,
    const std::string& suffix = "", bool mini = false);


};
}  // namespace audio_plugin