using System;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;
using System.Windows.Forms;

namespace AnimationTool.FaceAnimation
{
    public class SelectDataPoint : AnimationTool.SelectDataPoint
    {
        protected MovePreviewBar movePreviewBar;
        protected bool previousPreviewBarState;

        protected DataPoint previewBar;
        protected PictureBox pictureBox;
        protected Chart chart;
        protected double offset;

        public SelectDataPoint(DataPoint dataPoint, Chart chart, PictureBox pictureBox)
        {
            this.dataPoint = dataPoint;
            this.chart = chart;
            this.pictureBox = pictureBox;
            offset = Math.Round(AddPreviewBar.Length * 0.5, 2);
        }

        public override bool Do()
        {
            if (chart != null)
            {
                unselectAllDataPoints = new UnselectAllDataPoints(chart);

                foreach (DataPoint dp in chart.Series[0].Points)
                {
                    if (dp.Color == Color.Red)
                    {
                        previewBar = dp;
                        previousPreviewBarState = previewBar.IsEmpty;
                        if (previewBar != dataPoint && (previewBar.YValues[0] < dataPoint.YValues[0] + offset || previewBar.YValues[0] > dataPoint.YValues[1] - offset))
                        {
                            // if preview bar not selected, move to start of datapoint
                            movePreviewBar = new MovePreviewBar(previewBar, dataPoint, dataPoint.YValues[0] - offset, pictureBox);
                        }
                        break;
                    }
                }
            }
            else
            {
                previewBar = dataPoint;
            }

            if (previewBar == null) return false;

            if (!base.Do())
            {
                return false;
            }

            if (unselectAllDataPoints != null)
            {
                previewBar.IsEmpty = false;
            }

            return movePreviewBar == null || movePreviewBar.Do();
        }

        public override void Undo()
        {
            base.Undo();

            previewBar.IsEmpty = previousPreviewBarState;

            if (movePreviewBar != null)
            {
                movePreviewBar.Undo();
            }
        }
    }
}
