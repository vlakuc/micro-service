//
//  Created by Ivan Mejia on 12/24/16.
//
// MIT License
//
// Copyright (c) 2016 ivmeroLabs.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <std_micro_service.hpp>
#include "microsvc_controller.hpp"
#include "user_manager.hpp"

using namespace web;
using namespace http;

void MicroserviceController::initRestOpHandlers() {
    _listener.support(methods::GET, std::bind(&MicroserviceController::handleGet, this, std::placeholders::_1));
    _listener.support(methods::PUT, std::bind(&MicroserviceController::handlePut, this, std::placeholders::_1));
    _listener.support(methods::POST, std::bind(&MicroserviceController::handlePost, this, std::placeholders::_1));
    _listener.support(methods::DEL, std::bind(&MicroserviceController::handleDelete, this, std::placeholders::_1));
    _listener.support(methods::PATCH, std::bind(&MicroserviceController::handlePatch, this, std::placeholders::_1));
}

void MicroserviceController::handleGet(http_request message) {
    auto path = requestPath(message);
    if (!path.empty()) {
      //   message.relative_uri() 
        if (path[0] == "service" && path[1] == "test") {
            auto response = json::value::object();
            response["version"] = json::value::string("0.1.1");
            response["status"] = json::value::string("ready!");
            message.reply(status_codes::OK, response);
        }
    }
    else {
        message.reply(status_codes::NotFound);
    }
}

void MicroserviceController::handlePatch(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::PATCH));
}

void MicroserviceController::handlePut(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::PUT));
}

void MicroserviceController::handlePost(http_request message) {
  auto path = requestPath(message);
  if (!path.empty() && path[0] == "user") {
    message.
      extract_string().
      then([=](utility::string_t request) {
	  auto q = uri::split_query(request);
	  try {
	    if(path[1] == "registered") {
	      UserManager::getInstance().registerUser(q["id"], q["name"]);
	       
	       json::value response;
	       response["message"] = json::value::string(
							"succesful registration!");
	       message.reply(status_codes::OK, response);
	    }
	    else if(path[1] == "renamed") {
	       
	       UserManager::getInstance().hadnleUserRenamed(q["id"], q["name"]);
	       
	       json::value response;
	       response["message"] = json::value::string(
							"succesful rename!");
	       message.reply(status_codes::OK, response);

	    }
	    else if(path[1] == "connected") {
               std::string userId = q["id"];
	       UserManager::getInstance().hadnleUserConnected(userId);
	       json::value response;
	       response["message"] = json::value::string(
							"succesfuly connected!");
               RatingRequest req;
               req.userId = userId;
               UserManager::getInstance().getRating(req);
               auto i = 1;
               std::vector<json::value> vals;
               vals.reserve(req.topRated.size()); 
               for(const auto& u : req.topRated) {
                   json::value pos;
                   pos["position"] = i;
                   pos["name"] = json::value::string(u.second.name);
                   pos["rating"] = u.second.totalRev;
                   vals.push_back(pos);
                   i++;
               }

	       response["top_rated"] = json::value::array(vals);

               vals.clear();
               vals.reserve(req.neighbours.size());
               i = req.bestNeigbourPos;
               for(const auto& u : req.neighbours) {
                   json::value pos;
                   pos["position"] = i;
                   pos["name"] = json::value::string(u.second.name);
                   pos["rating"] = u.second.totalRev;
                   pos["is_current"] = u.second.id == userId;
                   vals.push_back(pos);
                   i++;
               }
               response["neigbour_list"] = json::value::array(vals);

	       message.reply(status_codes::OK, response);

	    }
	    else if(path[1] == "disconnected") {
	       UserManager::getInstance().hadnleUserDisconnected(q["id"]);
	       
	       json::value response;
	       response["message"] = json::value::string(
							"succesfuly disconnected!");
	       message.reply(status_codes::OK, response);

	    }
	    else if(path[1] == "deal") {
               Rating r {};
	       std::string s = q["amount"];
	       if (!s.empty())
		 r = std::stof(s);
	       
	       uint64_t t {};
	       s = q["time"];
	       if (!s.empty())
		 t = std::stoull(s);

	       TimePoint tp {std::chrono::nanoseconds(t)};
	       if (!t)
		 tp = Clock::now();
	       
	       UserManager::getInstance().hadnleUserDial(q["id"], tp, r);
	       
	       json::value response;
	       response["message"] = json::value::string(
							"succesful deal!");
	       message.reply(status_codes::OK, response);

	    }
	    else if(path[1] == "current") {
	       UserManager::getInstance().hadnleUserSetCurrent(q["id"]);
	       
	       json::value response;
	       response["message"] = json::value::string(
							"succesful!");
	       message.reply(status_codes::OK, response);

	    }


	  }
	  catch(UserManagerException & e) {
	    message.reply(status_codes::BadRequest, e.what());
	  }
	  catch(std::exception& e) {
	    message.reply(status_codes::BadRequest, e.what());
	  }
	});
  }
}


void MicroserviceController::handleDelete(http_request message) {    
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::DEL));
}

void MicroserviceController::handleHead(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::HEAD));
}

void MicroserviceController::handleOptions(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::OPTIONS));
}

void MicroserviceController::handleTrace(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::TRCE));
}

void MicroserviceController::handleConnect(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::CONNECT));
}

void MicroserviceController::handleMerge(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::MERGE));
}

json::value MicroserviceController::responseNotImpl(const http::method & method) {
    auto response = json::value::object();
    response["serviceName"] = json::value::string("C++ Mircroservice Sample");
    response["http_method"] = json::value::string(method);
    return response ;
}
