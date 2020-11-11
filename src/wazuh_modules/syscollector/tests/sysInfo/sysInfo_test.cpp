/*
 * Wazuh SysInfo
 * Copyright (C) 2015-2020, Wazuh Inc.
 * October 19, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#include "sysInfo_test.h"
#include "sysInfo.hpp"

void SysInfoTest::SetUp() {};

void SysInfoTest::TearDown()
{
};

using ::testing::_;
using ::testing::Return;

std::string SysInfo::getSerialNumber() const {return "";}
std::string SysInfo::getCpuName() const {return "";}
int SysInfo::getCpuMHz() const {return 0;}
int SysInfo::getCpuCores() const {return 0;}
void SysInfo::getMemory(nlohmann::json&) const {}
nlohmann::json SysInfo::getPackages() const {return "";}
nlohmann::json SysInfo::getOsInfo() const {return "";}
nlohmann::json SysInfo::getProcessesInfo() const {return {};}
nlohmann::json SysInfo::getNetworks() const {return {};}
nlohmann::json SysInfo::getPorts() const {return {};}

class SysInfoWrapper: public SysInfo
{
public:
    SysInfoWrapper() = default;
    ~SysInfoWrapper() = default;
    MOCK_METHOD(std::string, getSerialNumber, (), (const override));
    MOCK_METHOD(std::string, getCpuName, (), (const override));
    MOCK_METHOD(int, getCpuMHz, (), (const override));
    MOCK_METHOD(int, getCpuCores, (), (const override));
    MOCK_METHOD(void, getMemory, (nlohmann::json&), (const override));
    MOCK_METHOD(nlohmann::json, getPackages, (), (const override));
    MOCK_METHOD(nlohmann::json, getOsInfo, (), (const override));
    MOCK_METHOD(nlohmann::json, getProcessesInfo, (), (const override));
    MOCK_METHOD(nlohmann::json, getNetworks, (), (const override));
    MOCK_METHOD(nlohmann::json, getPorts, (), (const override));
};

TEST_F(SysInfoTest, hardware)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getSerialNumber()).WillOnce(Return("serial"));
    EXPECT_CALL(info, getCpuName()).WillOnce(Return("name"));
    EXPECT_CALL(info, getCpuCores()).WillOnce(Return(1));
    EXPECT_CALL(info, getCpuMHz()).WillOnce(Return(2902));
    EXPECT_CALL(info, getMemory(_));
    const auto result {info.hardware()};
    EXPECT_FALSE(result.empty());
}

TEST_F(SysInfoTest, packages)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getPackages()).WillOnce(Return("packages"));
    const auto result {info.packages()};
    EXPECT_FALSE(result.empty());
}

TEST_F(SysInfoTest, processes)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getProcessesInfo());
    const auto result {info.processes()};
    EXPECT_FALSE(result.empty());    
}

TEST_F(SysInfoTest, networks)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getNetworks());
    const auto result {info.networks()};
    EXPECT_FALSE(result.empty());    
}

TEST_F(SysInfoTest, ports)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getPorts());
    const auto result {info.ports()};
    EXPECT_FALSE(result.empty());
}

TEST_F(SysInfoTest, os)
{
    SysInfoWrapper info;
    EXPECT_CALL(info, getOsInfo());
    const auto result {info.os()};
    EXPECT_FALSE(result.empty());
}