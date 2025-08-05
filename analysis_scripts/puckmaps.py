#!/usr/bin/env python

"""
Plots the positions of pucks on top of the obstacles image for specific trials.
"""

import cv2
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from math import asin, cos, fabs, pi, sin

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

ARENA_NAMES = ["numRobots_16"]#, "numRobots_8", "numRobots_12", "numRobots_16"]
              # 'controllerBlindness_2' ]
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
                    , "sim_stadium_one_wall_double":"Double-Sized Stadium, One Wall" 
                   , "numRobots_4":"4 Robots"
                   , "numRobots_8":"8 Robots"
                   , "numRobots_12":"12 Robots"
                   , "numRobots_16":"16 Robots"
                   }

EXPERIMENT_NAMES = [ '' ] 

BEST_TRIAL = False
WORST_TRIAL = False
SPECIFIC_PERFORMANCE_TRIAL = True
SPECIFIC_PERFORMANCE = 0.1
SPECIFIC_TRIAL = False
SPECIFIC_TRIAL_NUMBER = 0

DRAW_GOAL = False
DRAW_PUCKS = True
DRAW_FINAL_ROBOTS = False
DRAW_HEATMAP_ROBOTS = True

N_ROBOTS = 16
N_PUCKS = 20
SAVE_FIG = True
START_TRIAL = 0
LAST_TRIAL = 49

ROBOT_RADIUS = 30
PUCK_RADIUS = 22
PLOW_LENGTH = 60
PLOW_ANGLE = -35 * pi / 180.0
GOAL_X = 310
GOAL_Y = 245

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
        filename = f"../performance_{SPECIFIC_PERFORMANCE}.pdf"
        #filename = f"../dummy.pdf"
        print(f"Saving {filename}")
        fig.savefig(filename, bbox_inches='tight')

def puckmap_for_arena(axes, arena_name):

    chosen_puck_df = None
    chosen_robot_df = None
    chosen_score = None
    chosen_trial_number = None
    best_score = float('inf')
    worst_score = 0
    lowest_score_difference = float('inf')
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
            chosen_score = score
            chosen_trial_number = trial
            print("Best trial so far: {}".format(trial))

        if WORST_TRIAL and score > worst_score:
            worst_score = score
            chosen_puck_df = puck_df
            chosen_robot_df = robot_df
            chosen_score = score
            chosen_trial_number = trial
            print("Worst trial so far: {}".format(trial))

        if SPECIFIC_PERFORMANCE_TRIAL and fabs(score - SPECIFIC_PERFORMANCE) < lowest_score_difference:
            lowest_score_difference = fabs(score - SPECIFIC_PERFORMANCE)
            chosen_puck_df = puck_df
            chosen_robot_df = robot_df
            chosen_score = score
            chosen_trial_number = trial
            print("Closest trial so far: {}".format(trial))

    if BEST_TRIAL or WORST_TRIAL or SPECIFIC_PERFORMANCE_TRIAL:
        print(f"Score of chosen trial: {chosen_score}")
        print(f"Index of chosen trial: {chosen_trial_number}")
        puckmap(axes, chosen_puck_df, chosen_robot_df, arena_name)

def draw_robot(x, y, theta, axes, transparency):
    contact_angle = pi/2 - asin(ROBOT_RADIUS / PLOW_LENGTH)

    # First create the 3 points of the plow.
    plow_points = [ 
               [x + ROBOT_RADIUS * cos(theta + PLOW_ANGLE - contact_angle),
                y + ROBOT_RADIUS * sin(theta + PLOW_ANGLE - contact_angle)], 
               [x + PLOW_LENGTH * cos(theta + PLOW_ANGLE),
                y + PLOW_LENGTH * sin(theta + PLOW_ANGLE)], 
               [x + ROBOT_RADIUS * cos(theta + PLOW_ANGLE + contact_angle),
                y + ROBOT_RADIUS * sin(theta + PLOW_ANGLE + contact_angle)]
    ]

    # Now fill in the circular section 
    delta_angle = 0.1
    start_angle = theta + PLOW_ANGLE + contact_angle
    stop_angle = start_angle + pi + 2*asin(ROBOT_RADIUS / PLOW_LENGTH) - delta_angle
    angle = start_angle + delta_angle
    circle_points = []
    while angle < stop_angle:
        circle_points.append([x + ROBOT_RADIUS * cos(angle), y + ROBOT_RADIUS * sin(angle)])
        angle += delta_angle

    point_array = np.array(plow_points + circle_points)
    filled_polygon = plt.Polygon(point_array, color=(1, 0, 0, transparency))
    axes.add_patch(filled_polygon)

    outline_polygon = plt.Polygon(point_array, color=(1, 1, 1, transparency), fill=False)
    axes.add_patch(outline_polygon)

    # Draw heading line
    radius = 0.9 * ROBOT_RADIUS
    line = plt.Line2D([x, x + radius * cos(theta)], [y, y + radius * sin(theta)], color=(1, 1, 1, transparency))
    axes.add_line(line)

def puckmap(axes, puck_df, robot_df, arena_name):
    #filename = 'images/{}/obstacles.png'.format(arena_name)
    filename = '../images/sim_stadium_one_wall/obstacles.png'.format(arena_name)

    obstacles = cv2.imread(filename)
    axes.imshow(obstacles, cmap='gray', interpolation='none')
    #axes.axis('off')
    axes.set_xticks([])
    axes.set_yticks([])

    if DRAW_GOAL:
        d = 20
        line1 = plt.Line2D([GOAL_X - d, GOAL_X + d], [GOAL_Y - d, GOAL_Y + d], color=(1, 1, 1))
        line2 = plt.Line2D([GOAL_X - d, GOAL_X + d], [GOAL_Y + d, GOAL_Y - d], color=(1, 1, 1))
        axes.add_line(line1)
        axes.add_line(line2)

    if DRAW_PUCKS:
        row = len(puck_df) - 1 # We're always interested in the final row.
        for i in range(N_PUCKS):
            col = 1 + 2*i
            x = puck_df.at[row, col]
            y = puck_df.at[row, col + 1]
            inner_circle = plt.Circle((x, y), PUCK_RADIUS, color='g', fill=True)
            circle = plt.Circle((x, y), PUCK_RADIUS, color='w', fill=False)
            axes.add_patch(inner_circle)
            axes.add_patch(circle)

    if DRAW_FINAL_ROBOTS:
        # Draw the final position of the robots
        row = len(robot_df) - 1 # We're always interested in the final row.
        for i in range(N_ROBOTS):
            col = 1 + 3*i
            x = robot_df.at[row, col]
            y = robot_df.at[row, col + 1]
            theta = robot_df.at[row, col + 2]
            draw_robot(x, y, theta, axes, 'r')

    n_rows = len(robot_df)
    if DRAW_HEATMAP_ROBOTS:
        final_proportion_to_draw = 0.01
        start_row = int(n_rows * (1 - final_proportion_to_draw))
        for row in range(start_row, n_rows):
            for i in range(N_ROBOTS):
                col = 1 + 3*i
                x = robot_df.at[row, col]
                y = robot_df.at[row, col + 1]
                theta = robot_df.at[row, col + 2]
                transparency = ((row - start_row) / (n_rows * final_proportion_to_draw)) ** 2
                draw_robot(x, y, theta, axes, transparency)


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
    dataframe = pd.read_csv(filename, sep=" ")

    # The columns are arbitrarily labelled by the first row.  We overwrite to
    # just use integer column names.
    dataframe.columns = [i for i in range(len(dataframe.columns))]

    return dataframe

main()
