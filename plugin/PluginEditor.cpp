#include "PluginEditor.h"
#include <cmath>
#if STEAMVOX_HAS_ARTWORK
#include <BinaryData.h>
#endif

namespace {
const auto brass = juce::Colour (0xffd7b479);
const auto copper = juce::Colour (0xff98704c);
const auto teal = juce::Colour (0xff284948);
const auto ivory = juce::Colour (0xfff1dfbb);
}

void SteamVoxEditor::BrassLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
  float position, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
#if STEAMVOX_HAS_ARTWORK
    static const auto big = juce::ImageFileFormat::loadFrom (
        BinaryData::steamvox_large_knob_png, BinaryData::steamvox_large_knob_pngSize);
    static const auto small = juce::ImageFileFormat::loadFrom (
        BinaryData::steamvox_small_knob_png, BinaryData::steamvox_small_knob_pngSize);
    const auto& picture = (w > 75 ? big : small);
    if (picture.isValid())
    {
        const auto c = bounds.getCentre();
        const auto angle = startAngle + position * (endAngle - startAngle);
        const auto scale = std::min (bounds.getWidth() / picture.getWidth(),
                                     bounds.getHeight() / picture.getHeight());
        const auto transform = juce::AffineTransform::translation (
            -picture.getWidth() * .5f, -picture.getHeight() * .5f)
            .scaled (scale).rotated (angle - juce::MathConstants<float>::pi)
            .translated (c.x, c.y);
        g.drawImageTransformed (picture, transform, true);
        return;
    }
#endif
    auto radius = std::min (bounds.getWidth(), bounds.getHeight()) * .39f;
    auto c = bounds.getCentre();
    juce::Rectangle<float> shadow (c.x - radius, c.y-radius + 3.f, radius * 2, radius * 2);
    g.setColour (juce::Colours::black.withAlpha (.55f)); g.fillEllipse (shadow.expanded (3.f));
    g.setColour (copper); g.drawEllipse (shadow, 3.f);
    juce::ColourGradient gradient (juce::Colour (0xfff4d7a0), c.x-radius, c.y-radius,
                                   juce::Colour (0xff6d4e31), c.x+radius, c.y+radius, false);
    gradient.addColour (.49, brass);
    g.setGradientFill (gradient);
    g.fillEllipse (c.x-radius,c.y-radius,radius*2,radius*2);
    g.setColour (juce::Colour (0xff493728));
    g.drawEllipse (c.x-radius+4,c.y-radius+4,2*radius-8,2*radius-8,2);
    auto angle = startAngle + position*(endAngle-startAngle);
    juce::Path marker;
    marker.addRoundedRectangle (-2.f,-radius*.88f,4.f,radius*.38f,2.f);
    g.setColour (juce::Colour (0xff1b3434));
    g.fillPath (marker,juce::AffineTransform::rotation (angle).translated (c.x,c.y));
    auto arc = juce::Path();
    arc.addCentredArc (c.x,c.y,radius+6,radius+6,0,startAngle,startAngle+(endAngle-startAngle)*position,true);
    g.setColour (juce::Colour (0xffe5c18b));
    g.strokePath (arc,juce::PathStrokeType (2.f));
}

SteamVoxEditor::SteamVoxEditor (SteamVoxAudioProcessor& p)
 : juce::AudioProcessorEditor (&p), processor (p)
{
    const char* ids[] = {"input","output","mix","boiler","steam","blastpipe"};
    const char* names[] = {"IN","OUT","MIX","BOILER","STEAM","BLASTPIPE"};
    for (size_t i = 0; i < controls.size(); ++i)
    {
        auto& s = controls[i];
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox,false,0,0);
        s.setLookAndFeel (&look);
        s.setPopupDisplayEnabled (true,true,this);
        s.setName (names[i]);
        s.setTitle (names[i]);
        addAndMakeVisible (s);
        attachments[i] = std::make_unique<Attachment> (p.parameters, ids[i], s);
        auto& label = legends[i];
        label.setText (names[i],juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (15.f,juce::Font::bold)));
        label.setColour (juce::Label::textColourId, ivory);
        addAndMakeVisible (label);
#if STEAMVOX_HAS_ARTWORK
        label.setVisible (false); // Approved faceplate already contains the labels.
#endif
    }
    osButton.setClickingTogglesState (true);
    osButton.setTooltip ("Enable real 2x oversampling for nonlinear processing");
    osAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.parameters, "oversampling", osButton);
    osButton.setColour (juce::TextButton::buttonColourId,teal);
    osButton.setColour (juce::TextButton::textColourOffId,ivory);
    addAndMakeVisible (osButton);
    setResizable (true,true);
    setResizeLimits (520,480,1100,1000);
    setSize (720,660);
    startTimerHz (30);
}
void SteamVoxEditor::timerCallback()
{
    auto value = processor.meterOutput.load (std::memory_order_relaxed);
    meter = .83f*meter+.17f*value;
    repaint();
}
void SteamVoxEditor::drawMeter (juce::Graphics& g,juce::Rectangle<float> area)
{
    auto c = area.getCentre();
    g.setColour (juce::Colour (0xffedc69d));
    g.fillRoundedRectangle (area,20);
    g.setColour (juce::Colour (0xff5d4d38));
    g.drawRoundedRectangle (area,20,5);
    auto seaY = area.getY()+area.getHeight()*.69f;
    g.setColour (juce::Colour (0xff7f9d96));
    g.fillRect (juce::Rectangle<float> (area.getX()+8,seaY,area.getWidth()-16,area.getBottom()-seaY-8));
    // Two flying seabirds, drawn as scalable vector ornament rather than baked-on controls.
    juce::Path seabird;
    seabird.startNewSubPath (c.x-27,c.y-9);
    seabird.quadraticTo (c.x-8,c.y-22,c.x,c.y-3);
    seabird.quadraticTo (c.x+11,c.y-22,c.x+31,c.y-13);
    g.setColour (juce::Colour (0xff425858));
    g.strokePath (seabird,juce::PathStrokeType (3.5f,juce::PathStrokeType::curved));
    g.setFont (juce::Font (juce::FontOptions (13.f,juce::Font::bold)));
    g.setColour (juce::Colour (0xff4d392c));
    g.drawText ("STEAMVOX  •  VU",area.removeFromTop (25),juce::Justification::centred);
    const auto pivot = juce::Point<float> (c.x,area.getBottom()-13);
    const auto norm = juce::jlimit (0.f,1.f,(float)((20*std::log10 (std::max (.0001f,meter))+36)/42));
    const auto angle = juce::degreesToRadians (-55.f+110.f*norm);
    auto end = pivot+juce::Point<float> (std::sin (angle)*area.getWidth()*.3f,-std::cos (angle)*area.getHeight()*.68f);
    g.setColour (juce::Colour (0xff683b2a));
    g.drawLine (pivot.x,pivot.y,end.x,end.y,2.5f);
    g.fillEllipse (pivot.x-5,pivot.y-5,10,10);
}
void SteamVoxEditor::paint (juce::Graphics& g)
{
#if STEAMVOX_HAS_ARTWORK
    static const auto face = juce::ImageFileFormat::loadFrom (
        BinaryData::steamvox_faceplate_png, BinaryData::steamvox_faceplate_pngSize);
    if (face.isValid())
    {
        g.drawImage (face, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
        const auto W = (float) getWidth(), H = (float) getHeight();
        const auto pivot = juce::Point<float> (W * .555f, H * .379f);
        const auto dbVal = 20.f * std::log10 (std::max (.0001f, meter));
        const auto t = juce::jlimit (0.f, 1.f, (dbVal + 36.f) / 42.f);
        const auto a = juce::degreesToRadians (-48.f + 93.f * t);
        const auto tip = pivot + juce::Point<float> (std::sin (a) * H * .15f,
                                                      -std::cos (a) * H * .15f);
        g.setColour (juce::Colour (0xff422b21));
        g.drawLine (pivot.x, pivot.y, tip.x, tip.y, juce::jmax (1.4f, H*.002f));
        g.fillEllipse (pivot.x-3, pivot.y-3, 6, 6);
        return;
    }
#endif
    g.fillAll (juce::Colour (0xff172c2b));
    auto bounds = getLocalBounds().toFloat().reduced (8.f);
    juce::ColourGradient patina (juce::Colour (0xff42625d),bounds.getTopLeft(),
                                  juce::Colour (0xff193735),bounds.getBottomRight(),false);
    g.setGradientFill (patina);g.fillRoundedRectangle (bounds,25);
    g.setColour (brass);g.drawRoundedRectangle (bounds,25,7.f);
    g.setColour (copper);g.drawRoundedRectangle (bounds.reduced (9.f),19,2.f);
    for (int i=0;i<2;++i)for (int j=0;j<2;++j)
    {
        auto x=i==0?bounds.getX()+22:bounds.getRight()-22;
        auto y=j==0?bounds.getY()+22:bounds.getBottom()-22;
        g.setColour (copper);g.fillEllipse (x-7,y-7,14,14);
        g.setColour (juce::Colour (0xff493728));g.drawLine (x-4,y+3,x+4,y-3,2);
    }
    auto title = juce::Rectangle<float> (bounds.getX()+75,bounds.getY()+17,bounds.getWidth()-150,54);
    g.setColour (juce::Colour (0xff182d2e));g.fillRoundedRectangle (title,10);
    g.setColour (brass);g.drawRoundedRectangle (title,10,3);
    g.setFont (juce::Font (juce::FontOptions (31.f,juce::Font::bold)));
    g.setColour (ivory);g.drawText ("STEAMTRAIN  /  STEAMVOX",title,juce::Justification::centred);
    auto scale = std::min (getWidth()/720.f,getHeight()/660.f);
    juce::Rectangle<float> vu (getWidth()*.24f,getHeight()*.145f,getWidth()*.52f,getHeight()*.31f);
    drawMeter (g,vu);
    g.setColour (brass.withAlpha (.65f));
    for (int i=0;i<3;++i)
    {
        auto x=getWidth()*(.22f+.28f*i);
        g.drawLine (x,getHeight()*.7f,x,getHeight()*.78f,2.f);
    }
    (void)scale;
}
void SteamVoxEditor::resized()
{
    auto W=(float)getWidth(),H=(float)getHeight();
#if STEAMVOX_HAS_ARTWORK
    const float centers[][2] = {
        {.136f,.343f},{.860f,.343f},{.490f,.790f},
        {.221f,.546f},{.490f,.546f},{.763f,.546f}
    };
#else
    const float centers[][2] = {
        {.12f,.29f},{.88f,.29f},{.50f,.87f},
        {.22f,.64f},{.50f,.64f},{.78f,.64f}
    };
#endif
    for (size_t i=0;i<controls.size();++i)
    {
#if STEAMVOX_HAS_ARTWORK
        float size = W * (i < 3 ? .080f : .151f);
#else
        float size = std::min(W,H)*(i<3?.112f:.185f);
#endif
        auto x=W*centers[i][0]-size*.5f,y=H*centers[i][1]-size*.5f;
        controls[i].setBounds (juce::Rectangle<int> ((int)x,(int)y,(int)size,(int)size));
        legends[i].setBounds ((int)(x-size*.16f),(int)(y+size*.86f),(int)(size*1.32f),24);
    }
#if STEAMVOX_HAS_ARTWORK
    osButton.setBounds ((int)(W*.15f),(int)(H*.779f),(int)(W*.13f),(int)(H*.042f));
#else
    osButton.setBounds ((int)(W*.83f),(int)(H*.85f),(int)(W*.10f),30);
#endif
}
juce::AudioProcessorEditor* SteamVoxAudioProcessor::createEditor()
{
    return new SteamVoxEditor (*this);
}
