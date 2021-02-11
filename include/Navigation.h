#pragma once

#include <experimental/filesystem>
#include <ros/package.h>

#include <iostream>
#include <string>

using namespace std;

class Navigation
{
public:
    Navigation();

    string get_ros_pkg_dir(string ros_pkg);
};