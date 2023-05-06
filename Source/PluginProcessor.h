/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// [LUCAS] : Define the slope of the filters
enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// [LUCAS] : Struct to hold the parameters of the EQ
struct ChainSettings
{
    float peakFreq { 0 }, peakGainInDb { 0 }, peakQ { 1.f };
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};

// [LUCAS] : Getters for the parameters of the EQ
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& parametersManager);

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
    juce::AudioProcessorValueTreeState parametersManager{
        *this,
        nullptr,
        "Parameters",
        createParameterLayout()
    };

private:

    // [LUCAS] : Define Filter as a single-precision float IIR filter
    using Filter = juce::dsp::IIR::Filter<float>;

    // [LUCAS] : Define CutFilter as a chain of four IIR filters
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // [LUCAS] : Define a MonoChain as a chain containing a CutFilter, a single IIR filter, and another CutFilter
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // [LUCAS] : Declare left and right MonoChains for processing stereo audio
    MonoChain leftChain, rightChain;

    // [LUCAS] : 
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    // [LUCAS] :
    void updatePeakFilter(const ChainSettings& chainSettings);

    // [LUCAS] :
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& updated);

    // [LUCAS] :
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(
        ChainType& leftLowCut,
        const CoefficientType& cutCoefficients,
        const Slope& lowCutSlope)
    {
        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);

        // [LUCAS] : 
        switch (lowCutSlope)
        {
            case Slope_12:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                break;
            }
            case Slope_24:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                break;
            }
            case Slope_36:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
                leftLowCut.template setBypassed<2>(false);
                break;
            }
            case Slope_48:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
                leftLowCut.template setBypassed<2>(false);
                *leftLowCut.template get<3>().coefficients = *cutCoefficients[3];
                leftLowCut.template setBypassed<3>(false);
                break;
            }
        }
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
