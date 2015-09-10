using System;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Anki.Cozmo;
using System.Drawing;

namespace AnimationTool
{
    partial class MainForm
    {
        DataPoint curDataPoint; // current selected point
        DataPoint curPreviewBar;

        double GetPixelPositionToValue(Axis axis, double pixel, int round)
        {
            double value = double.MaxValue;

            try
            {
                value = axis.PixelPositionToValue(pixel);
                return Math.Round(value, round);
            }
            catch (Exception) { }

            return value;
        }

        private void ChartXY_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left) return;

            curDataPoint = null;

            //Get the sender as the current chart
            curChart = (Chart)sender;

            ActionManager.Do(new SelectChart(curChart, ModifierKeys != Keys.Shift ? channelList : null));

            ActiveControl = curChart;

            curChartArea.RecalculateAxesScale();

            double maxX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Maximum);
            double minX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Minimum);
            double mouseX = e.X > maxX ? maxX : e.X < minX ? minX : e.X;

            double mouseXValue = GetPixelPositionToValue(curChartArea.AxisX, mouseX, 1);

            if (e.Y < curChart.Size.Height && e.Y > -1)
            {
                for (int i = 1; i < curPoints.Count; ++i) // do not select first point
                {
                    DataPoint dp = curPoints[i];

                    if (Math.Abs(Math.Round(mouseXValue - dp.XValue, 1)) < MoveSelectedDataPoints.DELTA_X)
                    {
                        curDataPoint = dp;
                        break;
                    }
                }
            }
        }

        private void ChartXY_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (curChart != (Chart)sender) // not same chart that we started on
                {
                    return;
                }

                //If we actually clicked on a datapoint
                if (curDataPoint != null && curDataPoint.MarkerSize == SelectDataPoint.MarkerSize)
                {
                    movingDataPointsWithMouse = true;

                    double maxX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Maximum);
                    double minX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Minimum);
                    double minY = 0;
                    double maxY = curChart.Size.Height - 1;
                    double mouseX = e.X > maxX ? maxX : e.X < minX ? minX : e.X; // force valid x
                    double mouseY = e.Y > maxY ? maxY : e.Y < minY ? minY : e.Y; // force valid y

                    double mouseXValue = GetPixelPositionToValue(curChartArea.AxisX, mouseX, 1);
                    double mouseYValue = GetPixelPositionToValue(curChartArea.AxisY, mouseY, 0);

                    int count = 0;

                    bool left = curDataPoint.XValue > mouseXValue;
                    bool right = curDataPoint.XValue < mouseXValue;
                    bool up = curDataPoint.YValues[0] < mouseYValue;
                    bool down = curDataPoint.YValues[0] > mouseYValue;

                    while ((left || right || up || down) && ++count < 100)
                    {
                        if (!ActionManager.Do(new MoveSelectedDataPointsOfSelectedCharts(channelList, left, right, up, down)))
                        {
                            break;
                        }

                        left = curDataPoint.XValue > mouseXValue;
                        right = curDataPoint.XValue < mouseXValue;
                        up = curDataPoint.YValues[0] < mouseYValue;
                        down = curDataPoint.YValues[0] > mouseYValue;
                    }

                    foreach (Chart chart in channelList)
                    {
                        chart.Refresh();
                    }

                    return;
                }
            }

            movingDataPointsWithMouse = false;
        }

        private void Chart_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left || movingDataPointsWithMouse) return; // if we're moving data points, don't change selections

            //If we actually clicked on a datapoint
            if (curDataPoint != null)
            {
                //if shift-click, simply add
                if (ModifierKeys == Keys.Shift)
                {
                    ActionManager.Do(new SelectDataPoint(curDataPoint));
                }
                else
                {
                    //Otherwise we unselect everything and select the current one
                    if (curChart.Name != "cFaceAnimation")
                    {
                        ActionManager.Do(new SelectDataPoint(curDataPoint, curChart));

                        if (curChart.Name == "cProceduralFace")
                        {
                            robotEngineMessenger.AnimateFace(Sequencer.ExtraData.Entries[curDataPoint.GetCustomProperty(Sequencer.ExtraData.Key)] as Sequencer.ExtraProceduralFaceData);
                        }
                    }
                    else
                    {
                        ActionManager.Do(new FaceAnimation.SelectDataPoint(curDataPoint, curChart, faceAnimation.pictureBox));

                        if (curPreviewBar != null)
                        {
                            ActionManager.Do(new FaceAnimation.SelectDataPoint(curPreviewBar, null, faceAnimation.pictureBox));
                        }
                    }
                }
            }
            else if (ModifierKeys != Keys.Shift)
            {
                //If we click in empty space, deselect all points
                ActionManager.Do(new UnselectAllDataPoints(curChart));
            }
        }

        private void ChartXY_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left || movingDataPointsWithMouse || curDataPoint != null) return;

            curChart = (Chart)sender;

            bool addDataPoint = true;

            if(curDataPoint != null)
            {
                addDataPoint = false;
            }

            if (addDataPoint)
            {
                double maxX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Maximum);
                double minX = curChartArea.AxisX.ValueToPixelPosition(curChartArea.AxisX.Minimum);
                double maxY = curChart.Size.Height - 1;
                double minY = 0;

                double mouseXValue = double.MaxValue;
                double mouseYValue = double.MaxValue;

                if (e.Y > maxY || e.Y < minY || e.X > maxX || e.X < minX)
                {
                    addDataPoint = false; // if out of bounds, don't add
                }

                if (addDataPoint)
                {
                    mouseXValue = GetPixelPositionToValue(curChartArea.AxisX, e.X, 1);
                    mouseYValue = GetPixelPositionToValue(curChartArea.AxisY, e.Y, 0);

                    if (mouseXValue < curChartArea.AxisX.Minimum || mouseXValue > curChartArea.AxisX.Maximum ||
                        mouseYValue < curChartArea.AxisY.Minimum || mouseYValue > curChartArea.AxisY.Maximum)
                    {
                        addDataPoint = false; // if out of bounds, don't add
                    }

                    if (addDataPoint)
                    {
                        for (int i = 0; i < curPoints.Count; ++i)
                        {
                            if (Math.Round(curPoints[i].XValue, 1) == Math.Round(mouseXValue, 1))
                            {
                                addDataPoint = false; // if point already there
                            }
                        }
                    }
                }

                if (addDataPoint)
                {
                    ActionManager.Do(new AddDataPoint(curChart, mouseXValue, mouseYValue, true, ModifierKeys != Keys.Shift));
                }
                else
                {
                    ActionManager.Do(new SelectAllDataPoints(curChart)); // if double click in invalid add area, select all
                }
            }
        }

        private void Sequencer_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (curChart != (Chart)sender) // not same chart that we started on
                {
                    return;
                }

                //If we actually clicked on a datapoint
                if (curDataPoint != null && curDataPoint.MarkerColor == SelectDataPoint.MarkerColor)
                {
                    curChartArea.RecalculateAxesScale();

                    movingDataPointsWithMouse = true;

                    double maxX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Maximum);
                    double minX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Minimum);
                    double mouseX = e.X > maxX ? maxX : e.X < minX ? minX : e.X;

                    double mouseXValue = GetPixelPositionToValue(curChartArea.AxisY, mouseX, 1);

                    int count = 0;

                    if (curPreviewBar != null && curPreviewBar.MarkerColor == SelectDataPoint.MarkerColor) // if moving preview bar
                    {
                        bool left = curPreviewBar.YValues[0] > mouseXValue;
                        bool right = curPreviewBar.YValues[1] < mouseXValue;

                        if (ActionManager.Do(new FaceAnimation.MoveSelectedPreviewBar(curPreviewBar, curDataPoint, left, right, faceAnimation.pictureBox)))
                        {
                            cFaceAnimation.Refresh();
                        }
                    }
                    else // else moving data points
                    {
                        bool left = curDataPoint.YValues[0] > mouseXValue;
                        bool right = curDataPoint.YValues[1] < mouseXValue;

                        while ((left || right) && ++count < 100)
                        {
                            ActionManager.Do(new MoveSelectedDataPointsOfSelectedCharts(channelList, left, right, false, false));

                            left = curDataPoint.YValues[0] > mouseXValue;
                            right = curDataPoint.YValues[1] < mouseXValue;
                        }

                        foreach (Chart chart in channelList)
                        {
                            chart.Refresh();
                        }
                    }

                    return;
                }
            }

            movingDataPointsWithMouse = false;
        }

        private void Sequencer_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left) return;

            curDataPoint = null;
            curPreviewBar = null;

            //Get the sender as the current chart
            curChart = (Chart)sender;

            ActionManager.Do(new SelectChart(curChart, ModifierKeys != Keys.Shift ? channelList : null));

            ActiveControl = curChart;

            curChartArea.RecalculateAxesScale();

            double maxX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Maximum);
            double minX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Minimum);
            double mouseX = e.X > maxX ? maxX : e.X < minX ? minX : e.X;

            double mouseXValue = GetPixelPositionToValue(curChartArea.AxisY, mouseX, 2);

            if (e.Y < curChart.Size.Height && e.Y > -1)
            {
                for (int i = 0; i < curPoints.Count; ++i)
                {
                    DataPoint dp = curPoints[i];

                    if (dp.IsEmpty) continue;

                    if (mouseXValue >= dp.YValues[0] && mouseXValue < dp.YValues[1])
                    {
                        if (dp.Color == System.Drawing.Color.Red)
                        {
                            curPreviewBar = dp;
                        }
                        else
                        {
                            curDataPoint = dp;
                        }
                    }
                }
            }
        }

        private void Sequencer_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left || movingDataPointsWithMouse) return;

            curChart = (Chart)sender;
            bool addDataPoint = true;

            if (curDataPoint != null)
            {
                addDataPoint = false;

                switch (curChart.Name)
                {
                    case "cBodyMotion":
                        ShowBodyForm();
                        break;
                    case "cAudioRobot":
                    case "cAudioDevice":
                        ShowVolumeForm();
                        break;
                    case "cProceduralFace":
                        ShowProceduralFaceForm();
                        break;
                }
            }

            if (addDataPoint)
            {
                double maxX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Maximum);
                double minX = curChartArea.AxisY.ValueToPixelPosition(curChartArea.AxisY.Minimum);

                double mouseXValue = 0;

                if (e.X > maxX || e.X < minX)
                {
                    addDataPoint = false; // if out of bounds, don't add
                }

                if (addDataPoint)
                {
                    mouseXValue = GetPixelPositionToValue(curChartArea.AxisY, e.X, 1);

                    if (mouseXValue < curChartArea.AxisY.Minimum || mouseXValue > curChartArea.AxisY.Maximum)
                    {
                        addDataPoint = false; // if out of bounds, don't add
                    }

                    if (addDataPoint)
                    {
                        for (int i = 0; i < curPoints.Count; ++i) // if data point already exists at that time, don't add
                        {
                            DataPoint dp = curPoints[i];

                            if (dp.IsEmpty) continue;

                            if (mouseXValue >= dp.YValues[0] && mouseXValue < dp.YValues[1])
                            {
                                addDataPoint = false;
                                break;
                            }
                        }
                    }
                }

                if (addDataPoint)
                {
                    Sequencer.ExtraData extraData = null;

                    bool body = false;
                    bool faceAnimationData = false;

                    switch (curChart.Name)
                    {
                        case "cProceduralFace":
                            faceAnimationData = true;
                            extraData = new Sequencer.ExtraProceduralFaceData();
                            break;
                        case "cFaceAnimation":
                            extraData = Sequencer.ExtraFaceAnimationData.SelectFaceFolder(selectFolder);
                            break;
                        case "cAudioRobot":
                        case "cAudioDevice":
                            extraData = Sequencer.ExtraAudioData.OpenWavFile(selectFolder);
                            break;
                        case "cBodyMotion":
                            body = true;
                            bodyForm.StartPosition = FormStartPosition.CenterParent;
                            DialogResult result = bodyForm.ShowDialog(curChart);

                            if (result == DialogResult.OK) // straight
                            {
                                extraData = new Sequencer.ExtraStraightData();
                            }
                            else if (result == DialogResult.No) // arc
                            {
                                extraData = new Sequencer.ExtraArcData();
                            }
                            else if (result == DialogResult.Yes) // turn in place
                            {
                                extraData = new Sequencer.ExtraTurnInPlaceData();
                            }
                            break;
                    }

                    if (extraData != null)
                    {
                        Sequencer.AddDataPoint add = null;

                        if (curChart.Name != "cFaceAnimation")
                        {
                            add = new Sequencer.AddDataPoint(curChart, extraData, mouseXValue, true, ModifierKeys != Keys.Shift);
                        }
                        else
                        {
                            add = new Sequencer.AddDataPoint(curChart, extraData, mouseXValue, faceAnimation.pictureBox, ModifierKeys != Keys.Shift);
                        }

                        ActionManager.Do(add);

                        if (body)
                        {
                            curDataPoint = add.dataPoint;
                            ShowBodyForm(true);
                        }
                        else if (faceAnimationData)
                        {
                            curDataPoint = add.dataPoint;
                            DataPoint previous = null;

                            for (int i = 0; i < curPoints.Count - 1; ++i)
                            {
                                if (curPoints[i + 1] == curDataPoint)
                                {
                                    if (!curPoints[i].IsEmpty && curPoints[i].Color != Color.Red)
                                    {
                                        previous = curPoints[i];
                                    }
                                    break;
                                }
                            }

                            ShowProceduralFaceForm(previous);
                        }
                    }
                }
                else
                {
                    ActionManager.Do(new SelectAllDataPoints(curChart)); // if double click in invalid add area, select all
                }
            }
        }

        private void ShowVolumeForm(Sequencer.ExtraAudioData extraData)
        {
            if (volumeForm.Open(extraData.Volume, curChart) == DialogResult.OK)
            {
                extraData.Volume = volumeForm.Volume;
            }
        }

        private void ShowProceduralFaceForm(Sequencer.ExtraProceduralFaceData previous, Sequencer.ExtraProceduralFaceData extraData)
        {
            if (faceForm.Open(previous != null ? previous : extraData) == DialogResult.OK)
            {
                extraData.faceAngle = faceForm.faceAngle;

                for (int i = 0; i < extraData.leftEye.Length; ++i)
                {
                    if (faceForm.eyes[i] != null)
                    {
                        extraData.leftEye[i] = faceForm.eyes[i].LeftValue;
                        extraData.rightEye[i] = faceForm.eyes[i].RightValue;
                    }
                }
            }
            else
            {
                if (previous != null)
                {
                    ActionManager.Do(new RemoveDataPoint(curDataPoint, curChart), true);
                }
                robotEngineMessenger.AnimateFace(previous != null ? previous : extraData); // Else if cancelled, set back to previous face
            }
        }

        private void ShowStraightForm(Sequencer.ExtraStraightData straightData, double maxTime, bool add)
        {
            if (straightForm.Open(maxTime, straightData.Speed_mms, straightData.Length, curChart) == DialogResult.OK)
            {
                straightData.Length = straightForm.Time;
                straightData.Speed_mms = straightForm.Speed;
            }
            else if(add)
            {
                ActionManager.Do(new RemoveDataPoint(curDataPoint, curChart), true);
            }
        }

        private void ShowArcForm(Sequencer.ExtraArcData arcData, double maxTime, bool add)
        {
            if (arcForm.Open(maxTime, arcData.Speed_mms, arcData.Radius_mm, arcData.Length, curChart) == DialogResult.OK)
            {
                arcData.Length = arcForm.Time;
                arcData.Speed_mms = arcForm.Speed;
                arcData.Radius_mm = arcForm.Radius;
            }
            else if (add)
            {
                ActionManager.Do(new RemoveDataPoint(curDataPoint, curChart), true);
            }
        }

        private void ShowTurnInPlaceForm(Sequencer.ExtraTurnInPlaceData turnInPlaceData, double maxTime, bool add)
        {
            if (turnInPlaceForm.Open(maxTime, turnInPlaceData.Angle_deg, turnInPlaceData.Length, curChart) == DialogResult.OK)
            {
                turnInPlaceData.Length = turnInPlaceForm.Time;
                turnInPlaceData.Angle_deg = turnInPlaceForm.Angle;
            }
            else if (add)
            {
                ActionManager.Do(new RemoveDataPoint(curDataPoint, curChart), true);
            }
        }
        
        private void ShowVolumeForm()
        {
            if (curDataPoint == null || curChart == null) return;

            Sequencer.ExtraAudioData extraData = Sequencer.ExtraData.Entries[curDataPoint.GetCustomProperty(Sequencer.ExtraData.Key)] as Sequencer.ExtraAudioData;

            ShowVolumeForm(extraData);

            // Hack: update info 
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] + MoveSelectedDataPoints.DELTA_X));
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] - MoveSelectedDataPoints.DELTA_X));

            curChart.Refresh();
        }

        private void ShowProceduralFaceForm(DataPoint previous = null)
        {
            if (curDataPoint == null || curChart == null) return;

            Sequencer.ExtraProceduralFaceData extraData = Sequencer.ExtraData.Entries[curDataPoint.GetCustomProperty(Sequencer.ExtraData.Key)] as Sequencer.ExtraProceduralFaceData;
            Sequencer.ExtraProceduralFaceData previousData = previous != null ? Sequencer.ExtraData.Entries[previous.GetCustomProperty(Sequencer.ExtraData.Key)] as Sequencer.ExtraProceduralFaceData : null;

            ShowProceduralFaceForm(previousData, extraData);

            // Hack: update info 
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] + MoveSelectedDataPoints.DELTA_X));
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] - MoveSelectedDataPoints.DELTA_X));

            curChart.Refresh();
        }

        private void ShowBodyForm(bool add = false)
        {
            if (curDataPoint == null || curChart == null) return;

            Sequencer.ExtraData extraData = Sequencer.ExtraData.Entries[curDataPoint.GetCustomProperty(Sequencer.ExtraData.Key)];
            double maxTime = Sequencer.ExtraData.GetDistanceFromNextRight(curDataPoint, curPoints);

            Sequencer.ExtraStraightData straightData = extraData as Sequencer.ExtraStraightData;

            if (straightData != null)
            {
                ShowStraightForm(straightData, maxTime, add);
            }
            else
            {
                Sequencer.ExtraArcData arcData = extraData as Sequencer.ExtraArcData;

                if (arcData != null)
                {
                    ShowArcForm(arcData, maxTime, add);
                }
                else
                {
                    Sequencer.ExtraTurnInPlaceData turnInPlaceData = extraData as Sequencer.ExtraTurnInPlaceData;

                    if (turnInPlaceData != null)
                    {
                        ShowTurnInPlaceForm(turnInPlaceData, maxTime, add);
                    }
                }
            }

            // Hack: update info 
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] + MoveSelectedDataPoints.DELTA_X));
            ActionManager.Do(new Sequencer.MoveDataPoint(curDataPoint, curDataPoint.YValues[0] - MoveSelectedDataPoints.DELTA_X));

            curChart.Refresh();
        }
    }
}
