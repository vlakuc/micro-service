
#include <mutex>
#include <set>
#include <algorithm>

#include "user_manager.hpp"

namespace {

    const int TOP_RATED_NUM = 15;

    // Week number of the year
    // (Monday as the first day of the week) as a decimal number [00,53].
    // All days in a new year preceding the first Monday are considered to be in week 0.
    int getWeekNum(const TimePoint& tp) {
        std::tm tm = {0};
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        gmtime_r(&tt, &tm);
        return (tm.tm_yday + 7 - (tm.tm_wday ? (tm.tm_wday - 1) : 6)) / 7;
    }
  
    bool isCurrentWeek(const TimePoint& tp) {
        TimePoint cur = Clock::now();
        return getWeekNum(cur) == getWeekNum(tp);
    }

    std::vector<UserDatabaseItem> getRated(const UserDatabase& db) {
        std::vector<UserDatabaseItem> res;
        typedef std::function<bool(const UserDatabaseItem&, const UserDatabaseItem&)> Comparator;
        Comparator compFunctor =
            [](const UserDatabaseItem& elem1, const UserDatabaseItem& elem2)
            {
                return elem1.second.totalRev > elem2.second.totalRev;
            };
        std::multiset<UserDatabaseItem, Comparator> setOfUsers(
            db.begin(), db.end(), compFunctor);

        int n = std::min(static_cast<int>(setOfUsers.size()), TOP_RATED_NUM); 
        std::copy_n(setOfUsers.begin(), n, std::back_inserter(res));
        return res;
    }
}

UserDatabase usersDB;
std::mutex usersDBMutex;

UserManager& UserManager::getInstance() {
    static UserManager m;
    return m;
}

UserManager::UserManager() {
  timerThread = std::thread( [=] {
      for(;;) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Rating:\n";
	std::unique_lock<std::mutex> lock { usersDBMutex };
	std::vector<UserDatabaseItem> usrs = getRated(usersDB);
        for (const auto& u : usrs) {
	  std::cout << u.second.name << " -> " << u.second.totalRev << std::endl;
	}
        std::cout << "EOF Rating\n";
      }
  } );
    timerThread.detach();
}

void UserManager::registerUser(const std::string& id,
			       const std::string& name) {

  if (id.empty()) {
    throw UserManagerException("empty user id!");
  }
  
  if (name.empty()) {
    throw UserManagerException("empty user name!");
  }

  std::unique_lock<std::mutex> lock { usersDBMutex };

  if (usersDB.find(id) != usersDB.end()) {
    throw UserManagerException("user already exists!");
  }
  UserInformation ui;
  ui.id = id;
  ui.name = name;
  usersDB.insert(
		 std::pair<std::string, UserInformation>(id,
							 ui));
}

void UserManager::hadnleUserConnected(const std::string& id) {
  std::unique_lock<std::mutex> lock { usersDBMutex };
  auto u = usersDB.find(id);
  if (u == usersDB.end()) {
    throw UserManagerException("user not registered!");
  }
  if (u->second.connected) {
    throw UserManagerException("user already connected!");
  }

  u->second.connected = true;
}

void UserManager::hadnleUserDisconnected(const std::string& id) {
  std::unique_lock<std::mutex> lock { usersDBMutex };
  auto u = usersDB.find(id);
  if (u == usersDB.end()) {
    throw UserManagerException("user not registered!");
  }
  if (!u->second.connected) {
    throw UserManagerException("user not connected!");
  }
  u->second.connected = false;
}

void UserManager::hadnleUserRenamed(const std::string& id,
				    const std::string& newName) {
  if (id.empty()) {
    throw UserManagerException("empty user id!");
  }
  
  if (newName.empty()) {
    throw UserManagerException("empty user name!");
  }
  std::unique_lock<std::mutex> lock { usersDBMutex };
  auto u = usersDB.find(id);
  if (u == usersDB.end()) {
    throw UserManagerException("user not registered!");
  }
  u->second.name = newName;
}

void UserManager::hadnleUserDial(const std::string& id, const TimePoint& tp, const float val) {
  std::unique_lock<std::mutex> lock { usersDBMutex };
  auto u = usersDB.find(id);
  if (u == usersDB.end()) {
    throw UserManagerException("user not registered!");
  }
  if (!u->second.connected) {
    throw UserManagerException("user not connected!");
  }
  if (!isCurrentWeek(u->second.lastDeal)) {
    u->second.totalRev = 0;
  }
  if (isCurrentWeek(tp)) {
    u->second.totalRev += val;
    u->second.lastDeal = tp;
  }
}
