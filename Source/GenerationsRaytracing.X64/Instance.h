﻿#pragma once

struct Instance
{
    float transform[3][4]{};
    unsigned int bottomLevelAS = 0;
    unsigned int instanceInfo = 0;
    unsigned int instanceMask = 0;
};
