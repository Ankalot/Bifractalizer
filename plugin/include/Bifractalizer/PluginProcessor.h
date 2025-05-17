#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>

using FloatSolver = Eigen::SparseLU<Eigen::SparseMatrix<float>>;
//using FloatSolver = Eigen::BiCGSTAB<Eigen::SparseMatrix<float>>;


namespace audio_plugin {
class AudioPluginAudioProcessor : public juce::AudioProcessor {
public:
  AudioPluginAudioProcessor();
  ~AudioPluginAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void setBypassed(bool new_bypass) {bypass = new_bypass;}
  bool getBypassed() {return bypass;}

  juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)

  juce::AudioProcessorValueTreeState apvts;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

  bool bypass = false;

  juce::AudioBuffer<float> inputBuffer, outputBuffer, 
    processInBuffer, processOutBuffer, prevBuffer;
  int inBufPos, outBufPosRead, outBufPosWrite, prevBlockOffset, prevHostBlockSize;
  float previousGain = 1.0f;
  // -~-~-~-~-~-~-~-~-~-~ vs clicks when changing settings -~-~-~-~-~-~-~-~-~-
  int buffers2Update = 0;
  int need2UpdateBuffers = 0;
  // -~-~-~-~-~-~-~-~-~-~-~-~-~--~-~-~-~-~-~-~-~-~-~-~-~-~--~-~-~-~-~-~-~-~-~-

  // -~-~-~-~-~-~-~-~-~-~-~-~-~ for </de>fractalizer -~-~-~-~-~-~-~-~-~-~-~-~-
  int processingN, max_terms = 20;
  std::vector<float> xGrid, two_pow_n, weights;
  // -~-~-~-~-~-~-~-~-~-~-~-~-~- for defractalizer -~-~-~-~-~-~-~-~-~-~-~-~-~-
  bool actualDefrMatrix = false; 
  Eigen::SparseMatrix<float> defrMatrix;
  FloatSolver defrSolver;
  juce::ThreadPool threadPool;
  std::atomic<bool> solverReady{true};
  void prepareDefractalizer();
  // -~-~-~-~-~-~-~-~-~-~-~-~-~--~-~-~-~-~-~-~-~-~-~-~-~-~--~-~-~-~-~-~-~-~-~-

  int getBlockSize() const;
  int getBlockOffset() const;
  float getGain() const;
  void updateBuffers(int hostBlockSize);
  void processCustomBlock();
};
}  // namespace audio_plugin