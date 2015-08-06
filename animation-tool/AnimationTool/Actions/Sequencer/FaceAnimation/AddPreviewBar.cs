using System.Windows.Forms.DataVisualization.Charting;
using System.Windows.Forms;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using AnimationTool.Sequencer;

namespace AnimationTool.FaceAnimation
{
    public class AddPreviewBar : Sequencer.AddDataPoint
    {
        public const double Length = 0.04;

        public AddPreviewBar(Chart chart, ExtraData extraData)
        {
            if (chart == null) return;

            this.chart = chart;
            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];

            dataPoint = new DataPoint();
            this.extraData = extraData;

            dataPoint.XValue = 0.5D;
            dataPoint.SetValueY(0, Math.Round(extraData.Length, 2));
            dataPoint.IsEmpty = true;
            dataPoint.Color = Color.Red;
        }
    }
}
