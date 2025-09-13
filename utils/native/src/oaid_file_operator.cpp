/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "oaid_hilog_wreapper.h"
#include "oaid_file_operator.h"

namespace OHOS {
namespace Cloud {
bool OAIDFileOperator::IsFileExsit(const std::string &fileName)
{
    if (fileName.empty()) {
        OAID_HILOGE(OAID_MODULE_COMMON, "filename is empty");
        return false;
    }
    if (access(fileName.c_str(), F_OK) != 0) {
        return false;
    }
    return true;
}

bool OAIDFileOperator::OpenAndReadFile(const std::string &fileName, std::string &destContent)
{
    std::ifstream inStream(fileName.c_str(), std::ios::in | std::ios::binary);
    if (inStream) {
        inStream.seekg(0, std::ios::end);
        destContent.resize(inStream.tellg());
        inStream.seekg(0, std::ios::beg);
        inStream.read(&destContent[0], destContent.size());
        inStream.close();
        OAID_HILOGE(OAID_MODULE_COMMON, "OpenAndReadFile success");
        return true;
    }
    OAID_HILOGE(OAID_MODULE_COMMON, "OpenAndReadFile failed");
    return false;
}

bool OAIDFileOperator::ClearFile(const std::string &fileName)
{
    struct stat statbuf {};
    if (lstat(fileName.c_str(), &statbuf) != 0) {
        OAID_HILOGE(OAID_MODULE_COMMON, "clear object is not file");
        return false;
    }
    if (S_ISREG(statbuf.st_mode)) {
        if (access(fileName.c_str(), F_OK) != 0) {
            OAID_HILOGE(OAID_MODULE_COMMON, "RemoveFile success, file is not exist");
            return true;
        }
        remove(fileName.c_str());
    }
    return true;
}
}
}