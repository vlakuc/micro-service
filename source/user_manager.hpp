#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>

#include <std_micro_service.hpp>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;


struct UserInformation {
   UserInformation() : totalRev(0), connected(false) {}
  std::string id;
  std::string name;
  TimePoint lastDeal;
  float totalRev;
  bool connected;
};


using UserDatabase = std::unordered_map<std::string, UserInformation>;
using UserDatabaseItem = std::pair<std::string, UserInformation>;
using UserList = std::vector<UserDatabaseItem>;

struct RatingRequest {
  UserList topRated;
  UserList neighbours;
  std::string userId;
  size_t userPos = 0;
  size_t bestNeigbourPos = 0;
  size_t topNum = 10;
  size_t nearNum = 10;
};

class UserManagerException : public std::exception {
  std::string _message;
public:
  UserManagerException(const std::string & message) :
    _message(message) { }
  const char * what() const throw() {
    return _message.c_str();
  }
};


class UserManager {

public:

  static UserManager& getInstance();

  void registerUser(const std::string& id,
		    const std::string& name);

  void hadnleUserConnected(const std::string& id);

  void hadnleUserDisconnected(const std::string& id);

  void hadnleUserRenamed(const std::string& id,
			 const std::string& newName);
  
  void hadnleUserDial(const std::string& id, const TimePoint& tp, const float val);

  void hadnleUserSetCurrent(const std::string& id);

  void getRating(RatingRequest& req);

private:

  UserManager();

  std::thread timerThread;


};
