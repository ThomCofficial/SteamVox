#include "PluginEditor.h"
#include <cmath>
#ifndef STEAMVOX_HAS_ARTWORK
#define STEAMVOX_HAS_ARTWORK 0
#endif
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
    const auto& picture = (w > 125 ? big : small);
    if (picture.isValid())
    {
        const auto c = bounds.getCentre();
        const auto angle = startAngle + position * (endAngle - startAngle);
        const auto scale = std::min (bounds.getWidth() / picture.getWidth(),
                                     bounds.getHeight() / picture.getHeight());
        const auto transform = juce::AffineTransform::translation (
            -picture.getWidth() * .5f, -picture.getHeight() * .5f)
            .scaled (scale).rotated (angle - (startAngle + endAngle) * .5f)
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
#if STEAMVOX_HAS_ARTWORK
    osButton.setButtonText ({});
    osButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    osButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
#endif
    osButton.setTooltip ("Enable real 2x oversampling for nonlinear processing");
    osAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.parameters, "oversampling", osButton);
#if ! STEAMVOX_HAS_ARTWORK
    osButton.setColour (juce::TextButton::buttonColourId,teal);
#endif
    osButton.setColour (juce::TextButton::textColourOffId,ivory);
    addAndMakeVisible (osButton);
    setResizable (true,true);
#if STEAMVOX_HAS_ARTWORK
    setResizeLimits (550,500,1200,1100);
    setSize (752,681);
#else
    setResizeLimits (520,480,1100,1000);
    setSize (720,660);
#endif
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
        const auto W = (float) getWidth(), H = (float) getHeight();
        constexpr float designW = 1254.f, designH = 1135.f;
        const float scale = std::min (W / designW, H / designH);
        const auto area = juce::Rectangle<float> ((W-designW*scale)*.5f,
             (H-designH*scale)*.5f,designW*scale,designH*scale);
        g.drawImage (face, area, juce::RectanglePlacement::stretchToFit);
        static const auto needle = juce::ImageFileFormat::loadFrom (
            BinaryData::steamvox_vu_needle_png, BinaryData::steamvox_vu_needle_pngSize);
        const auto pivot = area.getTopLeft() +
            juce::Point<float> (designW*.51f*scale,designH*.395f*scale);
        const auto dB = 20.f * std::log10 (std::max (.00001f, meter));
        const auto position = juce::jlimit (0.f,1.f,(dB+20.f)/23.f);
        const auto angle = juce::degreesToRadians (-54.f+108.f*position);
        if (needle.isValid())
        {
            const float factor = 153.f*scale/(float)needle.getHeight();
            const auto transform = juce::AffineTransform::translation (
                -(float)needle.getWidth()*.5f, -(float)needle.getHeight()*.965f)
                .scaled (factor).rotated (angle).translated (pivot.x,pivot.y);
            g.drawImageTransformed (needle,transform,true);
        }
        else
        {
            const auto tip = pivot + juce::Point<float> (std::sin(angle)*153.f*scale,
                                                       -std::cos(angle)*153.f*scale);
            g.setColour (juce::Colour (0xff513526));
            g.drawLine (pivot.x,pivot.y,tip.x,tip.y,2.f*scale);
        }
        const bool os = processor.parameters.getRawParameterValue ("oversampling")->load() > .5f;
        const auto light = area.getTopLeft() + juce::Point<float> (315.f*scale,935.f*scale);
        g.setColour (os ? juce::Colour (0xfff4ad38) : juce::Colour (0xff513d28));
        g.fillEllipse (light.x-8.f*scale,light.y-8.f*scale,16.f*scale,16.f*scale);
        if (os)
        {
            g.setColour (juce::Colour (0xfff5c778).withAlpha (.27f));
            g.fillEllipse (light.x-14.f*scale,light.y-14.f*scale,28.f*scale,28.f*scale);
        }
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
#if STEAMVOX_HAS_ARTWORK
    constexpr float designW = 1254.f, designH = 1135.f;
    const float W = (float)getWidth(), H = (float)getHeight();
    const float scale = std::min(W/designW,H/designH);
    const float left = (W-designW*scale)*.5f, top=(H-designH*scale)*.5f;
    const float centers[6][2] = {
        {175.f,400.f},{1070.f,400.f},{625.f,925.f},
        {278.f,650.f},{627.f,650.f},{974.f,650.f}
    };
    const float diameters[6] = {95.f,95.f,72.f,188.f,188.f,188.f};
    for(size_t i=0;i<controls.size();++i)
    {
        const float d=diameters[i]*scale;
        controls[i].setBounds (juce::roundToInt(left+(centers[i][0]-diameters[i]*.5f)*scale),
            juce::roundToInt(top+(centers[i][1]-diameters[i]*.5f)*scale),
            juce::roundToInt(d),juce::roundToInt(d));
        legends[i].setVisible(false);
    }
    osButton.setBounds(juce::roundToInt(left+210.f*scale),
        juce::roundToInt(top+899.f*scale),juce::roundToInt(130.f*scale),
        juce::roundToInt(68.f*scale));
#else
    const float W=(float)getWidth(),H=(float)getHeight();
    const float centers[][2]={{.12f,.29f},{.88f,.29f},{.50f,.87f},
                              {.22f,.64f},{.50f,.64f},{.78f,.64f}};
    for(size_t i=0;i<controls.size();++i)
    {
        const float d=std::min(W,H)*(i<3?.112f:.185f);
        const float x=W*centers[i][0]-d*.5f,y=H*centers[i][1]-d*.5f;
        controls[i].setBounds(juce::roundToInt(x),juce::roundToInt(y),
            juce::roundToInt(d),juce::roundToInt(d));
        legends[i].setBounds(juce::roundToInt(x-d*.16f),juce::roundToInt(y+d*.86f),
            juce::roundToInt(d*1.32f),24);
    }
    osButton.setBounds(juce::roundToInt(W*.83f),juce::roundToInt(H*.85f),
        juce::roundToInt(W*.10f),30);
#endif
}
juce::AudioProcessorEditor* SteamVoxAudioProcessor::createEditor()
{
    return new SteamVoxEditor (*this);
}
