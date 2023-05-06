/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// [LUCAS] : Struct to hold the parameters of the EQ
struct ChainSettings
{
    float peakFreq { 0 }, peakGainInDb { 0 }, peakQ { 1.f };
    float loCutFreq { 0 }, hiCutFreq { 0 };
    float loCutSlope { 1 }, hiCutSlope { 1 };
};

// [LUCAS] : Getters for the parameters of the EQ
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& param_manager);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // [LUCAS] : This method creates the parameters of my EQ
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  
    // [LUCAS] : This is the object that manages the parameters of my EQ
    juce::AudioProcessorValueTreeState param_manager{*this, nullptr, "Parameters", createParameterLayout()};

private:

    // [LUCAS] : Define Filter as a single-precision float IIR filter
    using Filter = juce::dsp::IIR::Filter<float>;

    // [LUCAS] : Define CutFilter as a chain of four IIR filters
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // [LUCAS] : Define a MonoChain as a chain containing a CutFilter, a single IIR filter, and another CutFilter
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // [LUCAS] : Declare left and right MonoChains for processing stereo audio
    MonoChain leftChain, rightChain;

    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
