/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// [LUCAS] : This enum defines and represents the slope of the filters
enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// [LUCAS] : This structure holds the parameters of the EQ
struct ChainSettings
{
    float peakFreq { 0 }, peakGainInDb { 0 }, peakQ { 1.f };
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};

// [LUCAS] : This function is a getter for the parameters settings of the EQ.
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

    // [LUCAS] : This enum defines and represents the positions of
    //           the different filter types within the MonoChain :
    //           [0] LowCut [1] Peak [2] HighCut  
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    // [LUCAS] : This defines an alias type, `Filter`,
    //           for a single-precision float IIR filter
    using Filter = juce::dsp::IIR::Filter<float>;

    // [LUCAS] : This defines an alias type, `Coefficients`,
    //           for a Filter::CoefficientsPtr    
    using Coefficients = Filter::CoefficientsPtr;

    // [LUCAS] : This defines an alias type, `CutFilter`,
    //           for a chain of four IIR filters
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // [LUCAS] : This defines a MonoChain as a chain containing :
    //           a CutFilter, a single IIR filter, and another CutFilter
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // [LUCAS] : Declaration of left and right MonoChains for processing stereo audio
    MonoChain leftChain, rightChain;

    // [LUCAS] : This template helper function updates the coefficients of
    //           an index-specific filter within a processing chain
    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        // [LUCAS] : Updates the coefficients of the filter at the given index in the chain 
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);

        // [LUCAS] : Sets the filer bypass status to false (i.e., activate the filter)
        chain.template setBypassed<Index>(false);
    }

    // [LUCAS] : This function updates all the filters in the audio processing chains :
    //           low cut filter, peak filter, and high cut filter
    //           based on the current parameters
    void updateFilters();

    // [LUCAS] : This function updates the peak filter coefficients of the
    //           left and right MonoChains based on the current chainSettings
    void updatePeakFilter(const ChainSettings& chainSettings);

    // [LUCAS] : This function updates the low cut filter coefficients of the
    // left and right MonoChains based on the current chainSettings
    void updateLowCutFilter(const ChainSettings& chainSettings);

    // [LUCAS] : This function updates the high cut filter coefficients of the
    // left and right MonoChains based on the current chainSettings
    void updateHighCutFilter(const ChainSettings& chainSettings);

    // [LUCAS] : This template function updates the coefficients of
    //           a CutFilter within a processing chain, based on the given slope setting
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(
        ChainType& chain,
        const CoefficientType& cutCoefficients,
        const Slope& slope)
    {
        // [LUCAS] : Bypasses all filters in the CutFilter chain
        chain.template setBypassed<0>(true);
        chain.template setBypassed<1>(true);
        chain.template setBypassed<2>(true);
        chain.template setBypassed<3>(true);

        // [LUCAS] : Updates and activate the filters that contribute to the desired slope
        switch (slope)
        {
            case Slope_48:
                update<3>(chain, cutCoefficients);
            case Slope_36:
                update<2>(chain, cutCoefficients);
            case Slope_24:
                update<1>(chain, cutCoefficients);
            case Slope_12:
                update<0>(chain, cutCoefficients);
        }
    }

    // [LUCAS] : This helper function updates the old Coefficients
    //           with the new updated coefficients
    static void updateCoefficients(Coefficients& old, const Coefficients& updated);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
