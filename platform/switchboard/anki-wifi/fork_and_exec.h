/**
 * File: fork_and_exec.h
 *
 * Author: seichert
 * Created: 11/7/2017
 *
 * Description: Fork, Exec a process and capture output
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#pragma once
#include <ostream>
#include <string>
#include <vector>

namespace Anki {

int ForkAndExec(const std::vector<std::string>& args, std::ostream& out);
void KillChildProcess();

} // namespace Anki