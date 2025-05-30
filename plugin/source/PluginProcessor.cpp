#include "Bifractalizer/PluginProcessor.h"
#include "Bifractalizer/PluginEditor.h"
#include "bifractalizer.cpp"

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>


namespace audio_plugin {
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ), apvts(*this, nullptr, "Parameters", createParameters()),
      defrSolver() {
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters() {
  juce::AudioProcessorValueTreeState::ParameterLayout params;
  
  params.add(std::make_unique<juce::AudioParameterFloat>(
      "frequency",
      "Frequency",
      juce::NormalisableRange<float>(20.0f, 350.0f, 0.1f),
      93.8f,
      juce::AudioParameterFloatAttributes().withLabel("frequency")
  ));

  params.add(std::make_unique<juce::AudioParameterFloat>(
      "blockOffset",
      "Block Offset",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
      0.0f,
      juce::AudioParameterFloatAttributes().withLabel("fraction")
  ));

  params.add(std::make_unique<juce::AudioParameterChoice>(
      "mode",
      "Mode",
      juce::StringArray {"Fractalizer", "Defractalizer"}, 
      0
  ));

  params.add(std::make_unique<juce::AudioParameterFloat>(
      "gain",
      "Gain",
      juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
      0.0f,
      "dB",
      juce::AudioProcessorParameter::genericParameter
  ));

  params.add(std::make_unique<juce::AudioParameterFloat>(
      "alpha",
      "Alpha",
      juce::NormalisableRange<float>(0.0f, 0.90f, 0.01f),
      0.5f
  ));

  params.add(std::make_unique<juce::AudioParameterInt>(
      "beta",
      "Beta",
      2, 8, 2
  ));
  
  return params;
}

const juce::String AudioPluginAudioProcessor::getName() const {
  return "Bifractalizer";//JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
             // programs, so this should be at least 1, even if you're not
             // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() {
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

int closestPowerOf2(int x) {
  if (x <= 0) return 0;  // Undefined for non-positive numbers
  
  double log2x = std::log2(x);
  int lowerExp = static_cast<int>(std::floor(log2x));
  int upperExp = lowerExp + 1;
  
  int lowerPow = 1 << lowerExp;      // 2^lowerExp (bit-shift for efficiency)
  int upperPow = 1 << upperExp;      // 2^upperExp
  
  // Check which is closer
  if (x - lowerPow <= upperPow - x) {
      return lowerPow;
  } else {
      return upperPow;
  }
}

void AudioPluginAudioProcessor::updateCoeffs() {
  const float alpha = static_cast<float>(*apvts.getRawParameterValue("alpha"));
  const int beta = static_cast<int>(*apvts.getRawParameterValue("beta"));

  if (beta != prevBeta) {
    max_terms = static_cast<int>(ceil(log(1000001)/log(beta)));
  }

  prevAlpha = alpha;
  prevBeta = beta;

  beta_pow_n.resize(max_terms), weights.resize(max_terms);
  beta_pow_n[0] = 1.0f;
  weights[0] = 1.0f;
  for (int n = 1; n < max_terms; ++n) {
      beta_pow_n[n] = beta_pow_n[n-1] * beta;
      weights[n] = weights[n-1] * alpha;
  }

  actualDefrMatrix = false;
}

void AudioPluginAudioProcessor::updateBuffers(int hostBlockSize) {
  // I am not 100% sure that these are the minimal outBufPosWrite and outBufSize possible
  //    but they work and and empirically I couldn't find a better option 
  
  inBufPos = getBlockOffset();
  prevBlockOffset = inBufPos;

  int blockSizeVal = getBlockSize();
  int outBufSize = static_cast<int>(std::ceil(static_cast<double>(hostBlockSize) / 
                    blockSizeVal) + (hostBlockSize % blockSizeVal == 0 ? 0 : 1)) * blockSizeVal + inBufPos;

  int numChannels = getTotalNumInputChannels();
  inputBuffer.setSize(numChannels, blockSizeVal);
  inputBuffer.clear();
  outputBuffer.setSize(numChannels, outBufSize);
  outputBuffer.clear();

  outBufPosRead = 0;
  if (hostBlockSize % (outBufSize - inBufPos) == 0)
    outBufPosWrite = outBufPosRead;
  else
    outBufPosWrite = (outBufPosRead + blockSizeVal) % outBufSize;

  int prevProcessingN = processingN;
  //processingN = closestPowerOf2(blockSizeVal); INTERPOLATION FORTH AND BACK = 💀 💀 💀
  processingN = blockSizeVal;
  processInBuffer.setSize(numChannels, processingN);
  processOutBuffer.setSize(numChannels, processingN);

  prevHostBlockSize = hostBlockSize;

  setLatencySamples(outBufPosWrite);
  updateHostDisplay();

  if (prevProcessingN != processingN) {
    // -~-~-~-~-~-~-~-~-~-~-~-~-~ for </de>fractalizer -~-~-~-~-~-~-~-~-~-~-~-~-
    xGrid = linspace(0, 1.0f, processingN, false);
    // -~-~-~-~-~-~-~-~-~-~-~-~-~- for defractalizer -~-~-~-~-~-~-~-~-~-~-~-~-~-
    actualDefrMatrix = false;
  }
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialisation that you need..
  juce::ignoreUnused(sampleRate);

  outBufPosRead = 0;    // for audio repeatability
  outputBuffer.clear(); // for audio repeatability
  prevBlockOffset = -1; // so inBufPos will be changed
  processingN = -1;
  updateCoeffs();
  updateBuffers(samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
  inputBuffer.clear();
  outputBuffer.clear();
  processInBuffer.clear();
  processOutBuffer.clear();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

int AudioPluginAudioProcessor::getBlockSize() const {
  return static_cast<int>(round(getSampleRate() / *apvts.getRawParameterValue("frequency")));
}

int AudioPluginAudioProcessor::getBlockOffset() const {
  int blockSizeVal = getBlockSize();
  return static_cast<int>(round(blockSizeVal*
          static_cast<float>(*apvts.getRawParameterValue("blockOffset")))) % blockSizeVal;
}

float AudioPluginAudioProcessor::getGain() const {
  return juce::Decibels::decibelsToGain(static_cast<float>(*apvts.getRawParameterValue("gain")));
}

void fadeOutBuffer(juce::AudioBuffer<float>& buffer) {
  const int numCh = buffer.getNumChannels();
  const int numSmpls = buffer.getNumSamples();
  for (int ch = 0; ch < numCh; ++ch) {
    float* buffer_data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSmpls; ++i) {
      // Cosine fade-out curve (1 → 0)
      float t = 1 - (float)i / (float)(numSmpls - 1);
      float fade = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * t);
      buffer_data[i] *= fade;
    }
  }
}

void fadeInBuffer(juce::AudioBuffer<float>& buffer) {
  const int numCh = buffer.getNumChannels();
  const int numSmpls = buffer.getNumSamples();
  for (int ch = 0; ch < numCh; ++ch) {
    float* buffer_data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSmpls; ++i) {
      // Cosine fade-in curve (0 → 1)
      float t = (float)i / (float)(numSmpls - 1);
      float fade = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::pi * t);
      buffer_data[i] *= fade;
    }
  }
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (prevAlpha != static_cast<float>(*apvts.getRawParameterValue("alpha")) ||
      prevBeta != static_cast<int>(*apvts.getRawParameterValue("beta"))) {
    updateCoeffs();
  }

  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();

  int hostBlockSize = buffer.getNumSamples();
  int blockSizeVal = getBlockSize();
  int outBufSize = static_cast<int>(std::ceil(static_cast<double>(hostBlockSize) / 
                    blockSizeVal) + (hostBlockSize % blockSizeVal == 0 ? 0 : 1)) * blockSizeVal + getBlockOffset();
  int blockOffset = getBlockOffset();

  int need2UpdateBuffersPrev = need2UpdateBuffers;
  if (need2UpdateBuffers != 0) {
    // WE HAVE ADDITIONAL LATENCY ON CHANGING SETTINGS (hostBlockSize SAMPLES)
    // FOR AVOIDING CLICKS
    updateBuffers(hostBlockSize);
    need2UpdateBuffers = 0;
  }

  if (blockSizeVal != inputBuffer.getNumSamples() || 
      outBufSize != outputBuffer.getNumSamples() ||
      prevBlockOffset != blockOffset ||
      prevHostBlockSize != hostBlockSize) {
    need2UpdateBuffers = 1 + need2UpdateBuffersPrev;
    // I AM 80% SURE THAT THIS IS THE MIN VALUE POSSIBLE FOR NO CLICKS
    buffers2Update = static_cast<int>(ceil(blockSizeVal / hostBlockSize)) + 3;
  }

  blockSizeVal = inputBuffer.getNumSamples();
  outBufSize = outputBuffer.getNumSamples();

  int bufPos = 0;
  while (bufPos < hostBlockSize) {
    int samplesToProcess = std::min(blockSizeVal - inBufPos, hostBlockSize - bufPos);

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
      const float* hostBufferPtr = buffer.getReadPointer(channel, bufPos);
      float* inputBufferPtr = inputBuffer.getWritePointer(channel, inBufPos);
      std::memcpy(inputBufferPtr, hostBufferPtr, sizeof(float) * samplesToProcess);
    }

    bufPos += samplesToProcess;
    inBufPos += samplesToProcess;
    if (inBufPos == blockSizeVal) {
      processCustomBlock();
      outBufPosWrite = (outBufPosWrite + blockSizeVal) % outBufSize;
      inBufPos = 0;
    }
  }

  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    const float* outputBufferPtr = outputBuffer.getReadPointer(channel, outBufPosRead);
    float* hostBufferPtr = buffer.getWritePointer(channel, 0);
    int samplesToCopy = std::min(outBufSize - outBufPosRead, hostBlockSize);
    std::memcpy(hostBufferPtr, outputBufferPtr, sizeof(float) * samplesToCopy);
    if (samplesToCopy < hostBlockSize) {
      const float* outputBufferPtr_ = outputBuffer.getReadPointer(channel, 0);
      float* hostBufferPtr_ = buffer.getWritePointer(channel, samplesToCopy);
      std::memcpy(hostBufferPtr_, outputBufferPtr_, sizeof(float) * (hostBlockSize - samplesToCopy));
    }
  }
  outBufPosRead = (outBufPosRead + hostBlockSize) % outBufSize;

  // ===================================== APPLY GAIN (smooth) =====================================
  const float targetGain = getGain();
  buffer.applyGainRamp(0, hostBlockSize, previousGain, targetGain);
  previousGain = targetGain;
  // ============================ AVOIDING CLICKS AFTER UPDATING BUFFERS ===========================
  if (buffers2Update != 0) {
    if (need2UpdateBuffers >= 1)
      fadeOutBuffer(buffer);
    if ((buffers2Update == 1) || (need2UpdateBuffers > 1))
      fadeInBuffer(buffer);
    if (buffers2Update > 1 && need2UpdateBuffers == 0)
      buffer.applyGain(0.0f);
    buffers2Update -= 1;
  }
  prevBuffer.makeCopyOf(buffer);
}

void AudioPluginAudioProcessor::prepareDefractalizer() {
  findDefractalizerMatrix(defrMatrix, beta_pow_n, weights, processingN, max_terms);

  solverReady.store(false);
  threadPool.addJob([this] {
    defrSolver.analyzePattern(defrMatrix);
    defrSolver.factorize(defrMatrix);
    solverReady.store(true);
  });

  //defrSolver.setMaxIterations(max_terms*2);
  //defrSolver.compute(defrMatrix);
  actualDefrMatrix = true;
}

void AudioPluginAudioProcessor::processCustomBlock() {
  // ======================================================================================================
  // ========================================== AUDIO PROCESSING ==========================================
  // ======================================================================================================
  for (int ch = 0; ch < getNumInputChannels(); ++ch) {
    processInBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, processingN);
  }
  
  if (bypass) {
    for (int ch = 0; ch < getNumInputChannels(); ++ch)
      processOutBuffer.copyFrom(ch, 0, processInBuffer, ch, 0, processingN);
  } else {
    if (apvts.getRawParameterValue("mode")->load()==0) {
      fractalize(xGrid, processInBuffer, processOutBuffer, beta_pow_n, weights, max_terms);
    } else {
      if (!actualDefrMatrix && solverReady) {
        prepareDefractalizer();
      }
      if (solverReady) {
        defractalize(processInBuffer, processOutBuffer, defrMatrix, defrSolver);
      } else {
        for (int ch = 0; ch < getNumInputChannels(); ++ch) {
          processOutBuffer.copyFrom(ch, 0, processInBuffer, ch, 0, processingN);
        }
      }
    }
  }

  for (int ch = 0; ch < getNumInputChannels(); ++ch) {
    inputBuffer.copyFrom(ch, 0, processOutBuffer, ch, 0, processingN);
  }
  // ======================================================================================================
  // ======================================================================================================
  // ======================================================================================================

  int totalNumInputChannels = inputBuffer.getNumChannels();
  int blockSizeVal = inputBuffer.getNumSamples();
  int outBufSize = outputBuffer.getNumSamples();

  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    const float* inputBufferPtr = inputBuffer.getReadPointer(channel, 0);
    float* outputBufferPtr = outputBuffer.getWritePointer(channel, outBufPosWrite);
    int samplesToCopy = std::min(outBufSize - outBufPosWrite, blockSizeVal);
    std::memcpy(outputBufferPtr, inputBufferPtr, sizeof(float) * samplesToCopy);
    if (samplesToCopy < blockSizeVal) {
      const float* inputBufferPtr_ = inputBuffer.getReadPointer(channel, samplesToCopy);
      float* outputBufferPtr_ = outputBuffer.getWritePointer(channel, 0);
      std::memcpy(outputBufferPtr_, inputBufferPtr_, sizeof(float) * (blockSizeVal - samplesToCopy));
    }
  }
}

bool AudioPluginAudioProcessor::hasEditor() const {
  return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
  return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data,
                                                    int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
}  // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}