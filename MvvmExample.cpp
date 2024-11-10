// JUCE header
#include <juce_gui_basics/juce_gui_basics.h>

// observable-property library: https://github.com/JanWilczek/observable-property
#include <observable_property/ObservableProperty.hpp>
#include <observable_property/juce/LiveObservableProperty.hpp>

// non-existent header, the class is a container for frequencies and  corresponding gains
#include <dsp/MagnitudeResponse.hpp>

// standard library header
#include <memory>

// GoogleTest header for the unit test below
#include <gtest/gtest.h>

// namespaces from observable-property
using namespace wolfsound;
using namespace wolfsound::juce;

class EqFilterViewModel {
public:
  using CutoffFrequencyChangedUseCase =
      std::function<void(double newCutoffFrequencyHz)>;

  explicit EqFilterViewModel(
      CutoffFrequencyChangedUseCase onCutoffFrequencyChanged)
      : frequencySliderValue_{100.},
        cutoffFrequencyChanged_{std::move(onCutoffFrequencyChanged)} {}

  LIVE_OBSERVABLE_PROPERTY(frequencySliderValue, double)

  void onCutoffFrequencyChanged(double newValue) {
    cutoffFrequencyChanged_(newValue);
  }

private:
  CutoffFrequencyChangedUseCase cutoffFrequencyChanged_;
};

class EqFilterComponent : public juce::Component {
public:
  explicit EqFilterComponent(std::unique_ptr<EqFilterViewModel> viewModel)
      : viewModel_{std::move(viewModel)} {
    frequencySlider_.setSliderStyle(juce::Slider::IncDecButtons);
    frequencySlider_.setRange(juce::Range{30., 10'000.}, 0.1);

    frequencySlider_.setValue(viewModel_->frequencySliderValue().value(),
                              juce::dontSendNotification);

    frequencySlider_.onValueChange = [this] {
      viewModel_->onCutoffFrequencyChanged(frequencySlider_.getValue());
    };

    connections_.push_back(
        viewModel_->frequencySliderValue().observe([this](double newValue) {
          frequencySlider_.setValue(newValue, juce::dontSendNotification);
        }));

    addAndMakeVisible(frequencySlider_);
  }

  void resized() override {
    //...
  }

private:
  std::unique_ptr<EqFilterViewModel> viewModel_;
  std::vector<ScopedConnection> connections_;

  juce::Slider frequencySlider_;
};

// Filter Model -> recalculate magnitude response
class EqFilter {
public:
  OBSERVABLE_PROPERTY(magnitudeResponse, dsp::MagnitudeResponse)

  void onCutoffFrequencyChanged(double newCutoffFrequency) {
    cutoffFrequency_ = newCutoffFrequency;
    magnitudeResponse_.setValueForced(calculateMagnitudeResponse());
  }

private:
  [[nodiscard]] dsp::MagnitudeResponse calculateMagnitudeResponse() const {
    dsp::MagnitudeResponse result;
    // magic
    return result;
  }

  double cutoffFrequency_{100.};
};

// MagnitudeResponseViewModel
class MagnitudeResponsePlotViewModel {
public:
  explicit MagnitudeResponsePlotViewModel(
      ObservableProperty<dsp::MagnitudeResponse>& magnitudeResponse)
      : magnitudeResponse_(magnitudeResponse.value()) {
    connections_.push_back(
        magnitudeResponse.observe([this](const auto& newResponse) {
          magnitudeResponse_ = newResponse;
          updatePlot();
        }));
  }

  LIVE_OBSERVABLE_PROPERTY(plot, juce::Path)

  void onPlotBoundsChanged(const juce::Rectangle<int>& newBounds) {
    plotBounds_ = newBounds;
    updatePlot();
  }

private:
  void updatePlot() { plot_.setValueForced(calculateMagnitudeResponsePlot()); }

  [[nodiscard]] juce::Path calculateMagnitudeResponsePlot() const {
    juce::Path result;
    // magic
    return result;
  }

  juce::Rectangle<int> plotBounds_;
  dsp::MagnitudeResponse magnitudeResponse_;
  std::vector<ScopedConnection> connections_;
};

// MagnitudeResponseComponent
class PlotComponent : public juce::Component {
public:
  explicit PlotComponent(
      std::unique_ptr<MagnitudeResponsePlotViewModel> plotViewModel)
      : plotViewModel_{std::move(plotViewModel)} {
    connections_.push_back(plotViewModel_->plot().observe(
        [this](const juce::Path&) { repaint(); }));
  }

  void paint(juce::Graphics& g) override { drawPlot(g); }

  void resized() override {
    plotViewModel_->onPlotBoundsChanged(getLocalBounds());
  }

private:
  void drawPlot(juce::Graphics& g) {
    g.setColour(juce::Colours::white);
    g.setOpacity(1.f);
    g.strokePath(plotViewModel_->plot().value(), juce::PathStrokeType{5.f});
  }

  std::unique_ptr<MagnitudeResponsePlotViewModel> plotViewModel_;
  std::vector<ScopedConnection> connections_;
};

void wiring() {
  EqFilter filter;

  EqFilterComponent eqFilterComponent{
      std::make_unique<EqFilterViewModel>([&](double newCutoffFrequencyHz) {
        filter.onCutoffFrequencyChanged(newCutoffFrequencyHz);
      })};

  PlotComponent plotComponent{std::make_unique<MagnitudeResponsePlotViewModel>(
      filter.magnitudeResponse())};
}

// unit test
TEST(MagnitudeResponsePlotViewModel, RecalculatesPlotOnBoundsChanged) {
  // given
  MutableObservableProperty<dsp::MagnitudeResponse> magnitudeResponse{
      /* frequencies and gains */};
  MagnitudeResponsePlotViewModel testee{magnitudeResponse};

  // when
  testee.onPlotBoundsChanged(juce::Rectangle<int>{0, 0, 100, 100});

  // then
  ASSERT_EQ(juce::Path{/* correct values */}, testee.plot().value());
}
