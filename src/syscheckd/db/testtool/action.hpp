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
#ifndef _ACTION_H
#define _ACTION_H
#include <json.hpp>
#include <mutex>
#include "fimDB.hpp"

class TestAction
{
public:
    TestAction();
    virtual void execute(const nlohmann::json& action_data);
    virtual ~TestAction();
protected:
    std::string m_dbPath;
    std::string m_outPath;
    std::unique_ptr<FIMDB>  m_database;
    int m_actionId;
};

class InsertAction final : public TestAction
{
    void execute(const nlohmann::json& action_data) override
    {
    }
};

class UpdateAction final : public TestAction
{
    void execute(const nlohmann::json& action_data) override
    {
    }
};

class RemoveAction final : public TestAction
{
    void execute(const nlohmann::json& action_data) override
    {
    }
};


#endif //_ACTION_H
