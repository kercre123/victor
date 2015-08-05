using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms.DataVisualization.Charting;
using System.Windows.Forms;

namespace AnimationTool.FaceAnimation
{
    public class MoveSelectedPreviewBar : Action
    {
        protected DataPoint previewBar;
        protected DataPoint dataPoint;
        protected PictureBox pictureBox;

        protected MovePreviewBar movePreviewBar;

        protected bool left;
        protected bool right;

        public MoveSelectedPreviewBar(DataPoint previewBar, DataPoint dataPoint, bool left, bool right, PictureBox pictureBox)
        {
            this.previewBar = previewBar;
            this.dataPoint = dataPoint;
            this.pictureBox = pictureBox;
            this.left = left;
            this.right = right;
        }

        public bool Do()
        {
            if (previewBar == null || dataPoint == null || pictureBox == null)
            {
                return false;
            }

            if(left)
            {
                double destination = Math.Round(previewBar.YValues[0] - MoveSelectedDataPoints.DELTA_X, 2);

                if (destination < Math.Round(dataPoint.YValues[0] - MoveSelectedDataPoints.DELTA_X, 2))
                {
                    return false;
                }

                movePreviewBar = new MovePreviewBar(previewBar, dataPoint, destination, pictureBox);
            }
            else if (right)
            {
                double destination = Math.Round(previewBar.YValues[0] + MoveSelectedDataPoints.DELTA_X, 2);

                if (destination > Math.Round(dataPoint.YValues[1], 2))
                {
                    return false;
                }

                movePreviewBar = new MovePreviewBar(previewBar, dataPoint, destination, pictureBox);
            }

            if(movePreviewBar == null || !movePreviewBar.Do())
            {
                return false;
            }

            return true;
        }

        public void Undo()
        {
            movePreviewBar.Undo();
        }
    }
}
