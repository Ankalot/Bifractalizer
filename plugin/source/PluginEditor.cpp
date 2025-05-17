#include "Bifractalizer/PluginEditor.h"
#include "Bifractalizer/PluginProcessor.h"


namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p): AudioProcessorEditor(&p), processorRef(p),
      freqAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(
        processorRef.getAPVTS(), "frequency", freqSlider)),
      phaseAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(
        processorRef.getAPVTS(), "blockOffset", phaseSlider)),
      modeButton(),
      modeAttachment(new juce::AudioProcessorValueTreeState::ButtonAttachment(
        processorRef.getAPVTS(), "mode", modeButton)),
      gainAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(
        processorRef.getAPVTS(), "gain", gainSlider)),
      alphaAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(
        processorRef.getAPVTS(), "alpha", alphaSlider)),
      betaAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(
        processorRef.getAPVTS(), "beta", betaSlider))   {

  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::background_jpg,          // Resource name (auto-generated)
      BinaryData::background_jpgSize       // Resource size
  );

  addAndMakeVisible(modeButton);

  fractalizerLabel.setColour(juce::Label::textColourId, juce::Colours::black);
  fractalizerLabel.setFont(juce::Font("Comic Sans MS", 60.0f, juce::Font::bold).boldened());
  fractalizerLabel.setText("FRACTALIZER", juce::dontSendNotification);
  addAndMakeVisible(fractalizerLabel);

  setupKnob(freqSlider, 20.0f, 350.0f, 0.1f, 1, freqKnobElement, freqLabel, "Freq", " Hz");
  setupKnob(phaseSlider, 0.0f, 1.0f, 0.001f, 3, phaseKnobElement, phaseLabel, "Phase");
  setupKnob(gainSlider, -24.0f, 24.0f, 0.1f, 1, gainKnobElement, gainLabel, "Gain", " dB");
  setupKnob(alphaSlider, 0.0f, 0.9f, 0.01f, 2, alphaKnobElement, alphaLabel, "Alpha", "", true);
  setupKnob(betaSlider, 2.0f, 8.0f, 1.0f, 0, betaKnobElement, betaLabel, "Beta", "", true);

  setSize(500, 300);
}

void AudioPluginAudioProcessorEditor::setupKnob(juce::Slider& slider, float min, float max, float dval,
      int numDec, std::unique_ptr<KnobElement>& knobElement, juce::Label& label, 
      const std::string& labelText, const std::string& suffix, bool mini) {
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
  slider.setNumDecimalPlacesToDisplay(numDec);
  slider.setRange(min, max, dval);
  if (suffix != "") {
    slider.setTextValueSuffix(suffix);
  }
  
  slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
  slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
  slider.setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::black.withAlpha(0.3f));
  
  if (mini)
    knobElement = std::make_unique<KnobElement>(-10);
  else
    knobElement = std::make_unique<KnobElement>();
  knobElement->setRotationRange(-juce::MathConstants<float>::pi * 0.75f, 
                                  juce::MathConstants<float>::pi * 0.75f);
  slider.setLookAndFeel(knobElement.get());

  juce::Font labelFont;
  if (mini)
    labelFont = juce::Font("Comic Sans MS", 26.0f, juce::Font::bold);
  else
    labelFont = juce::Font("Comic Sans MS", 30.0f, juce::Font::bold);
  label.setText(labelText, juce::dontSendNotification);
  label.setFont(labelFont);
  label.setColour(juce::Label::textColourId, juce::Colours::black);
  label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(label);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
  freqSlider.setLookAndFeel(nullptr);
  phaseSlider.setLookAndFeel(nullptr);
  gainSlider.setLookAndFeel(nullptr);
  alphaSlider.setLookAndFeel(nullptr);
  betaSlider.setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  if (!backgroundImage.isNull()) {
    g.drawImage(backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
  } else {
    g.fillAll(juce::Colours::darkgrey);
  }
}

void AudioPluginAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  const int fractalizer_y = 40;

  const int knob_size = 120;
  const int knob_y = 140;
  const int knob_x = 25;
  const int knob_dx = knob_size - 10;
  const int knob_label_y = 240;
  const int knob_label_height = 30;
  const int mini_knob_size = 80;
  const int mini_knob_label_dx = 50;


  modeButton.setBounds(55, fractalizer_y, 90, 70);
  fractalizerLabel.setBounds(135, fractalizer_y+5, 300, 60);

  freqSlider.setBounds(knob_x, knob_y, knob_size, knob_size);
  freqLabel.setBounds(knob_x, knob_label_y, knob_size, knob_label_height);

  phaseSlider.setBounds(knob_x + knob_dx, knob_y, knob_size, knob_size);
  phaseLabel.setBounds(knob_x + knob_dx, knob_label_y, knob_size, knob_label_height);

  gainSlider.setBounds(knob_x + 2*knob_dx, knob_y, knob_size, knob_size);
  gainLabel.setBounds(knob_x + 2*knob_dx, knob_label_y, knob_size, knob_label_height);

  alphaSlider.setBounds(knob_x + 3*knob_dx, knob_y-13, mini_knob_size, mini_knob_size);
  alphaLabel.setBounds(knob_x + 3*knob_dx + mini_knob_label_dx + 8, knob_y-3,
     mini_knob_size, knob_label_height);

  betaSlider.setBounds(knob_x + 3*knob_dx, knob_y+57, mini_knob_size, mini_knob_size);
  betaLabel.setBounds(knob_x + 3*knob_dx + mini_knob_label_dx + 4, knob_y+67,
     mini_knob_size, knob_label_height);
}
}  // namespace audio_plugin