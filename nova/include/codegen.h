#pragma once
#include "ast.h"
#include <string>

void codegenProgram(const ProgramNode& program, const std::string& outputFile,
                    const std::string& filename, int optLevel = 0);