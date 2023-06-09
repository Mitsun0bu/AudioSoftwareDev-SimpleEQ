/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // [LUCAS] : Setting the sample rate and the number of samples per block
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = 1;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    // [LUCAS] : Create an AudioBlock<float> wrapper for the input buffer
    juce::dsp::AudioBlock<float> block(buffer);

    // [LUCAS] : Provide separate blocks for left and right channels
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    // [LUCAS] : Provide processing context for each channel
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // [LUCAS] : Process the left and right channels with their respective chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    // [LUCAS] : Un-comment this line to use the SimpleEQAudioProcessorEditor
    // return new SimpleEQAudioProcessorEditor (*this);

    // [LUCAS] : Un-comment this line to use the GenericAudioProcessorEditor
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

// [LUCAS] : This function is a getter for the parameters settings of the EQ.
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& parametersManager)
{
    ChainSettings settings;

    settings.lowCutFreq     = parametersManager.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq    = parametersManager.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq       = parametersManager.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDb   = parametersManager.getRawParameterValue("Peak Gain")->load();
    settings.peakQ          = parametersManager.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope    = static_cast<Slope>(static_cast<int>(parametersManager.getRawParameterValue("LowCut Slope")->load()));
    settings.highCutSlope   = static_cast<Slope>(static_cast<int>(parametersManager.getRawParameterValue("HighCut Slope")->load()));

    return (settings);
}

// [LUCAS] : This function updates the peak filter coefficients for the
//           left and right MonoChains based on the current chainSettings
void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    // [LUCAS] : Create the peak filter coefficients based on the current chainSettings
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQ,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDb)
    );

    // [LUCAS] : Update the peak filter coefficients for the left and right channels
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

// [LUCAS] : This helper function updates the old Coefficients
//           with the new updated coefficients
void SimpleEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& updated)
{
    // [LUCAS] : Assigns the values of the updated coefficients to the old coefficients.
    *old = *updated;
}

// [LUCAS] : This function updates the low cut filters of the left and right chains 
//           based on the current chainSettings
void SimpleEQAudioProcessor::updateLowCutFilter(const ChainSettings &chainSettings)
{
    // [LUCAS] : Calculates the high-pass filter coefficients using the Butterworth method 
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        getSampleRate(),
        2 * (chainSettings.lowCutSlope + 1)
    );

    // [LUCAS] : Updates the coefficients and slope of the low cut filters
    //           in both the left and right chains
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

// [LUCAS] : This function updates the high cut filters of the left and right chains 
//           based on the current chainSettings
void SimpleEQAudioProcessor::updateHighCutFilter(const ChainSettings &chainSettings)
{
    // [LUCAS] : Calculates the low-pass filter coefficients using the Butterworth method 
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        getSampleRate(),
        2 * (chainSettings.highCutSlope + 1)
    );

    // [LUCAS] : Updates the coefficients and slope of the low cut filters
    //           in both the left and right chains
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, cutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, cutCoefficients, chainSettings.highCutSlope);
}

// [LUCAS] : This function updates all the filters in the audio processing chains :
//           low cut filter, peak filter, and high cut filter
//           based on the current parameters
void SimpleEQAudioProcessor::updateFilters()
{
    // Retrieves the chain settings from the parameters manager
    auto chainSettings = getChainSettings(parametersManager);

    // Calls the appropriate update functions to update the filter coefficients and settings
    // in both left and right audio processing chains
    updatePeakFilter(chainSettings);   
    updateLowCutFilter(chainSettings);
    updateHighCutFilter(chainSettings);
}

//[LUCAS] : This method creates the parameters for my EQ
juce::AudioProcessorValueTreeState::ParameterLayout
    SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("LowCut Freq", 1),
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("HighCut Freq", 1),
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Peak Freq", 1),
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        750.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Peak Gain", 1),
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("Peak Quality", 1),
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.0f
    ));
    
    juce::StringArray parametersArray;
    for(int i = 0; i < 4; ++i)
    {
        juce::String parameterString;
        parameterString << (12 + i * 12);
        parameterString << " db/Oct";
        parametersArray.add(parameterString);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("LowCut Slope", 1),
        "LowCut Slope",
        parametersArray,
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("HighCut Slope", 1),
        "HighCut Slope",
        parametersArray,
        0
    ));

    return (layout);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
