#
# Script to plot a buffer of data from memory
#
#
# Plots:
#   Plot of buffer data. Option to change y axis log or linear.
#   This version only compatible with codescape 7.2
#

from codescape import da
import wx
from collections import deque
from cmath import phase
from math import log10
from CSUtils import DA

def setup_graph_control(graph, xMin, xMax, yRange, logYAxis):
    # graph setup
    graph.set_background_colour((218,218,218))
    font = wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL, faceName="Arial", underline=True)
    
    # x_axis setup
    x_interval = xMax/8
    graph.x_axis.set_range(xMin, xMax)
    graph.x_axis.set_draw_line()
    graph.x_axis.set_draw_grid(x_interval)
    graph.x_axis.set_draw_tick(x_interval)
    graph.x_axis.set_draw_tick_text(x_interval, "%d")
    graph.x_axis.set_title("Index")

    # y_axis setup
    y_interval = yRange / 8;
    graph.y_axis.set_range(yRange, 0)
    graph.y_axis.set_draw_line()
    graph.y_axis.set_draw_grid(y_interval)
    graph.y_axis.set_draw_tick(y_interval/4)
    graph.y_axis.set_draw_tick_text(y_interval, "%-0.2f")
    if (logYAxis):
        yAxisLabel = "Magnitude (dB)"
    else:
        yAxisLabel = "Magnitude (dB)"
    graph.y_axis.set_title(yAxisLabel, distance_from_axis = 30)


class MyRegion(wx.Frame):
    def __init__(self):
    
        import sys
    
        wx.Frame.__init__(self, None)
        self.SetBackgroundColour((128,128,128)) # dark grey

        # Setup the buffer
        self.maxCoeffs      = int(sys.argv[3]);
        self.buf1           = [0] * self.maxCoeffs;
        self.buf2           = [0] * self.maxCoeffs;
        self.buf1Name       = sys.argv[1];
        self.buf2Name       = sys.argv[2];
        
        # Get the Buffer location, which are fixed
        self.buf1Locn = self.getBufLocn(self.buf1Name)
        print self.buf1Locn
        
        self.buf2Locn = self.getBufLocn(self.buf2Name)
        
        # Define how often to update the graph
        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.OnTimer, self.timer)
        self.timer.Start(5000) # configure timer (ms)

        # Create log/lin checkbox
        self.logPlotBox = wx.CheckBox(self, pos=(10, 20), label="Logarithmic plot")
        self.logPlotBox.Bind(wx.EVT_CHECKBOX, self.OnlogPlotBoxClicked)
        self.logYAxis = 0;

        
        # create and setup a GraphControl
        self.yLimitLinear = 1<<31;
        self.graph = wx.GraphControl(self, pos=(10,35), size=(800,300))
        self.line1_style = wx.GraphLineStyle(line_colour = (255,0,0), line_width = 2) # red line
        self.line2_style = wx.GraphLineStyle(line_colour = (0,255,0), line_width = 2) # green line
        setup_graph_control(self.graph, 0, self.maxCoeffs-1, self.yLimitLinear, self.logYAxis)


        self.SetFont(wx.Font(faceName = 'Arial', pointSize = 10, family = wx.FONTFAMILY_DEFAULT, style = wx.FONTSTYLE_NORMAL, weight = wx.FONTWEIGHT_NORMAL))

        self.xMin = 0;
        self.xMinStatic = wx.StaticText(self, pos=(17, 353), label="xMin")
        self.xMinTextCtrl = wx.TextCtrl(self, pos=(60, 350), value=str(self.xMin))
        self.xMinSlider = wx.Slider(self, pos=(170, 350), value=self.xMin, minValue=0, maxValue=self.maxCoeffs)
        self.xMinTextCtrl.Bind(wx.EVT_TEXT, self.OnXMinChanged)
        self.xMinSlider.Bind(wx.EVT_SCROLL, self.OnXMinSliderScroll)
        self.xMinSlider.SetValue(self.xMin);

        self.xMax = self.maxCoeffs;
        self.xMaxStatic = wx.StaticText(self, pos=(407, 353), label="xMax")
        self.xMaxTextCtrl = wx.TextCtrl(self, pos=(450, 350), value=str(self.xMax))
        self.xMaxSlider = wx.Slider(self, pos=(560, 350), value=self.xMax, minValue=0, maxValue=self.maxCoeffs)
        self.xMaxTextCtrl.Bind(wx.EVT_TEXT, self.OnXMaxChanged)
        self.xMaxSlider.Bind(wx.EVT_SCROLL, self.OnXMaxSliderScroll)
        
        self.ReloadData()
        self.RedrawGraph()

    def OnlogPlotBoxClicked(self, evt):
        self.logYAxis = evt.IsChecked()   
        self.RedrawGraph()

    def OnXMinChanged(self, evt):
        self.xMin = int(evt.String)   
        if (self.xMin < 1):
            self.xMin = 1;
        if (self.xMin > self.maxCoeffs):
            self.xMin = self.maxCoeffs;
        self.RedrawGraph()

    def OnXMinSliderScroll(self, evt):
        evt.EventObject.SetValue(evt.Position)
        self.xMin = evt.Position
        self.xMinTextCtrl.SetValue(str(self.xMin))

    def OnXMaxChanged(self, evt):
        self.xMax = int(evt.String)   
        if (self.xMax < 1):
            self.xMax = 1;
        if (self.xMax > self.maxCoeffs):
            self.xMax = self.maxCoeffs;
        self.RedrawGraph()

    def OnXMaxSliderScroll(self, evt):
        evt.EventObject.SetValue(evt.Position)
        self.xMax = evt.Position
        self.xMaxTextCtrl.SetValue(str(self.xMax))

    # Run this every time tick
    def OnTimer(self, evt):
        self.ReloadData()
        self.RedrawGraph()
    
    def ReloadData(self):        
        # Load buffers (element_type=4, unsigned 32 bit, memory_type=1, minimData )
        if (self.buf1Locn != 0):
            self.buf1        = DA.ReadMemoryBlock(self.buf1Locn, self.maxCoeffs, -2)
            self.shiftData(self.buf1)

        if (self.buf2Locn != 0):
            self.buf2        = DA.ReadMemoryBlock(self.buf2Locn, self.maxCoeffs, -2)

    def RedrawGraph(self):
        # Find peak and rescale
        peak1 = self.findPeak(self.buf1)
        peak2 = self.findPeak(self.buf2)
        try:
            scale = float(peak1)/float(peak2)
        except:
            scale = 1
        
        for x in range(0, len(self.buf2)):
            self.buf2[x] = self.buf2[x] * scale
        peak = peak1+1
                   

        # Pick log if required        
        if self.logYAxis:
            line1 = self.getLogLine(self.buf1)
            peak = 20*log10(peak);
            line2 = self.getLogLine(self.buf2)
        else:
            line1 = self.getLinearLine(self.buf1)
            line2 = self.getLinearLine(self.buf2)

        # rescale y-axis to peak 
        setup_graph_control(self.graph, self.xMin, self.xMax, peak, self.logYAxis)

        # Draw the graph lines
        self.graph.clear_data()
        self.graph.add_line(line1, self.line1_style)
        self.graph.add_line(line2, self.line2_style)
        self.graph.redraw()

    def getLinearLine(self, data):
        return [(x, data[x]) for x in range(0, len(data))]
        
    def getLogLine(self, data):
        # Add in 0dB as a noise floor to stop any errors with log10(0)
        return [(x, 20*log10(abs(data[x])+1)) for x in range(0, len(data))]

    def getBufLocn(self, name):
        try:
            bufLocn = DA.EvaluateSymbol(name)
            #bufLocn = bufLocn + (16384/2) - self.maxCoeffs/2
        except:
            bufLocn = 0
        return(bufLocn)
     
    def findPeak(self, data):
        peak = 0
        for x in range(0, len(data)):
            peak = max(peak, data[x]);
        return(peak)
    
    def shiftData(self, data):
        dataMin = 0
        for x in range(0, len(data)):
            dataMin = min(dataMin, data[x]);
        if dataMin < 0:
            shift = -1*dataMin
        else:
            shift = 0
        for x in range(0, len(data)):
            data[x] = data[x]+shift


if __name__ == "__main__":
    app = wx.PySimpleApp()
    region = MyRegion()
    region.Show()
    app.MainLoop()
