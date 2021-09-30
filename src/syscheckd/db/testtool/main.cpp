/*
 * Wazuh FIMDB
 * Copyright (C) 2015-2021, Wazuh Inc.
 *
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <fstream>
#include <stdio.h>
#include <memory>
#include <json.hpp>
#include "dbsync.h"
#include "cmdArgsHelper.h"
#include "action.hpp"

static void loggerFunction(const char* msg)
{
    std::cout << "Msg: " << msg << std::endl;
}

int main(int argc, const char* argv[])
{

    CmdLineArgs args{argc, argv};


    return 0;
}
