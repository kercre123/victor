using System;
public class RunningStat {
  int m_n;
  float m_oldM, m_newM, m_oldS, m_newS;

  public RunningStat() {
    m_n = 0;
    m_oldM = 0.0f;
    m_newM = 0.0f;
    m_oldS = 0.0f;
    m_newS = 0.0f;
  }

  // clear data
  public void Clear() {
    m_n = 0;
  }
    
  // add a new value
  public void Push(float x) {
    m_n++;
        
    // See Knuth TAOCP vol 2, 3rd edition, page 232
    if (m_n == 1) {
      m_oldM = m_newM = x;
      m_oldS = 0.0f;
    }
    else {
      m_newM = m_oldM + (x - m_oldM) / m_n;
      m_newS = m_oldS + (x - m_oldM) * (x - m_newM);
            
      // set up for next iteration
      m_oldM = m_newM;
      m_oldS = m_newS;
    }
  }

  // returns the number of values in the accumulator
  public int NumDataValues() {
    return m_n;
  }
    
  // returns the mean of the sample
  public float Mean() {
    return (m_n > 0) ? m_newM : 0.0f;
  }
    
  // returns the variance of the sample
  public float Variance() {
    return ((m_n > 1) ? m_newS / (m_n - 1) : 0.0f);
  }
    
  // returns the standard deviation of the sample
  public float StandardDeviation() {
    return (float)System.Math.Sqrt(Variance());
  }
    
  // Logs collected data
  public void LogStats(String eventName) {
    float fData = NumDataValues();
    DAS.Debug(eventName, "f_numDataValues " + fData);
    fData = Mean();
    DAS.Debug(eventName, "i_mean " + fData);
    fData = Variance();
    DAS.Debug(eventName, "f_variance " + fData);
    fData = StandardDeviation();
    DAS.Debug(eventName, "f_standardDeviation " + fData);
  }

}

