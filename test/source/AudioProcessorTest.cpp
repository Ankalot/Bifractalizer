// test/source/AudioProcessorTest.cpp
#include <Bifractalizer/PluginProcessor.h>
#include <gtest/gtest.h>
#include <random>


namespace audio_plugin_test {

class AudioProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<audio_plugin::AudioPluginAudioProcessor>();
    }

    std::unique_ptr<audio_plugin::AudioPluginAudioProcessor> processor;
};

// Testing the correctness of block splitting at different frequencies
//     with different phases
TEST_F(AudioProcessorTest, BypassDoesNotCorruptAudio) {
    const int numChannels = 2;
    const int hostBlockSize = 512;
    const int numHostBlocks[] = {  3,   3,   3,   3,   3,   5,   5,    5,   10,   10};
    const int blockSizes[] =    {150, 256, 333, 500, 512, 666, 777, 1024, 1400, 2048};
    const float phases[] =      {0.2, 0.1,   0, 0.4,   0, 0.6, 0.9,  0.3,  0.8,  0.5};
    const int resultOffsets[] = {180,  26, 333, 700,   0,1066,1476, 1331, 2520, 3072};
    const int numIters = sizeof(blockSizes)/4;
    const double sampleRate = 48000;

    processor->setPlayConfigDetails(
        numChannels,
        numChannels,
        sampleRate,
        hostBlockSize
    );

    juce::AudioBuffer<float> inputBuffer, outputBuffer, tempBuffer;
    tempBuffer.setSize(numChannels, hostBlockSize);
    juce::MidiBuffer midiBuffer;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f); 

    processor->setBypassed(true);

    for (int iter = 0; iter < numIters; ++iter) {
        const int numSamples = hostBlockSize*numHostBlocks[iter];
        inputBuffer.setSize(numChannels, numSamples);
        outputBuffer.setSize(numChannels, numSamples);

        for (int ch = 0; ch < numChannels; ++ch) {
            auto* inputBuffer_data = inputBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                inputBuffer_data[i] = dist(gen);
            }
        }

        *processor->getAPVTS().getRawParameterValue("frequency") = sampleRate/blockSizes[iter];
        *processor->getAPVTS().getRawParameterValue("blockOffset") = phases[iter];
        *processor->getAPVTS().getRawParameterValue("gain") = 0.0f;
        processor->prepareToPlay(sampleRate, hostBlockSize);

        for (int i = 0; i < numHostBlocks[iter]; ++i) {
            for (int ch = 0; ch < numChannels; ++ch)
                tempBuffer.copyFrom(ch, 0, inputBuffer, ch, i*hostBlockSize, hostBlockSize);
            processor->processBlock(tempBuffer, midiBuffer);
            for (int ch = 0; ch < numChannels; ++ch)
                outputBuffer.copyFrom(ch, i*hostBlockSize, tempBuffer, ch, 0, hostBlockSize);
        }
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const auto* inputBuffer_data = inputBuffer.getReadPointer(ch);
            const auto* outputBuffer_data = outputBuffer.getReadPointer(ch);
            
            const int offset = resultOffsets[iter];
            for (int i = 0; i < offset; ++i) {
                ASSERT_FLOAT_EQ(outputBuffer_data[i], 0.0f)
                    << "Sample mismatch at channel " << ch << ", sample " << i << ", iter " << iter;
            }
            for (int i = offset; i < numSamples; ++i) {
                ASSERT_FLOAT_EQ(inputBuffer_data[i-offset], outputBuffer_data[i])
                    << "Sample mismatch at channel " << ch << ", sample " << i << ", iter " << iter;
            }
        }
    }
}

void checkFractalizeAndDefractalize(audio_plugin::AudioPluginAudioProcessor *processor, const int hostBlockSize) {
    const int numChannels = 2;
    const double sampleRate = 48000;

    processor->setPlayConfigDetails(
        numChannels,
        numChannels,
        sampleRate,
        hostBlockSize
    );
    processor->setBypassed(false);

    juce::AudioBuffer<float> inputBuffer, outputBuffer;
    juce::MidiBuffer midiBuffer;
    inputBuffer.setSize(numChannels, hostBlockSize);
    outputBuffer.setSize(numChannels, hostBlockSize);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* inputBuffer_data = inputBuffer.getWritePointer(ch);
        for (int i = 0; i < hostBlockSize; ++i) {
            inputBuffer_data[i] = dist(gen);
        }
    }

    *processor->getAPVTS().getRawParameterValue("frequency") = sampleRate/hostBlockSize;
    *processor->getAPVTS().getRawParameterValue("blockOffset") = 0.0f;
    *processor->getAPVTS().getRawParameterValue("gain") = 0.0f;

    // FORWARD AND BACKWARD
    for (int iter = 0; iter < 2; ++iter) {
        for (int ch = 0; ch < numChannels; ++ch) {
            outputBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, hostBlockSize);
        }

        processor->prepareToPlay(sampleRate, hostBlockSize);

        *processor->getAPVTS().getRawParameterValue("mode") = iter * 1.0f;
        processor->processBlock(outputBuffer, midiBuffer);
        *processor->getAPVTS().getRawParameterValue("mode") = 1.0f - iter;
        processor->processBlock(outputBuffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch) {
            const auto* inputBuffer_data = inputBuffer.getReadPointer(ch);
            const auto* outputBuffer_data = outputBuffer.getReadPointer(ch);
            
            for (int i = 0; i < hostBlockSize; ++i) {
                jassert(juce::approximatelyEqual(inputBuffer_data[i], outputBuffer_data[i]));
            }
        }
    }
}

// Testing fractalize and defractalize functions:
//   fractalize audio -> defractalize it -> get initial audio
//   defractalize audio -> fractalize it -> get initial audio
TEST_F(AudioProcessorTest, FractalizeAndDefractalize) {
    const int hostBlockSizes[] = {256, 300, 333, 500, 512, 600, 777, 1024};
    const int numIters = sizeof(hostBlockSizes)/4;
    for (int iter = 0; iter < numIters; ++iter) {
        checkFractalizeAndDefractalize(processor.get(), hostBlockSizes[iter]);
    }
}

}  // namespace audio_plugin_test