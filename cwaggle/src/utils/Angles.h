/**
 * Some common calculations on angles.
 *
 * @author Andrew Vardy
 */
#ifndef ANGLES_H
#define ANGLES_H

#include <cassert>
#include <cmath>
#include <iostream>
using namespace std;

#define PI M_PI
#define TWO_PI (2.0 * M_PI)
#define PI_OVER_180 (M_PI / 180.0)
#define PI_OVER_2 (M_PI / 2.0)
#define ONE_EIGHTY_OVER_PI (180.0 / M_PI)
#define SQRT_2_OVER_2 (M_SQRT2 / 2.0);

namespace Angles {

// Constrain the given angle to the range (-Pi, Pi].
double constrainAngle(double a)
{
    // BAD: These loops should be replaced.
    while (a > M_PI) {
        a -= 2 * M_PI;
    }
    while (a <= -M_PI) {
        a += 2 * M_PI;
    }
    return a;
}

double getAngularDifference(double angleA, double angleB)
{
    angleA = constrainAngle(angleA);
    angleB = constrainAngle(angleB);
    double error = fabs(angleA - angleB);
    if (error > M_PI) {
        error -= (double)M_PI * 2;
        error = fabs(error);
    }
    return error;
}

/** 
 * Return the angle between the two given angles with the smallest absolute
 * value.  Meanwhile, the value returned will have a sign.
 */
double getSmallestSignedAngularDifference(double a, double b)
{
    // https://stackoverflow.com/questions/1878907/the-smallest-difference-between-2-angles
    return atan2(sin(a-b), cos(a-b));
}


double int2angle(int i, int topI)
{
    return (double)(TWO_PI * i / topI);
}

int angle2int(double angle, int topI)
{
    return (int)(topI * angle / TWO_PI + 0.5);
}

};

#endif
