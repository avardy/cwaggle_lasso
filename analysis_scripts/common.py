#!/usr/bin/env python
# Provides the 'barPlot' function and 'readCSV'.
import csv
import numpy as np
import matplotlib.pyplot as plt

def test():

    # Example data for a 2-class barplot
    menMeans = (20, 35, 30, 35, 27)
    menStd =   (2, 3, 4, 1, 2)
    womenMeans = (25, 32, 34, 20, 25)
    womenStd =   (3, 5, 2, 3, 3)
    meanData = (menMeans, womenMeans)
    stdData = (menStd, womenStd)
    labels = ('men', 'women')
    colors = ('g', 'y')
    title = 'Scores by group and gender/class'
    xLabels = ('1', '2', '3', '4', '5')
    xLabel = 'Group #'
    yLabel = 'Scores'
    hatches = ('/', '.')
    barPlot(meanData, stdData, labels, colors, title, xLabels, xLabel, yLabel, hatches)

    # Extending example for a 3-class barplot
    childMeans = (25, 40, 35, 40, 32)
    childStd =   (2, 3, 4, 1, 2)
    meanData = (menMeans, womenMeans, childMeans)
    stdData = (menStd, womenStd, childStd)
    labels = ('men', 'women', 'children')
    colors = ('g', 'y', 'b')
    hatches = ('/', '.', '*')
    barPlot(meanData, stdData, labels, colors, title, xLabels, xLabel, yLabel, hatches)

def barPlot(meanData, stdData, labels, colors, title, xLabels, xLabel, yLabel, hatches):
    ylower = float("inf")
    yupper = 0
    for meanList in meanData:
        ylower = min(ylower, min(meanList))
        yupper = max(yupper, max(meanList))
    barPlotWithLimits(meanData, stdData, labels, colors, title, xLabels, xLabel, yLabel, (ylower, yupper), 0, hatches)

def barPlotWithLimits(meanData, stdData, labels, colors, title, xLabels, xLabel, yLabel, yLimits, legendLocation, hatches):
    nDatasets = len(meanData)
    nXpoints = len(meanData[0])

    # X-locations
    xLocs = np.arange(nXpoints)

    # Spacing between groups of bars
    spacing = 0.4

    # Width of the bars
    width = (1 - spacing) / nDatasets

    fig = plt.figure()
    ax = fig.add_subplot(111)

    rectsList = []
    if nDatasets == 1:
        rectsList.append( ax.bar(xLocs, meanData[0], width, color=colors[0], yerr=stdData[0], ecolor='k', hatch=hatches[0]) )
    else:
        for i in range(nDatasets):
            rectsList.append( ax.bar(xLocs+i*width, meanData[i], width, color=colors[i], yerr=stdData[i], label=labels[i], ecolor='k', hatch=hatches[i]) )

    ax.set_title(title)
    ax.set_ylabel(yLabel)
    ax.set_xlabel(xLabel)
    ax.set_xticks(xLocs + (1 - spacing)/2)
    ax.set_xticklabels(xLabels)
    ax.legend(loc=legendLocation)

    plt.ylim(yLimits)

    plt.show()

def readOneColumnCSV(filename):
    f = open(filename, 'rt')
    Y = []
    try:
        reader = csv.reader(f, delimiter=' ')
        for row in reader:
            Y.append(float(row[0]))
        return [Y]
    finally:
        f.close()

def readCSV(filename, xColumn, yColumn, delim=' '):
    f = open(filename, 'rt')
    X = []
    Y = []
    try:
        reader = csv.reader(f, delimiter=delim)
        for row in reader:
            X.append(float(row[xColumn]))
            Y.append(float(row[yColumn]))
        return [X, Y]
    finally:
        f.close()

if __name__ == "__main__":
    test()
