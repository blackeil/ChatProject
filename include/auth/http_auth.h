#pragma once

#include <string>
#include "httplib.h"

std::string extract_token_from_request(const httplib::Request& req);

