#!/usr/bin/env python

"""
Unlike 'plots.py', this script uses pandas.  I had to move to pandas because the data files 
from the live robot trials differ in length, even between trials on the same robot.
"""

import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

import pandas as pd
import matplotlib.pyplot as plt

# Modify font size for all matplotlib plots.
plt.rcParams.update({'font.size': 9.1})

#
# Customize the following parameters...
#

LIVE = False
#BASE_NAME = "data_live/{}_robots_{}_pucks_state_0".format(N_ROBOTS, N_PUCKS)
#BASE_NAME = "data/{}_robots_{}_pucks".format(N_ROBOTS, N_PUCKS)
if LIVE:
    BASE_NAME = "../data_live"
else:
    BASE_NAME = "../data"
TITLE = ''

COLUMNS_OF_INTEREST = ['ssd']
#COLUMNS_OF_INTEREST = ['cumPropSlowed']

ARENA_NAMES = [ 
            #'normal', 'puckSensingDistance_100', 'controllerBlindness_2' ]
            #'sim_stadium_no_wall', 'sim_stadium_one_wall', 'sim_stadium_two_walls', 'sim_stadium_three_walls' ]
            #'sim_stadium_one_wall', 'sim_stadium_no_wall', 'sim_stadium_one_wall_double' ]
            #"numPucks_10", "numPucks_20", "numPucks_40", "numPucks_60", "numPucks_80", "numPucks_100" ]
            "numRobots_4", "numRobots_8", "numRobots_12", "numRobots_16"]
            #"12_robots_20_pucks_state_0", "16_robots_20_pucks_state_0" ] 
            #"20_robots_20_pucks_state_0" ] 
            #"sim_stadium_no_wall", "sim_stadium_one_wall" ]

ARENA_LONG_NAMES = { '':''
                    , "normal":"Unlimited Range"
                    , "puckSensingDistance_100":"Range 100"
                    , "controllerBlindness_1":"Blind"
                    , "controllerBlindness_2":"Blind"
                    , "sim_stadium_no_wall":"No Wall"
                    , "sim_stadium_one_wall":"One Wall"
                    , "sim_stadium_two_walls":"Two Walls"
                    , "sim_stadium_three_walls":"Three Walls"
                    , "sim_stadium_one_wall_double":"Double-Sized Stadium, One Wall"
                   , "numPucks_10":"10 Pucks"
                   , "numPucks_20":"20 Pucks"
                   , "numPucks_40":"40 Pucks"
                   , "numPucks_60":"60 Pucks"
                   , "numPucks_80":"80 Pucks"
                   , "numPucks_100":"100 Pucks"
                   , "numRobots_4":"4 Robots"
                   , "numRobots_8":"8 Robots"
                   , "numRobots_12":"12 Robots"
                   , "numRobots_16":"16 Robots"
                   , "numRobots_20":"20 Robots"
                   , "4_robots_20_pucks_state_0":"4 Robots"
                   , "8_robots_20_pucks_state_0":"8 Robots"
                   , "12_robots_20_pucks_state_0":"12 Robots"
                   , "16_robots_20_pucks_state_0":"16 Robots"
                   , "20_robots_20_pucks_state_0":"20 Robots"
                   , "sim_stadium_no_wall":"No Wall"
                   , "sim_stadium_one_wall" :"One Wall"}

EXPERIMENT_NAMES = [ '' ]
                   #  "puckSensingDistance_0"
                   #, "puckSensingDistance_30"
                   #, "puckSensingDistance_60"
                   #, "puckSensingDistance_90"
                   #, "puckSensingDistance_120"
                   #, "puckSensingDistance_1000"
                   #"escapeStrategy_none"
                   #, "escapeStrategy_classic" 
                   #, "escapeStrategy_scatter" 
                   #, "escapeStrategy_centroid" 
                   #, "escapeStrategy_field" 
                   #  "robotSensingDistance_0"
                   #, "robotSensingDistance_30"
                   #, "robotSensingDistance_60"
                   #, "robotSensingDistance_90"
                   #, "robotSensingDistance_120"
                   #, "robotSensingDistance_1000"
                   #] 
EXPERIMENT_LONG_NAMES = { '':''
                        , "normal":"normal"
                        , "puckSensingDistance_100":"Short-Range"
                        , "controllerBlindness_1":"Blind"
                        , "controllerBlindness_2":"Blind"
                        , "blind":"Blind"
                        , "before":"before"
                        , "after":"after"
                        , "puckSensingDistance_0":"0"
                        , "puckSensingDistance_30":"30"
                        , "puckSensingDistance_60":"60"
                        , "puckSensingDistance_90":"90"
                        , "puckSensingDistance_120":"120"
                        , "puckSensingDistance_1000":"1000"
                        , "escapeStrategy_none":"none"
                        , "escapeStrategy_classic":"classic"
                        , "escapeStrategy_scatter":"scatter"
                        , "escapeStrategy_centroid":"centroid"
                        , "escapeStrategy_field":"field"
                        , "robotSensingDistance_0":"0"
                        , "robotSensingDistance_30":"30"
                        , "robotSensingDistance_60":"60"
                        , "robotSensingDistance_90":"90"
                        , "robotSensingDistance_120":"120"
                        , "robotSensingDistance_1000":"1000"
                        , "filterConstant_0.4":"Filter Constant: 0.4" 
                        , "filterConstant_0.5":"Filter Constant: 0.5"
                        , "filterConstant_0.6":"Filter Constant: 0.6"
                        , "filterConstant_0.7":"Filter Constant: 0.7"
                        , "filterConstant_0.8":"Filter Constant: 0.8"
                        , "filterConstant_0.9":"Filter Constant: 0.9"
                        , "filterConstant_1":"Filter Constant: 1.0"
                        , "filterConstant_1.2":"Filter Constant: 1.2"
                        , "filterConstant_1.5":"Filter Constant: 1.5"
                        , "filterConstant_2":"Filter Constant: 2"
                        , "filterConstant_3":"Filter Constant: 3"
                        , "filterConstant_4":"Filter Constant: 4"
                        , "filterConstant_5":"Filter Constant: 5"
                        , "filterConstant_7.5":"Filter Constant: 7.5"
                        , "filterConstant_10":"Filter Constant: 10"
                        } 

#EXPERIMENT_NAMES = [ "4_robots_10_pucks_state_0" , "4_robots_20_pucks_state_0" ]
#EXPERIMENT_LONG_NAMES = { "4_robots_10_pucks_state_0":"10 Pucks"
#                        , "4_robots_20_pucks_state_0":"20 Pucks" }

COLUMN_NAMES = ['time', 'ssd', 'propSlowed', 'cumPropSlowed', 'avgTau', 'avgMedianTau', 'avgFilteredTau', 'avgState']
COLUMN_LONG_NAMES = { "time":"Time"
                    , "ssd":"ASD"
                    , "propSlowed":"Prop. Slowed"
                    , "cumPropSlowed":"Cum. Prop. Slowed"
                    , "avgTau":"Tau"
                    , "avgMedianTau":"Median Tau"
                    , "avgFilteredTau":"Filtered Tau"
                    , "avgState":"State"
                    }

if LIVE:
    X_LABEL = "Time (secs)"
else:
    X_LABEL = "Time (steps)"
SAVE_FIG = True
START_TRIAL = 0
LAST_TRIAL = 49
PLOT_TRIALS = True
PLOT_MEAN = True
COLLAPSE_EXPERIMENTS = False

def main():
    if COLLAPSE_EXPERIMENTS:
        fig, axes = plt.subplots(len(COLUMNS_OF_INTEREST), len(ARENA_NAMES), sharex='col', sharey='row', squeeze=False)
        #fig, axes = plt.subplots(len(COLUMNS_OF_INTEREST), len(ARENA_NAMES), sharex='col', squeeze=False)
    else:
        fig, axes = plt.subplots(len(EXPERIMENT_NAMES * len(COLUMNS_OF_INTEREST)), len(ARENA_NAMES), sharex='col', sharey='row', squeeze=False)

    subplot_column = 0
    for arena_name in ARENA_NAMES:
        plots_for_arena(axes, arena_name, subplot_column)
        subplot_column += 1

    # Add headers for subplot columns (if more than one used).
    if len(ARENA_NAMES) > 1:
        for col in range(len(ARENA_NAMES)):
            axes[0, col].set_title(ARENA_LONG_NAMES[ARENA_NAMES[col]])

    if len(TITLE) > 0:
        fig.suptitle(TITLE)

    # Make more space between subplot rows
    #plt.subplots_adjust(hspace=0.45)

    plt.show()
    if SAVE_FIG:
        filename = "plots.pdf"
        fig.savefig(filename, bbox_inches='tight')

def plots_for_arena(axes, arena_name, subplot_column):
    axes_index = 0

    if COLLAPSE_EXPERIMENTS:
        for j in range(len(COLUMNS_OF_INTEREST)):
            for i in range(len(EXPERIMENT_NAMES)):
                plot(axes[axes_index, subplot_column], arena_name, EXPERIMENT_NAMES[i], COLUMNS_OF_INTEREST[j])
            axes_index += 1
    else:
        for i in range(len(EXPERIMENT_NAMES)):
            for j in range(len(COLUMNS_OF_INTEREST)):
                plot(axes[axes_index, subplot_column], arena_name, EXPERIMENT_NAMES[i], COLUMNS_OF_INTEREST[j])
                axes_index += 1

    axes[-1, subplot_column].set_xlabel(X_LABEL)

    
def plot(axes, arena_name, experiment, column_of_interest):
    axes.axhline(y=0, color='k', linewidth=0.5)

    if column_of_interest == 'cumPropSlowed':
        axes.set_aspect(1.0)
    elif LIVE:
        axes.set_aspect(4000.0)
    else:
        axes.set_aspect(250000.0)

    all_dfs = []
    for trial in range(START_TRIAL, LAST_TRIAL + 1):
        df = dataframe_per_trial(axes, arena_name, experiment, column_of_interest, trial)
        all_dfs.append(df)

    df_concat = pd.concat(all_dfs)
    by_row_index = df_concat.groupby(df_concat.index)
    if PLOT_MEAN:
        by_row_index.mean().plot(ax=axes, x='time', y=column_of_interest, label=EXPERIMENT_LONG_NAMES[experiment], linewidth=1, color='black')

    axes.set_ylabel(COLUMN_LONG_NAMES[column_of_interest])

    #handles, labels = axes.get_legend_handles_labels()
    #axes.legend(handles, labels)

    #if not COLLAPSE_EXPERIMENTS:
    axes.get_legend().remove()

    # Show the legend in lower right and reverse its order
    #axes.legend(handles[::-1], labels[::-1], loc=1)

def dataframe_per_trial(axes, arena_name, experiment, column_of_interest, trial):

    #filename = "{}/{}/{}/{}/stats_{}.dat".format(BASE_NAME, arena_name, experiment, ROBOT, trial)
    filename = BASE_NAME + "/"
    if len(arena_name) > 0:
        filename += arena_name + "/"
    if len(experiment) > 0:
        filename += experiment + "/"
    if LIVE:
        filename += "r4/"
    filename += "stats_{}.dat".format(trial)

    print("Loading: {}".format(filename))
    dataframe = pd.read_csv(filename, " ")

    dataframe.columns = COLUMN_NAMES

    if PLOT_TRIALS:
        #label = "Exp: {}, Trial: {}".format(experiment, trial)
        label = "{}".format(trial)
        dataframe.plot(ax=axes, x='time', y=column_of_interest, label=label, linewidth=0.5)
        axes.title.set_text(EXPERIMENT_LONG_NAMES[experiment])

    #print(dataframe.info())

    return dataframe

main()
