#pragma once

#include <juce_audio_processors/juce_audio_processors.h>


class TexturedButton : public juce::ImageButton {
public:
    TexturedButton(const juce::File& assetsDir, const std::string& imgPath) {
        buttonImage = juce::ImageCache::getFromFile(assetsDir.getChildFile(imgPath));
        
        jassert(!buttonImage.isNull());

        setImages(false, true, true,
                  buttonImage, 1.0f, juce::Colours::transparentBlack,
                  buttonImage, 1.0f, juce::Colours::transparentBlack,
                  buttonImage, 0.8f, juce::Colours::transparentBlack);
        
        setClickingTogglesState(true);
    }

    void paint(juce::Graphics& g) override {
        ImageButton::paint(g);
        
        // Draw the text
        g.setColour(juce::Colours::black);
        g.setFont(juce::Font("Comic Sans MS", 60.0f, juce::Font::bold).boldened());
        
        if (getToggleState())
            g.drawText("DE", getLocalBounds(), juce::Justification::centred);
        else
            g.drawText("", getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void clicked() override {
        ImageButton::clicked();
        repaint();
    }

private:
    juce::Image buttonImage;
};

/*#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class CircleEffectComponent : public juce::Component, public juce::Timer {
public:
    CircleEffectComponent() : progress(0.0f), centerPoint(), shouldExpand(true) {
        setInterceptsMouseClicks(false, false);
        setOpaque(false); // Important: mark as non-opaque
    }

    void startEffect(juce::Point<int> center) {
        centerPoint = center;
        progress = 0.0f;
        shouldExpand = true;
        setVisible(true);
        startTimerHz(60);
    }

    void stopEffect() {
        shouldExpand = false;
        startTimerHz(60);
    }

    void paint(juce::Graphics& g) override {
        if (progress <= 0.0f) return;
        setVisible(false);

        auto bounds = getLocalBounds().toFloat();
        float maxRadius = std::sqrt(std::pow(bounds.getWidth(), 2) + std::pow(bounds.getHeight(), 2));
        float currentRadius = maxRadius * progress;
    
        auto localCenter = getLocalPoint(nullptr, centerPoint).toFloat();

        juce::Path circle;
        circle.addEllipse(localCenter.x - currentRadius,
                        localCenter.y - currentRadius,
                        currentRadius * 2,
                        currentRadius * 2);
        
        // Get the parent's image in this component's area
        auto* parent = getParentComponent();
        if (parent == nullptr) {
            setVisible(true);
            return;
        }
            
        juce::Image background = parent->createComponentSnapshot(parent->getLocalArea(this, getLocalBounds()));
        
        setVisible(true);

        // Create a copy to invert
        juce::Image inverted (background.getFormat(), background.getWidth(), background.getHeight(), true);
        juce::Image::BitmapData srcData (background, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData destData (inverted, juce::Image::BitmapData::writeOnly);
        
        // Manually invert each pixel
        for (int y = 0; y < background.getHeight(); ++y)
        {
            for (int x = 0; x < background.getWidth(); ++x)
            {
                auto* srcPixel = srcData.getPixelPointer (x, y);
                auto* destPixel = destData.getPixelPointer (x, y);
                
                if (background.getFormat() == juce::Image::RGB)
                {
                    destPixel[0] = 255 - srcPixel[0]; // R
                    destPixel[1] = 255 - srcPixel[1]; // G
                    destPixel[2] = 255 - srcPixel[2]; // B
                }
                else if (background.getFormat() == juce::Image::ARGB)
                {
                    destPixel[0] = srcPixel[0]; // Alpha (unchanged)
                    destPixel[1] = 255 - srcPixel[1]; // R
                    destPixel[2] = 255 - srcPixel[2]; // G
                    destPixel[3] = 255 - srcPixel[3]; // B
                }
            }
        }
        
        // Apply the inverted image only within our path
        g.reduceClipRegion(circle);
        g.drawImageAt(inverted, 0, 0);
    }

    void timerCallback() override {
        if (isTimerRunning()) {
            if (shouldExpand) {
                progress += 0.05f;
                if (progress >= 1.0f) {
                    progress = 1.0f;
                    stopTimer();
                }
            } else {
                progress -= 0.05f;
                if (progress <= 0.0f) {
                    progress = 0.0f;
                    stopTimer();
                    setVisible(false);
                }
            }
            repaint();
        }
    }

private:
    float progress;
    juce::Point<int> centerPoint;
    bool shouldExpand;
};

class TexturedButton : public juce::ImageButton {
public:
    TexturedButton(const juce::File& assetsDir, const std::string& imgPath) {
        buttonImage = juce::ImageCache::getFromFile(assetsDir.getChildFile(imgPath));
        
        jassert(!buttonImage.isNull());

        setImages(false, true, true,
                buttonImage, 1.0f, juce::Colours::transparentBlack,
                buttonImage, 1.0f, juce::Colours::transparentBlack,
                buttonImage, 0.8f, juce::Colours::transparentBlack);
        
        setClickingTogglesState(true);
    }

    void setEffectComponent(CircleEffectComponent* effectComp) {
        effectComponent = effectComp;
    }

    void paint(juce::Graphics& g) override {
        ImageButton::paint(g);
        
        if (getToggleState()) {
            g.setColour(juce::Colours::black);
            g.setFont(juce::Font("Comic Sans MS", 60.0f, juce::Font::bold).boldened());
            g.drawText("DE", getLocalBounds(), juce::Justification::centred);
        }
    }

    void clicked() override {
        ImageButton::clicked();
        
        if (effectComponent != nullptr) {
            if (getToggleState()) {
                auto center = localPointToGlobal(getLocalBounds().getCentre());
                effectComponent->startEffect(center);
            } else {
                effectComponent->stopEffect();
            }
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

private:
    juce::Image buttonImage;
    CircleEffectComponent* effectComponent = nullptr;
};*/