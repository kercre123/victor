using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class ChangeAllChartDurations : Action
    {
        protected List<ChangeChartDuration> changeChartDurations;

        protected double xValueOld;
        protected double xValueNew;

        public ChangeAllChartDurations(List<ChartForm> chartForms, double xValueNew)
        {
            changeChartDurations = new List<ChangeChartDuration>();

            this.xValueNew = xValueNew;
            xValueOld = Properties.Settings.Default.maxTime;

            foreach (ChartForm c in chartForms)
            {
                DataPoint dp = c.chart.Series[0].Points[0]; // if chart has sequencer point
                bool sequencer = dp.IsCustomPropertySet(Sequencer.ExtraData.Key) && Sequencer.ExtraData.Entries.ContainsKey(dp.GetCustomProperty(Sequencer.ExtraData.Key));

                if (sequencer)
                {
                    changeChartDurations.Add(new Sequencer.ChangeChartDuration(c.chart, xValueNew));
                }
                else // else if XYchart
                {
                    changeChartDurations.Add(new ChangeChartDuration(c.chart, xValueNew));
                }
            }
        }

        public virtual bool Do()
        {
            if (changeChartDurations == null) return false;

            for (int i = 0; i < changeChartDurations.Count; ++i)
            {
                if (!changeChartDurations[i].Do())
                {
                    changeChartDurations.RemoveAt(i--);
                }
            }

            if (changeChartDurations.Count == 0)
            {
                return false;
            }

            Properties.Settings.Default["maxTime"] = xValueNew;
            Properties.Settings.Default.Save();

            return true;
        }

        public virtual void Undo()
        {
            Properties.Settings.Default["maxTime"] = xValueOld;
            Properties.Settings.Default.Save();

            foreach (ScaleChartDuration action in changeChartDurations)
            {
                action.Undo();
            }
        }
    }
}
