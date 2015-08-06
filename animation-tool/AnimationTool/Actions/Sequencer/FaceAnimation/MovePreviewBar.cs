using System.Windows.Forms.DataVisualization.Charting;
using System;
using System.IO;
using AnimationTool.Sequencer;
using System.Windows.Forms;

namespace AnimationTool.FaceAnimation
{
    public class MovePreviewBar : Sequencer.MoveDataPoint
    {
        protected PictureBox pictureBox;
        protected DataPoint previewBar;
        protected ExtraFaceAnimationData extraFaceAnimationData;

        public MovePreviewBar(DataPoint previewBar, DataPoint dataPoint, double yValue, PictureBox pictureBox)
        {
            this.previewBar = previewBar;
            this.dataPoint = dataPoint;
            this.yValue = yValue;
            this.pictureBox = pictureBox;

            previousY = previewBar.YValues[0];

            extraData = ExtraData.Entries[previewBar.GetCustomProperty(ExtraData.Key)];
            extraFaceAnimationData = ExtraData.Entries[dataPoint.GetCustomProperty(ExtraData.Key)] as ExtraFaceAnimationData;
        }

        public MovePreviewBar(DataPoint previewBar, double yValue)
        {
            this.previewBar = previewBar;
            this.yValue = yValue;

            previousY = previewBar.YValues[0];

            extraData = ExtraData.Entries[previewBar.GetCustomProperty(ExtraData.Key)];
        }

        public override bool Do()
        {
            if (previewBar == null || extraData == null) return false;

            previewBar.SetValueY(Math.Round(yValue, 2), Math.Round(yValue + extraData.Length, 2));

            if (dataPoint != null && extraFaceAnimationData != null && pictureBox != null)
            {
                previewBar.ToolTip = extraFaceAnimationData.FileName + " (" + previewBar.YValues[0] + ", " + previewBar.YValues[1] + ")";

                double key = Math.Round(previewBar.YValues[0] - dataPoint.YValues[0], 1);

                if (extraFaceAnimationData.Images.ContainsKey(key))
                {
                    pictureBox.ImageLocation = extraFaceAnimationData.Images[key];
                    pictureBox.SizeMode = PictureBoxSizeMode.StretchImage;
                }

                extraData.FileName = Path.GetFileName(pictureBox.ImageLocation);
                previewBar.ToolTip = extraData.FileName + " (" + previewBar.YValues[0] + ", " + previewBar.YValues[1] + ")";
            }

            return true;
        }

    public override void Undo()
        {
            previewBar.SetValueY(Math.Round(previousY, 2), Math.Round(previousY + extraData.Length, 2));
            previewBar.ToolTip = extraData.FileName + " (" + previewBar.YValues[0] + ", " + previewBar.YValues[1] + ")";

            if (dataPoint != null && extraFaceAnimationData != null && pictureBox != null)
            {
                double key = Math.Round(previewBar.YValues[0] - dataPoint.YValues[0], 1);

                if (extraFaceAnimationData.Images.ContainsKey(key))
                {
                    pictureBox.ImageLocation = extraFaceAnimationData.Images[key];
                    pictureBox.SizeMode = PictureBoxSizeMode.StretchImage;
                }
            }
        }
    }
}
