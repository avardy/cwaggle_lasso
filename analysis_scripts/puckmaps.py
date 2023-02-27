#!/usr/bin/env python

"""
Plots the positions of pucks on top of the obstacles image for specific trials.
"""

import cv2
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
    BASE_NAME = "data_live"
else:
    BASE_NAME = "data"

ARENA_NAMES = [ 'controllerBlindness_2' ]
              # 'normal', 'puckSensingDistance_100', 'controllerBlindness_2' ]
              # 'sim_stadium_no_wall', 'sim_stadium_one_wall', 'sim_stadium_two_walls', 'sim_stadium_three_walls' ]

ARENA_LONG_NAMES = { "normal":"Unlimited Range"
                    , "puckSensingDistance_100":"Range 100"
                    , "controllerBlindness_1":"Blind"
                    , "controllerBlindness_2":"Blind"
                    ,  "sim_stadium_no_wall":"No Wall"
                    , "sim_stadium_one_wall":"One Wall"
                    , "sim_stadium_two_walls":"Two Walls"
                    , "sim_stadium_three_walls":"Three Walls"
                    , "sim_stadium_one_wall_double":"Double-Sized Stadium, One Wall" }

EXPERIMENT_NAMES = [ '' ] 

BEST_TRIAL = False
WORST_TRIAL = True
SPECIFIC_TRIAL = False
SPECIFIC_TRIAL_NUMBER = 0

N_ROBOTS = 8
N_PUCKS = 20
SAVE_FIG = True
START_TRIAL = 0
LAST_TRIAL = 49

def main():
    fig, axes = plt.subplots(1, len(ARENA_NAMES), sharex='col', squeeze=False)

    subplot_column = 0
    for arena_name in ARENA_NAMES:
        puckmap_for_arena(axes[0, subplot_column], arena_name)
        subplot_column += 1

    # Add headers for subplot columns (if more than one used).
    if len(ARENA_NAMES) > 1:
        for col in range(len(ARENA_NAMES)):
            axes[0, col].set_title(ARENA_LONG_NAMES[ARENA_NAMES[col]])

    plt.show()
    if SAVE_FIG:
        filename = "{}.pdf".format(BASE_NAME)
        fig.savefig(filename, bbox_inches='tight')

def puckmap_for_arena(axes, arena_name):

    chosen_puck_df = None
    chosen_robot_df = None
    best_score = float('inf')
    worst_score = 0
    for trial in range(START_TRIAL, LAST_TRIAL + 1):
        
        robot_df = dataframe_per_trial('robotPose', arena_name, trial)
        puck_df = dataframe_per_trial('puckPosition', arena_name, trial)

        if SPECIFIC_TRIAL and trial == SPECIFIC_TRIAL_NUMBER:
            puckmap(axes, puck_df, robot_df, arena_name)

        stats_df = dataframe_per_trial('stats', arena_name, trial)

        # Get the score for the last row
        score = stats_df.at[ len(stats_df)-1, 1 ]
        if BEST_TRIAL and score < best_score:
            best_score = score
            chosen_puck_df = puck_df
            chosen_robot_df = robot_df
            print("Best trial so far: {}".format(trial))

        if WORST_TRIAL and score > worst_score:
            worst_score = score
            chosen_puck_df = puck_df
            chosen_robot_df = robot_df
            print("Worst trial so far: {}".format(trial))

    if BEST_TRIAL or WORST_TRIAL:
        puckmap(axes, chosen_puck_df, chosen_robot_df, arena_name)

def puckmap(axes, puck_df, robot_df, arena_name):
    #filename = 'images/{}/obstacles.png'.format(arena_name)
    filename = 'images/sim_stadium_one_wall/obstacles.png'.format(arena_name)

    obstacles = cv2.imread(filename)
    axes.imshow(obstacles, cmap='gray', interpolation='none')
    axes.axis('off')

    # Draw the pucks
    row = len(puck_df) - 1 # We're always interested in the final row.
    for i in range(N_PUCKS):
        col = 1 + 2*i
        x = puck_df.at[row, col]
        y = puck_df.at[row, col + 1]
        circle = plt.Circle((x, y), 22, color='w')
        inner_circle = plt.Circle((x, y), 12, color='g')
        axes.add_patch(circle)
        axes.add_patch(inner_circle)

    # Draw the robots
    row = len(robot_df) - 1 # We're always interested in the final row.
    for i in range(N_ROBOTS):
        col = 1 + 3*i
        x = robot_df.at[row, col]
        y = robot_df.at[row, col + 1]
        circle = plt.Circle((x, y), 30, color='r')
        axes.add_patch(circle)

    #for x, y in df:
    #df.plot(ax=axes, x='time', y=column_of_interest, label=label, linewidth=0.5)

def dataframe_per_trial(datatype, arena_name, trial):

    filename = BASE_NAME + "/"
    if len(arena_name) > 0:
        filename += arena_name + "/"
    if LIVE:
        filename += "r4/"
    filename += "{}_{}.dat".format(datatype, trial)

    print("Loading: {}".format(filename))
    dataframe = pd.read_csv(filename, " ")

    # The columns are arbitrarily labelled by the first row.  We overwrite to
    # just use integer column names.
    dataframe.columns = [i for i in range(len(dataframe.columns))]

    return dataframe

main()
