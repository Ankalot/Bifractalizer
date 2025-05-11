#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "BinaryData.h"


class KnobLabel : public juce::Label {
public:
    KnobLabel() : juce::Label() {}

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
        return createIgnoredAccessibilityHandler(*this);
    }

    juce::TextEditor* createEditorComponent() override {
        auto* ed = new juce::TextEditor(getName());
        ed->applyFontToAllText(getLookAndFeel().getLabelFont(*this));
        copyAllExplicitColoursTo(*ed);

        ed->setBorder(juce::BorderSize<int>());
        ed->setIndents(2, 0);
        ed->setJustification(juce::Justification::centred);
        ed->setPopupMenuEnabled(false);

        return ed;
    }
};


class KnobElement : public juce::LookAndFeel_V4 {
public:
    KnobElement() {
        knob = juce::ImageCache::getFromMemory(
            BinaryData::knob_png,          // Resource name (auto-generated)
            BinaryData::knob_pngSize       // Resource size
        );
        knobBackground = juce::ImageCache::getFromMemory(
            BinaryData::knob_outer_png,          // Resource name (auto-generated)
            BinaryData::knob_outer_pngSize       // Resource size
        );
        
        jassert(!knob.isNull());
        jassert(!knobBackground.isNull());
        
        minAngle = -juce::MathConstants<float>::pi * 0.75f;
        maxAngle = juce::MathConstants<float>::pi * 0.75f;
        
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::lightgrey.withAlpha(0.4f));
        setColour(juce::TextEditor::textColourId, juce::Colours::black);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::lightgrey.withAlpha(0.4f));
        setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        setColour(juce::CaretComponent::caretColourId, juce::Colours::black);
    }
    
    void setRotationRange(float newMinAngle, float newMaxAngle) {
        minAngle = newMinAngle;
        maxAngle = newMaxAngle;
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider& slider) override {
        rotaryStartAngle = minAngle;
        rotaryEndAngle = maxAngle;
        
        const float angle = juce::jmap(sliderPosProportional, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);
        
        const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        const auto center = bounds.getCentre();
        
        drawScaledImage(g, knobBackground, bounds);
        
        juce::Graphics::ScopedSaveState state(g);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.addTransform(juce::AffineTransform::rotation(angle, center.x, center.y));
        drawScaledImage(g, knob, bounds);
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override {
        auto layout = LookAndFeel_V4::getSliderLayout(slider);
        if (slider.getTextBoxPosition() == juce::Slider::TextBoxBelow) {
            layout.textBoxBounds.translate(0, -14);
        }
        return layout;
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override {
        auto l = new KnobLabel();
        l->setJustificationType(juce::Justification::centred);
        l->setKeyboardType(juce::TextInputTarget::decimalKeyboard);
        l->setColour(juce::Label::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
        l->setColour(juce::TextEditor::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
        l->setColour(juce::TextEditor::highlightColourId, slider.findColour(juce::Slider::textBoxHighlightColourId));
        return l;
    }

    juce::Font getLabelFont(juce::Label& label) override {
        if (auto* slider = dynamic_cast<juce::Slider*>(label.getParentComponent())) {
            return juce::Font("Comic Sans MS", 20.0f, juce::Font::bold);
        }
        return LookAndFeel_V4::getLabelFont(label);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));

        if (!label.isBeingEdited()) {
            g.setFont(getLabelFont(label));
            g.setColour(label.findColour(juce::Label::textColourId));
            
            // Draw text with exact centering
            auto textBounds = label.getLocalBounds();
            g.drawText(label.getText(),
                       textBounds,
                       label.getJustificationType(),
                       false); // Don't use ellipsis
        }
    }

    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        g.fillAll(juce::Colours::transparentBlack);
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        // do nothing
    }
    
private:
    void drawScaledImage(juce::Graphics& g, const juce::Image& image, 
                        juce::Rectangle<float> bounds) {
        const auto imageSize = juce::Rectangle<float>(0, 0, image.getWidth(), image.getHeight());
        float scale = juce::jmin(bounds.getWidth() / imageSize.getWidth(),
                               bounds.getHeight() / imageSize.getHeight());
        const auto scaledSize = imageSize * scale;
        const auto drawRect = scaledSize.withCentre(bounds.getCentre());
        
        g.drawImage(image, 
                  drawRect.getX(), drawRect.getY(), drawRect.getWidth(), drawRect.getHeight(),
                  0, 0, image.getWidth(), image.getHeight());
    }

    juce::Image knob;
    juce::Image knobBackground;
    float minAngle, maxAngle;
};