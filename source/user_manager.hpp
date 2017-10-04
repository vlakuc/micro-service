#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>

#include <std_micro_service.hpp>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::nanoseconds>;
using Rating = float;

struct UserInformation {
  UserInformation() : totalRev(0), connected(false) {}
  UserInformation(const std::string& uid, const std::string& uname):
    id(uid), name(uname), totalRev(0), connected(false) {}

  std::string id;
  std::string name;
  TimePoint lastDeal;
  Rating totalRev;
  bool connected;
};


using UserDatabase = std::unordered_map<std::string, UserInformation>;
using UserDatabaseItem = std::pair<std::string, UserInformation>;
using UserList = std::vector<UserDatabaseItem>;


struct RatingRequest {
  UserList topRated;           // OUT: first [topNum] users in the rating 
  UserList neighbours;         // OUT: [userId] and +/- [nearNum] users in the rating  
  std::string userId;          // IN: ID of the user to get rating for
  size_t userPos = 0;          // OUT: [userId] position in the rating
  size_t bestNeigbourPos = 0;  // OUT: position of the user with the highest rating from +/- [nearNum] group
  size_t topNum = 10;          // IN: number of users in the top list
  size_t nearNum = 10;         // IN: number of users with higher and lower rating than [userId] to be included in the list
  size_t totalUsers = 0;       // OUT: number of users in the database
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
  
  void hadnleUserDial(const std::string& id, const TimePoint& tp, const Rating& val);

  void hadnleUserSetCurrent(const std::string& id);

  void getRating(RatingRequest& req);

private:

  UserManager();
  ~UserManager();

  std::thread timerThread;


};
