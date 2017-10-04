
#include <mutex>
#include <set>
#include <algorithm>
#include <boost/timer/timer.hpp>

#include "user_manager.hpp"

namespace {
    int ratingTimeout = 1;
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

    void setRatingTimeout() {
        if(const char* env_p = std::getenv("RATING_TIMEOUT")) {
            try {
                ratingTimeout = std::stoi(env_p);
            }
            catch (std::exception& e) {
                std::cout << "Bad timeout value: " << e.what() << '\n';
            }
        }
    }
    
}

UserDatabase usersDB;
std::string currentUserId;
std::mutex usersDBMutex;
std::atomic_bool timeToExit(false);

UserManager& UserManager::getInstance() {
    static UserManager m;
    return m;
}

UserManager::UserManager() {
  setRatingTimeout();
  timerThread = std::thread( [=] {
      while(!timeToExit) {
	std::this_thread::sleep_for(std::chrono::seconds(ratingTimeout));
        std::cout << "=== Rating:\n";
	std::vector<UserDatabaseItem> usrs;
	RatingRequest req;
        req.userId = currentUserId;
	try {
	    boost::timer::auto_cpu_timer t;
	    getRating(req);
	    std::cout << "=== TOP " << req.topNum << " ===" << std::endl;
	    auto i = 1;
	    for (const auto& u : req.topRated) {
		std::cout << i << ". " << u.second.name << " --> " << u.second.totalRev << std::endl;
		i++;
	    }
	    std::cout << "=== USER " << " ===" << std::endl;
            i = req.bestNeigbourPos;
	    for (const auto& u : req.neighbours) {
                std::string mark;
                if (u.second.id == currentUserId)
                    mark = "* ";
		std::cout << mark << i << ". " << u.second.name << " --> " << u.second.totalRev << std::endl;
		i++;
	    }
	    std::cout << "=== EOF Rating\n";
	    std::cout << "=== Total users: " << req.totalUsers << " ===" << std::endl;
	}
	catch(UserManagerException & e) {
	    std::cout << "Failed to get rating: " << e.what() << std::endl;
	}
      }
  } );
}

UserManager::~UserManager()
{
  timeToExit = true;
  timerThread.join();
}

void UserManager::hadnleUserSetCurrent(const std::string& id)
{
    std::unique_lock<std::mutex> lock { usersDBMutex };
    if (usersDB.find(id) == usersDB.end()) {
      throw UserManagerException("user does not exist!");
    }
    currentUserId = id;
}

void UserManager::getRating(RatingRequest& req)
{
    req.topRated.clear();
    req.neighbours.clear();
    req.userPos = 0;
    req.bestNeigbourPos = 0;
  
    std::unique_lock<std::mutex> lock { usersDBMutex };

    // Check for outdated ratings
    for(auto& u : usersDB) {
      if (u.second.totalRev != 0) {
	if (!isCurrentWeek(u.second.lastDeal)) {
	  u.second.totalRev = 0;
	}
      }
    }

    // Make sorted snapshot of users database and fill top rated list
    typedef std::function<bool(const UserDatabaseItem&, const UserDatabaseItem&)> Comparator;
    Comparator compFunctor =
	[](const UserDatabaseItem& elem1, const UserDatabaseItem& elem2)
	{
	    return elem1.second.totalRev > elem2.second.totalRev;
	};
    std::multiset<UserDatabaseItem, Comparator> setOfUsers(
	usersDB.begin(), usersDB.end(), compFunctor);

    int n = std::min(setOfUsers.size(), req.topNum);
    req.topRated.reserve(n);
    std::copy_n(setOfUsers.begin(), n, std::back_inserter(req.topRated));
    req.totalUsers = setOfUsers.size();

    // If requested fill rating for the particular user
    if (!req.userId.empty()) {
	auto it = std::find_if(setOfUsers.begin(),
			       setOfUsers.end(),
			       [&](const UserDatabaseItem& p ) { return p.second.id == req.userId;});
	if (it == setOfUsers.end()) {
	    throw UserManagerException("cannot find user rating!");
	}
	auto fit = next(it);
	auto bit = it;
	if (bit != setOfUsers.begin())
	    bit = prev(bit);
	for (int i = 0; i < req.nearNum - 1; i++)
	{
	    if (fit != setOfUsers.end())
		fit = next(fit);
	    if (bit != setOfUsers.begin())
		bit = prev(bit);
	    if (fit == setOfUsers.end() && bit == setOfUsers.begin())
		break;
	}
	if (fit != setOfUsers.end())
	    fit = next(fit);
	req.bestNeigbourPos = std::distance(setOfUsers.begin(), bit) + 1;
	req.userPos = req.bestNeigbourPos + std::distance(bit, it);
	req.neighbours.reserve(std::distance(bit, fit));
        std::copy(bit, fit, std::back_inserter(req.neighbours));
    }
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
  usersDB.insert(UserDatabaseItem(id, ui));
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

void UserManager::hadnleUserDial(const std::string& id, const TimePoint& tp, const Rating& val) {
  std::unique_lock<std::mutex> lock { usersDBMutex };
  auto u = usersDB.find(id);
  if (u == usersDB.end()) {
    throw UserManagerException("user not registered!");
  }
  if (!u->second.connected) {
    throw UserManagerException("user not connected!");
  }
  if (u->second.totalRev != 0 && !isCurrentWeek(u->second.lastDeal)) {
    u->second.totalRev = 0;
  }
  if (isCurrentWeek(tp)) {
    u->second.totalRev += val;
    u->second.lastDeal = tp;
  }
}
