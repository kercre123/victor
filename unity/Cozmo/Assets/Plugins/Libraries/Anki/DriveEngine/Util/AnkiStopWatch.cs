using System;
using System.Diagnostics;

public class AnkiStopWatch {

  // internals
  private uint count_;
  private double accumulator_;
  private double average_;
  private uint aboveAverageCount_;
  private double aboveAverageAccumulator_;
  private double aboveAverageAverage_;
  private double max_;
  private double averageLimit_;
  private Stopwatch stopWatch_;
  private RunningStat tickLength_;

  public AnkiStopWatch() {
    stopWatch_ = new Stopwatch();
    tickLength_ = new RunningStat();
  }

  public void Start() {
    stopWatch_.Start();
    averageLimit_ = 0;
  }

  public double Stop() {
    stopWatch_.Stop();
    long diffMs = stopWatch_.ElapsedMilliseconds;
    stopWatch_.Reset();
    
    count_++;
    accumulator_ += diffMs;
    tickLength_.Push((float)diffMs);
    
    // make sure we have enough data for averaging
    if (count_ > 60) {
      average_ = accumulator_ / (double)count_;
      if ((averageLimit_ > 0.5 && diffMs > averageLimit_) ||
        (averageLimit_ < 0.5 && diffMs > average_ * 2.0)) {
        aboveAverageAccumulator_ += diffMs;
        aboveAverageCount_++;
        aboveAverageAverage_ = aboveAverageAccumulator_ / (double)aboveAverageCount_;
        if (diffMs > max_)
          max_ = diffMs;
      }
    }
    
    return diffMs;
  }
  
  public void LogStats(string eventName) {
    DAS.Debug(eventName, "f_average " + average_);
    DAS.Debug(eventName, "i_tickCount " + count_);
    DAS.Debug(eventName, "f_aboveAveragePercent " + ((double)aboveAverageCount_ / (double)count_));
    DAS.Debug(eventName, "f_aboveAverageAverage " + aboveAverageAverage_);
    DAS.Debug(eventName, "i_aboveAverageCount " + aboveAverageCount_);
    DAS.Debug(eventName, "f_aboveAverageMax " + max_);
    tickLength_.LogStats(eventName);
  }

}

