/* ************************************************************************
 * Copyright 2015 Vratis, Ltd.
 *
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
 * ************************************************************************ */

#pragma once
#ifndef _KERNEL_CAHCE_HPP_
#define _KERNEL_CAHCE_HPP_

#if MLOpen_BACKEND_OPENCL

#include <string>
#include <unordered_map>
#include "MLOpen.h"
#include "OCLKernel.hpp"
#include "OCL/CLHelper.hpp"

struct SimpleHash {
	size_t operator()(const std::pair<std::string, std::string>& p) const {
		using std::hash;
		return (hash<std::string>()(p.first) ^ hash<std::string>()(p.second));
	}
};

/**
 * @brief The KernelCache class Build and cache kernels
 * singleton
 */
class KernelCache
{

public:

	typedef std::pair<std::string, std::string> Key;
    typedef std::unordered_map< Key, OCLKernel, SimpleHash > KernelMap;

    static KernelCache& getInstance();

	static OCLKernel get(cl_command_queue &queue,
						 const std::string& algorithm,
						 const std::string& network_config,
                         const std::string& program_name,
                         const std::string& kernel_name,
						 const std::vector<size_t>& ldims,
						 const std::vector<size_t>& gdims,
                         const std::string& params = "");

	static OCLKernel get( const std::string& algorithm,
						 const std::string& network_config);

    mlopenStatus_t getProgram(cl_program &program,
							cl_command_queue& queue,
                              const std::string& program_name,
                              const std::string& params = "");

	OCLKernel getKernel(cl_command_queue &queue,
						 const std::string& algorithm,
						 const std::string& network_config,
                         const std::string& program_name,
                         const std::string& kernel_name,
						 const std::vector<size_t>& ldims,
						 const std::vector<size_t>& gdims,
                         const std::string& params = "");
	
	OCLKernel getKernel( const std::string& algorithm,
						 const std::string& network_config);


private:

    KernelMap kernel_map;

    KernelCache();

    static KernelCache singleton;
	
};

#endif // MLOpen_BACKEND_OPENCL

#endif //_KERNEL_CACHE_HPP_